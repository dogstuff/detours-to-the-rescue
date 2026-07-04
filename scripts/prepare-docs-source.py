#!/usr/bin/env python3
"""Prepare a build-tree Zensical source tree with SDK symbol reference pages."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SDK_SCRIPT_DIR = REPO_ROOT / "modules/sdk/scripts"
PCDOGS_BLUEPRINT = REPO_ROOT / "modules/sdk/blueprints/dttr_pcdogs.py"
ICON_SOURCE = REPO_ROOT / "assets/icons/dttr.svg"

sys.path.insert(0, str(SDK_SCRIPT_DIR))
from symbol_manifest import MANIFEST_FILENAME, SCHEMA_FILENAME  # noqa: E402

SYMBOL_REFERENCE_TITLE = "PCDogs Symbols"
SYMBOL_REFERENCE_BASE_PATH = "modding-sdk/symbols/pcdogs"
SYMBOL_REFERENCE_OVERVIEW_PATH = f"{SYMBOL_REFERENCE_BASE_PATH}/index.md"
VERSION_TAG_RE = re.compile(r"^v(\d+)\.(\d+)\.(\d+)(?:-(preview|rc)\.(\d+))?$")
STABLE_RELEASE_FLOOR = (3, 0, 0)


@dataclass(frozen=True, slots=True)
class VersionTag:
    tag: str
    major: int
    minor: int
    patch: int
    prerelease: str | None = None
    prerelease_number: int = 0

    @property
    def is_stable(self) -> bool:
        return self.prerelease is None

    @property
    def version(self) -> tuple[int, int, int]:
        return (self.major, self.minor, self.patch)

    @property
    def sort_key(self) -> tuple[int, int, int, int, int]:
        prerelease_rank = {
            None: 3,
            "rc": 2,
            "preview": 1,
        }[self.prerelease]
        return (*self.version, prerelease_rank, self.prerelease_number)


@dataclass(frozen=True, slots=True)
class DocsDownloadVersions:
    stable: str
    nightly: str


@dataclass(frozen=True, slots=True)
class PCDOGSFixture:
    build: str
    filename: str

    def path_in(self, directory: Path) -> Path:
        return directory / self.filename


@dataclass(frozen=True, slots=True)
class XRefMetadataJob:
    script_name: str
    output_stem: str
    arg_name: str

    @property
    def script_path(self) -> Path:
        return SDK_SCRIPT_DIR / self.script_name

    def output_path(self, metadata_dir: Path, fixture: PCDOGSFixture) -> Path:
        return metadata_dir / f"{self.output_stem}-{fixture.build.lower()}.json"


@dataclass(frozen=True, slots=True)
class XRefMetadataOutput:
    arg_name: str
    path: Path


PCDOGS_FIXTURES = (
    PCDOGSFixture("EN", "pcdogs_en.exe"),
    PCDOGSFixture("EU", "pcdogs_eu.exe"),
    PCDOGSFixture("SC", "pcdogs_sc.exe"),
)

XREF_METADATA_JOBS = (
    XRefMetadataJob(
        "resolve_symbol_addresses.py",
        "symbol-addresses",
        "--symbol-addresses",
    ),
    XRefMetadataJob(
        "generate_function_call_xrefs.py",
        "function-call-xrefs",
        "--function-call-xrefs",
    ),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Copy docs sources and add SDK symbol reference pages.",
    )
    parser.add_argument("--source-dir", type=Path, default=Path("docs/pages"))
    parser.add_argument("--config", type=Path, default=Path("docs/zensical.toml"))
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--pcdogs-fixtures-dir",
        type=Path,
        default=Path(os.environ.get("PCDOGS_FIXTURES_DIR", "gamefiles")),
        help="Directory containing required pcdogs_*.exe files for symbol metadata.",
    )
    return parser.parse_args()


def missing_pcdogs_fixtures(pcdogs_fixtures_dir: Path) -> list[Path]:
    missing: list[Path] = []
    for fixture in PCDOGS_FIXTURES:
        path = fixture.path_in(pcdogs_fixtures_dir)

        if not path.is_file():
            missing.append(path)

    return missing


def resolved_path(path: Path) -> Path:
    return path.resolve(strict=False)


def paths_overlap(left: Path, right: Path) -> bool:
    return left == right or left in right.parents or right in left.parents


def validate_output_dir(
    output_dir: Path, *, source_dir: Path, config_path: Path
) -> Path:
    output_dir = resolved_path(output_dir)
    exact_protected = {
        "repository root": REPO_ROOT,
        "current working directory": resolved_path(Path.cwd()),
    }
    overlap_protected = {
        "source docs directory": resolved_path(source_dir),
        "docs config": resolved_path(config_path),
    }

    if output_dir == Path(output_dir.anchor):
        raise ValueError(f"refusing to delete filesystem root: {output_dir}")

    for label, path in exact_protected.items():
        if output_dir == path:
            raise ValueError(f"refusing output dir {output_dir}: matches {label}")

    for label, path in overlap_protected.items():
        if paths_overlap(output_dir, path):
            raise ValueError(
                f"refusing output dir {output_dir}: overlaps {label} {path}"
            )

    return output_dir


def validate_source_tree(source_dir: Path) -> None:
    source_dir = resolved_path(source_dir)
    for path in source_dir.rglob("*"):
        if not path.is_symlink():
            continue

        target = resolved_path(path)
        if source_dir not in target.parents and target != source_dir:
            raise ValueError(f"refusing out-of-tree docs symlink: {path} -> {target}")


def run_checked(command: list[str], *, label: str) -> None:
    try:
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"{label} failed with exit code {exc.returncode}") from exc


def parse_version_tag(tag: str) -> VersionTag | None:
    match = VERSION_TAG_RE.fullmatch(tag)
    if not match:
        return None

    prerelease = match.group(4)
    prerelease_number = int(match.group(5) or 0)

    return VersionTag(
        tag=tag,
        major=int(match.group(1)),
        minor=int(match.group(2)),
        patch=int(match.group(3)),
        prerelease=prerelease,
        prerelease_number=prerelease_number,
    )


def version_download_url(version: str, artifact: str) -> str:
    return (
        "https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/"
        f"{version}/downloads/{artifact}"
    )


def git_version_tags() -> list[VersionTag]:
    try:
        result = subprocess.run(
            ["git", "tag", "--list", "v*"],
            cwd=REPO_ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        tags: list[str] = []
    else:
        tags = [line.strip() for line in result.stdout.splitlines()]

    ci_tag = os.environ.get("CI_COMMIT_TAG", "").strip()
    if ci_tag:
        tags.append(ci_tag)

    parsed = [version for tag in tags if (version := parse_version_tag(tag))]
    return sorted(set(parsed), key=lambda version: version.sort_key)


def latest_version_tag(*, stable: bool) -> VersionTag | None:
    versions = git_version_tags()
    if stable:
        versions = [
            version
            for version in versions
            if version.is_stable and version.version >= STABLE_RELEASE_FLOOR
        ]

    return versions[-1] if versions else None


def resolve_docs_download_versions() -> DocsDownloadVersions:
    nightly = os.environ.get("DTTR_DOCS_NIGHTLY_VERSION", "").strip()
    stable = os.environ.get("DTTR_DOCS_STABLE_VERSION", "").strip()

    if nightly and parse_version_tag(nightly) is None:
        raise ValueError(f"invalid DTTR_DOCS_NIGHTLY_VERSION: {nightly}")

    if stable and parse_version_tag(stable) is None:
        raise ValueError(f"invalid DTTR_DOCS_STABLE_VERSION: {stable}")

    if not nightly:
        latest = latest_version_tag(stable=False)
        if latest is None:
            raise ValueError(
                "could not resolve docs nightly version; fetch git tags or set "
                "DTTR_DOCS_NIGHTLY_VERSION"
            )

        nightly = latest.tag

    if not stable:
        latest_stable = latest_version_tag(stable=True)
        stable = latest_stable.tag if latest_stable else nightly

    return DocsDownloadVersions(stable=stable, nightly=nightly)


def docs_download_replacements() -> dict[str, str]:
    versions = resolve_docs_download_versions()
    return {
        "__DTTR_DOCS_STABLE_VERSION__": versions.stable,
        "__DTTR_DOCS_NIGHTLY_VERSION__": versions.nightly,
        "__DTTR_DOCS_VANILLA_STABLE_DOWNLOAD_URL__": version_download_url(
            versions.stable,
            "dttr-release.zip",
        ),
        "__DTTR_DOCS_VANILLA_NIGHTLY_DOWNLOAD_URL__": version_download_url(
            versions.nightly,
            "dttr-release.zip",
        ),
        "__DTTR_DOCS_MODDING_STABLE_DOWNLOAD_URL__": version_download_url(
            versions.stable,
            "dttr-modding-release.zip",
        ),
        "__DTTR_DOCS_MODDING_NIGHTLY_DOWNLOAD_URL__": version_download_url(
            versions.nightly,
            "dttr-modding-release.zip",
        ),
        "__DTTR_DOCS_MOD_TEMPLATE_STABLE_DOWNLOAD_URL__": version_download_url(
            versions.stable,
            "dttr-mod-template-c.zip",
        ),
        "__DTTR_DOCS_MOD_TEMPLATE_NIGHTLY_DOWNLOAD_URL__": version_download_url(
            versions.nightly,
            "dttr-mod-template-c.zip",
        ),
    }


def resolve_docs_download_placeholders(
    text: str, replacements: Mapping[str, str] | None = None
) -> str:
    if replacements is None:
        replacements = docs_download_replacements()

    for token, value in replacements.items():
        text = text.replace(token, value)

    return text


def resolve_docs_config(config_text: str) -> str:
    return resolve_docs_download_placeholders(config_text)


def resolve_docs_page_placeholders(
    pages_out: Path, replacements: Mapping[str, str]
) -> None:
    for path in pages_out.rglob("*.md"):
        text = path.read_text()
        resolved = resolve_docs_download_placeholders(text, replacements)
        if resolved != text:
            path.write_text(resolved)


def prepare_symbol_reference_docs(
    symbols_root_dir: Path, *, metadata_dir: Path, pcdogs_fixtures_dir: Path
) -> list[Path]:
    # This directory also contains copied hand-authored wrapper pages.
    symbols_root_dir.mkdir(parents=True, exist_ok=True)

    xref_metadata = generate_xref_metadata(
        metadata_dir,
        pcdogs_fixtures_dir=pcdogs_fixtures_dir,
    )
    metadata_args = [
        value for item in xref_metadata for value in (item.arg_name, str(item.path))
    ]

    manifest_dir = symbols_root_dir / "pcdogs"

    run_checked(
        [
            sys.executable,
            str(SDK_SCRIPT_DIR / "generate_symbol_metadata.py"),
            "--metadata-dir",
            str(manifest_dir),
            str(PCDOGS_BLUEPRINT),
            *metadata_args,
        ],
        label="symbol manifest generation",
    )
    return [
        *(item.path for item in xref_metadata),
        manifest_dir / MANIFEST_FILENAME,
        manifest_dir / SCHEMA_FILENAME,
    ]


def generate_xref_metadata(
    metadata_dir: Path, *, pcdogs_fixtures_dir: Path
) -> list[XRefMetadataOutput]:
    metadata: list[XRefMetadataOutput] = []
    missing = missing_pcdogs_fixtures(pcdogs_fixtures_dir)
    if missing:
        print(
            "missing PCDOGS fixtures; generating docs SymbolManifest without "
            "analysis: " + ", ".join(str(path) for path in missing),
            file=sys.stderr,
        )
        return metadata

    metadata_dir.mkdir(parents=True, exist_ok=True)

    for fixture in PCDOGS_FIXTURES:
        pcdogs_fixture_path = fixture.path_in(pcdogs_fixtures_dir)

        for job in XREF_METADATA_JOBS:
            output_path = job.output_path(metadata_dir, fixture)
            run_checked(
                [
                    sys.executable,
                    str(job.script_path),
                    "--blueprint",
                    str(PCDOGS_BLUEPRINT),
                    "--gamefile",
                    str(pcdogs_fixture_path),
                    "--build",
                    fixture.build,
                    "--output",
                    str(output_path),
                ],
                label=f"{job.output_stem} metadata generation for {fixture.build}",
            )
            metadata.append(XRefMetadataOutput(job.arg_name, output_path))

    return metadata


def render_icon_png(output_path: Path, size: int, *, label: str) -> Path:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    run_checked(
        [
            "rsvg-convert",
            "-w",
            str(size),
            "-h",
            str(size),
            "-f",
            "png",
            "-o",
            str(output_path),
            str(ICON_SOURCE),
        ],
        label=label,
    )
    return output_path


def prepare_docs_brand_assets(pages_out: Path) -> list[Path]:
    images_dir = pages_out / "assets/images"
    return [
        render_icon_png(
            images_dir / "favicon.png",
            48,
            label="docs favicon generation",
        ),
        render_icon_png(
            images_dir / "logo.png",
            64,
            label="docs logo generation",
        ),
    ]


def relative_to_output(path: Path, output_dir: Path) -> str:
    try:
        return path.relative_to(output_dir).as_posix()
    except ValueError:
        return path.as_posix()


def write_docs_manifest(
    *,
    output_dir: Path,
    source_dir: Path,
    config_path: Path,
    metadata_paths: list[Path],
) -> Path:
    manifest = {
        "schema_version": 1,
        "source_dir": source_dir.as_posix(),
        "config_path": config_path.as_posix(),
        "symbols": {
            "symbol_wrappers": {
                "base_path": SYMBOL_REFERENCE_BASE_PATH,
                "overview": {
                    "title": SYMBOL_REFERENCE_TITLE,
                    "path": SYMBOL_REFERENCE_OVERVIEW_PATH,
                },
            }
        },
        "metadata": [relative_to_output(path, output_dir) for path in metadata_paths],
    }
    manifest_path = output_dir / "docs-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest_path


def main() -> int:
    args = parse_args()
    source_dir = resolved_path(args.source_dir)
    config_path = resolved_path(args.config)

    try:
        validate_source_tree(source_dir)
        output_dir = validate_output_dir(
            args.output_dir,
            source_dir=source_dir,
            config_path=config_path,
        )
        replacements = docs_download_replacements()
        docs_config = resolve_docs_download_placeholders(
            config_path.read_text(), replacements
        )
    except (OSError, ValueError) as exc:
        print(exc, file=sys.stderr)
        return 1

    pages_out = output_dir / "pages"
    overrides_dir = config_path.parent / "overrides"
    overrides_out = output_dir / "overrides"
    metadata_dir = output_dir / "metadata" / "symbols"
    if output_dir.exists():
        shutil.rmtree(output_dir)

    pages_out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source_dir, pages_out)
    resolve_docs_page_placeholders(pages_out, replacements)

    if overrides_dir.exists():
        validate_source_tree(overrides_dir)
        shutil.copytree(overrides_dir, overrides_out)

    try:
        brand_asset_paths = prepare_docs_brand_assets(pages_out)
    except (FileNotFoundError, RuntimeError) as exc:
        print(exc, file=sys.stderr)
        return 1

    symbols_root_dir = pages_out / "modding-sdk" / "symbols"
    try:
        metadata_paths = prepare_symbol_reference_docs(
            symbols_root_dir,
            metadata_dir=metadata_dir,
            pcdogs_fixtures_dir=args.pcdogs_fixtures_dir,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        print(exc, file=sys.stderr)
        return 1

    (output_dir / "zensical.toml").write_text(docs_config)

    write_docs_manifest(
        output_dir=output_dir,
        source_dir=source_dir,
        config_path=config_path,
        metadata_paths=[*brand_asset_paths, *metadata_paths],
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
