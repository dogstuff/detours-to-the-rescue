#!/usr/bin/env python3
"""Prepare a build-tree Zensical source tree with SDK symbol reference pages."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import TYPE_CHECKING, Protocol

if TYPE_CHECKING:

    class SymbolPage(Protocol):
        name: object
        anchor: object

    class SymbolCategory(Protocol):
        display: object


REPO_ROOT = Path(__file__).resolve().parents[1]
SDK_SCRIPT_DIR = REPO_ROOT / "modules/sdk/scripts"
PCDOGS_BLUEPRINT = REPO_ROOT / "modules/sdk/blueprints/dttr_pcdogs.py"
SYMBOL_REFERENCE_BASE_PATH = "modding-sdk/symbols/pcdogs"
SYMBOL_NAV_STUB = '    { "PCDogs Symbols" = "modding-sdk/symbols/pcdogs/index.md" },'


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
class SymbolMetadataInputs:
    symbol_addresses: list[Path]
    function_call_xrefs: list[Path]

    @classmethod
    def from_outputs(cls, metadata: list[XRefMetadataOutput]) -> SymbolMetadataInputs:
        return cls(
            symbol_addresses=[
                item.path for item in metadata if item.arg_name == "--symbol-addresses"
            ],
            function_call_xrefs=[
                item.path
                for item in metadata
                if item.arg_name == "--function-call-xrefs"
            ],
        )


@dataclass(frozen=True, slots=True)
class NavPage:
    title: str
    path: str


@dataclass(frozen=True, slots=True)
class NavGroup:
    title: str
    entries: tuple[NavPage | NavGroup, ...]


@dataclass(frozen=True, slots=True)
class OverviewManifest:
    title: str
    path: str


@dataclass(frozen=True, slots=True)
class SymbolWrapperManifest:
    base_path: str
    overview: OverviewManifest
    detail_count: int


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


def validate_nav_stub(config_path: Path) -> None:
    config_text = config_path.read_text()
    if SYMBOL_NAV_STUB not in config_text:
        raise ValueError(f"could not find the PCDogs symbol nav entry in {config_path}")


def validate_output_dir(output_dir: Path, *, source_dir: Path, config_path: Path) -> Path:
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
            raise ValueError(f"refusing output dir {output_dir}: overlaps {label} {path}")

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
    symbol_docs_dir: Path, *, metadata_dir: Path, gamefile_dir: Path
) -> list[XRefMetadataOutput]:
    if symbol_docs_dir.exists():
        shutil.rmtree(symbol_docs_dir)

    metadata = generate_xref_metadata(metadata_dir, gamefile_dir=gamefile_dir)
    metadata_args = [
        value for item in metadata for value in (item.arg_name, str(item.path))
    ]
    run_checked(
        [
            sys.executable,
            str(SDK_SCRIPT_DIR / "generate_symbol_docs.py"),
            "--output-dir",
            str(symbol_docs_dir),
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


def ensure_sdk_script_dir_on_path() -> None:
    script_dir = str(SDK_SCRIPT_DIR)
    if script_dir not in sys.path:
        sys.path.insert(0, script_dir)


def toml_nav_lines(entry: NavPage | NavGroup, indent: int) -> list[str]:
    prefix = " " * indent
    if isinstance(entry, NavPage):
        return [f"{prefix}{{ {json.dumps(entry.title)} = {json.dumps(entry.path)} }},"]

    lines = [f"{prefix}{{ {json.dumps(entry.title)} = ["]
    for child in entry.entries:
        lines.extend(toml_nav_lines(child, indent + 2))
    lines.append(f"{prefix}] }},")
    return lines


def symbol_reference_path(path: str) -> str:
    return f"modding-sdk/symbols/{path}"


def blueprint_symbol_categories(
    metadata: list[XRefMetadataOutput],
) -> list[SymbolCategory]:
    ensure_sdk_script_dir_on_path()
    from generate_headers import load_blueprint, split_row_unstable_rows
    from generate_symbol_docs import symbol_doc_categories

    blueprint = load_blueprint(PCDOGS_BLUEPRINT)
    stable, unstable = split_row_unstable_rows(blueprint)
    inputs = SymbolMetadataInputs.from_outputs(metadata)
    return symbol_doc_categories(
        stable,
        unstable,
        function_call_xrefs=inputs.function_call_xrefs,
        symbol_addresses=inputs.symbol_addresses,
    )


def symbol_page_title(
    kind: str,
    page: SymbolPage,
    duplicate_names: Counter[str],
) -> str:
    title = str(page.name)
    return f"{title} ({kind})" if duplicate_names[title] > 1 else title


def symbol_nav_group(metadata: list[XRefMetadataOutput]) -> NavGroup:
    ensure_sdk_script_dir_on_path()
    from generate_symbol_docs import (
        iter_category_cards,
        symbol_detail_output_path,
        symbol_overview_output_path,
    )

    category_entries: list[NavPage | NavGroup] = [
        NavPage("Overview", symbol_reference_path(symbol_overview_output_path()))
    ]

    for category in blueprint_symbol_categories(metadata):
        symbol_pages = sorted(
            iter_category_cards(category),
            key=lambda item: (
                str(item[1].name).casefold(),
                str(item[0]),
                str(item[1].anchor),
            ),
        )
        duplicate_names = Counter(str(page.name) for _kind, page in symbol_pages)
        pages = tuple(
            NavPage(
                symbol_page_title(kind, page, duplicate_names),
                symbol_reference_path(symbol_detail_output_path(page)),
            )
            for kind, page in symbol_pages
        )
        if pages:
            category_entries.append(NavGroup(str(category.display), pages))

    return NavGroup("PCDogs Symbols", tuple(category_entries))


def symbol_nav_entry(metadata: list[XRefMetadataOutput]) -> str:
    return "\n".join(
        toml_nav_lines(
            symbol_nav_group(metadata),
            4,
        )
    )


def write_prepared_config(
    config_path: Path, output_path: Path, metadata: list[XRefMetadataOutput]
) -> None:
    config_text = config_path.read_text()
    replacement = symbol_nav_entry(metadata)
    rewritten = config_text.replace(SYMBOL_NAV_STUB, replacement, 1)
    output_path.write_text(rewritten)


def write_docs_manifest(
    *,
    output_dir: Path,
    source_dir: Path,
    config_path: Path,
    symbol_docs_dir: Path,
    metadata_paths: list[Path],
) -> Path:
    symbols_dir = symbol_docs_dir / "pcdogs" / "symbols"
    manifest = DocsManifest(
        schema_version=1,
        source_dir=source_dir.as_posix(),
        config_path=config_path.as_posix(),
        symbols=SymbolReferenceManifest(
            symbol_wrappers=SymbolWrapperManifest(
                base_path=SYMBOL_REFERENCE_BASE_PATH,
                overview=OverviewManifest(
                    title="PCDogs Symbols",
                    path=f"{SYMBOL_REFERENCE_BASE_PATH}/index.md",
                ),
                detail_count=len(list(symbols_dir.glob("*.md"))),
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
        validate_nav_stub(config_path)
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

    symbol_docs_dir = pages_out / "modding-sdk" / "symbols"
    try:
        metadata = prepare_symbol_reference_docs(
            symbol_docs_dir,
            metadata_dir=metadata_dir,
            gamefile_dir=args.gamefile_dir,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        print(exc, file=sys.stderr)
        return 1

    write_prepared_config(
        config_path,
        output_dir / "zensical.toml",
        metadata,
    )
    write_docs_manifest(
        output_dir=output_dir,
        source_dir=source_dir,
        config_path=config_path,
        symbol_docs_dir=symbol_docs_dir,
        metadata_paths=[item.path for item in metadata],
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
