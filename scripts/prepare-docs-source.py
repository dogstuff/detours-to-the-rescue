#!/usr/bin/env python3
"""Prepare a build-tree Zensical source tree with SDK symbol reference pages."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SDK_SCRIPT_DIR = REPO_ROOT / "modules/sdk/scripts"
PCDOGS_BLUEPRINT = REPO_ROOT / "modules/sdk/blueprints/dttr_pcdogs.py"
SYMBOL_REFERENCE_TITLE = "PCDogs Symbols"
SYMBOL_REFERENCE_BASE_PATH = "modding-sdk/symbols/pcdogs"
SYMBOL_REFERENCE_OVERVIEW_PATH = f"{SYMBOL_REFERENCE_BASE_PATH}/index.md"
EXPECTED_SYMBOL_NAV_ENTRY = f"""    {{ "{SYMBOL_REFERENCE_TITLE}" = [
      {{ "{SYMBOL_REFERENCE_TITLE}" = "{SYMBOL_REFERENCE_OVERVIEW_PATH}" }},
    ] }},"""


@dataclass(frozen=True, slots=True)
class GamefileBuild:
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

    def output_path(self, metadata_dir: Path, build: GamefileBuild) -> Path:
        return metadata_dir / f"{self.output_stem}-{build.build.lower()}.json"


@dataclass(frozen=True, slots=True)
class XRefMetadataOutput:
    arg_name: str
    path: Path


@dataclass(frozen=True, slots=True)
class SymbolOverviewManifest:
    title: str
    path: str


@dataclass(frozen=True, slots=True)
class SymbolWrapperManifest:
    base_path: str
    overview: SymbolOverviewManifest


@dataclass(frozen=True, slots=True)
class SymbolReferenceManifest:
    symbol_wrappers: SymbolWrapperManifest


@dataclass(frozen=True, slots=True)
class DocsManifest:
    schema_version: int
    source_dir: str
    config_path: str
    symbols: SymbolReferenceManifest
    metadata: list[str]


GAMEFILE_BUILDS = (
    GamefileBuild("EN", "pcdogs_en.exe"),
    GamefileBuild("EU", "pcdogs_eu.exe"),
    GamefileBuild("SC", "pcdogs_sc.exe"),
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
        "--gamefile-dir",
        type=Path,
        default=Path(os.environ.get("DTTR_PCDOGS_FIXTURE_DIR", "gamefiles")),
        help="Directory containing required pcdogs_*.exe files for symbol metadata.",
    )
    return parser.parse_args()


def require_gamefiles(gamefile_dir: Path) -> None:
    missing = [
        gamefile.path_in(gamefile_dir)
        for gamefile in GAMEFILE_BUILDS
        if not gamefile.path_in(gamefile_dir).is_file()
    ]
    if missing:
        raise FileNotFoundError(
            "missing required PCDOGS gamefiles for function-call xrefs: "
            + ", ".join(str(path) for path in missing)
        )


def resolved_path(path: Path) -> Path:
    return path.resolve(strict=False)


def paths_overlap(left: Path, right: Path) -> bool:
    return left == right or left in right.parents or right in left.parents


def validate_symbol_nav_entry(config_path: Path) -> None:
    config_text = config_path.read_text()
    if EXPECTED_SYMBOL_NAV_ENTRY not in config_text:
        raise ValueError(f"could not find the PCDogs symbol nav entry in {config_path}")


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
    symbols_root_dir: Path, *, metadata_dir: Path, gamefile_dir: Path
) -> list[XRefMetadataOutput]:
    # This directory also contains copied hand-authored wrapper pages.
    symbols_root_dir.mkdir(parents=True, exist_ok=True)

    metadata = generate_xref_metadata(metadata_dir, gamefile_dir=gamefile_dir)
    metadata_args = [
        value for item in metadata for value in (item.arg_name, str(item.path))
    ]
    run_checked(
        [
            sys.executable,
            str(SDK_SCRIPT_DIR / "generate_symbol_docs.py"),
            "--output-dir",
            str(symbols_root_dir),
            *metadata_args,
        ],
        label="symbol docs generation",
    )
    return metadata


def generate_xref_metadata(
    metadata_dir: Path, *, gamefile_dir: Path
) -> list[XRefMetadataOutput]:
    metadata: list[XRefMetadataOutput] = []
    require_gamefiles(gamefile_dir)
    metadata_dir.mkdir(parents=True, exist_ok=True)

    for gamefile in GAMEFILE_BUILDS:
        gamefile_path = gamefile.path_in(gamefile_dir)

        for job in XREF_METADATA_JOBS:
            output_path = job.output_path(metadata_dir, gamefile)
            run_checked(
                [
                    sys.executable,
                    str(job.script_path),
                    "--blueprint",
                    str(PCDOGS_BLUEPRINT),
                    "--gamefile",
                    str(gamefile_path),
                    "--build",
                    gamefile.build,
                    "--output",
                    str(output_path),
                ],
                label=f"{job.output_stem} metadata generation for {gamefile.build}",
            )
            metadata.append(XRefMetadataOutput(job.arg_name, output_path))

    return metadata


def relative_to_output(path: Path, output_dir: Path) -> str:
    try:
        return path.relative_to(output_dir).as_posix()
    except ValueError:
        return path.as_posix()


def copy_docs_config(config_path: Path, output_path: Path) -> None:
    output_path.write_text(config_path.read_text())


def write_docs_manifest(
    *,
    output_dir: Path,
    source_dir: Path,
    config_path: Path,
    metadata_paths: list[Path],
) -> Path:
    manifest = DocsManifest(
        schema_version=1,
        source_dir=source_dir.as_posix(),
        config_path=config_path.as_posix(),
        symbols=SymbolReferenceManifest(
            symbol_wrappers=SymbolWrapperManifest(
                base_path=SYMBOL_REFERENCE_BASE_PATH,
                overview=SymbolOverviewManifest(
                    title=SYMBOL_REFERENCE_TITLE,
                    path=SYMBOL_REFERENCE_OVERVIEW_PATH,
                ),
            )
        ),
        metadata=[relative_to_output(path, output_dir) for path in metadata_paths],
    )
    manifest_path = output_dir / "docs-manifest.json"
    manifest_path.write_text(json.dumps(asdict(manifest), indent=2) + "\n")
    return manifest_path


def main() -> int:
    args = parse_args()
    source_dir = resolved_path(args.source_dir)
    config_path = resolved_path(args.config)

    try:
        validate_symbol_nav_entry(config_path)
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
    metadata_dir = output_dir / "metadata" / "symbols"
    if output_dir.exists():
        shutil.rmtree(output_dir)

    pages_out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source_dir, pages_out)

    symbols_root_dir = pages_out / "modding-sdk" / "symbols"
    try:
        metadata = prepare_symbol_reference_docs(
            symbols_root_dir,
            metadata_dir=metadata_dir,
            gamefile_dir=args.gamefile_dir,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        print(exc, file=sys.stderr)
        return 1

    copy_docs_config(
        config_path,
        output_dir / "zensical.toml",
    )
    write_docs_manifest(
        output_dir=output_dir,
        source_dir=source_dir,
        config_path=config_path,
        metadata_paths=[item.path for item in metadata],
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
