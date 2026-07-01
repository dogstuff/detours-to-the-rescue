#!/usr/bin/env python3
"""Prepare a build-tree Zensical source tree with SDK symbol reference pages."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SDK_SCRIPT_DIR = REPO_ROOT / "modules/sdk/scripts"
PCDOGS_BLUEPRINT = REPO_ROOT / "modules/sdk/blueprints/dttr_pcdogs.py"

sys.path.insert(0, str(SDK_SCRIPT_DIR))
from symbol_manifest import MANIFEST_FILENAME, SCHEMA_FILENAME  # noqa: E402

SYMBOL_REFERENCE_TITLE = "PCDogs Symbols"
SYMBOL_REFERENCE_BASE_PATH = "modding-sdk/symbols/pcdogs"
SYMBOL_REFERENCE_OVERVIEW_PATH = f"{SYMBOL_REFERENCE_BASE_PATH}/index.md"


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

    if overrides_dir.exists():
        validate_source_tree(overrides_dir)
        shutil.copytree(overrides_dir, overrides_out)

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

    (output_dir / "zensical.toml").write_text(config_path.read_text())

    write_docs_manifest(
        output_dir=output_dir,
        source_dir=source_dir,
        config_path=config_path,
        metadata_paths=metadata_paths,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
