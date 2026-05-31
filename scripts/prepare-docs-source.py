#!/usr/bin/env python3
"""Prepare a build-tree Zensical source tree with generated docs pages."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

SYMBOL_NAV_ENTRY = '{ "Symbol Wrappers" = "modding-sdk/generated/pcdogs/" },'


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Copy docs sources and add generated symbol wrappers pages.",
    )
    parser.add_argument("--source-dir", type=Path, default=Path("docs/pages"))
    parser.add_argument("--config", type=Path, default=Path("docs/zensical.toml"))
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def toml_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def page_title(path: Path) -> str:
    for line in path.read_text().splitlines():
        if line.startswith("# "):
            return line.removeprefix("# ").removesuffix(" Symbols").strip()
    return path.stem.replace("-", " ").title()


def category_nav_entry(title: str, docs_path: str, surface_dir: Path) -> str:
    category_pages = [
        path for path in surface_dir.glob("*.md") if path.name != "index.md"
    ]
    category_pages.sort(key=lambda path: page_title(path).casefold())

    lines = [f"    {{ {toml_string(title)} = ["]
    for page in category_pages:
        lines.append(
            "      "
            f"{{ {toml_string(page_title(page))} = "
            f"{toml_string(docs_path + '/' + page.name)} }},"
        )
    lines.append("    ] },")
    return "\n".join(lines)


def config_with_symbol_category_nav(config_text: str, symbol_docs_dir: Path) -> str:
    if SYMBOL_NAV_ENTRY not in config_text:
        raise RuntimeError(
            f"missing nav placeholder in Zensical config: {SYMBOL_NAV_ENTRY}"
        )
    return config_text.replace(
        SYMBOL_NAV_ENTRY,
        category_nav_entry(
            "Symbol Wrappers",
            "modding-sdk/generated/pcdogs",
            symbol_docs_dir / "pcdogs",
        ),
    )


def main() -> int:
    args = parse_args()
    output_dir = args.output_dir
    pages_out = output_dir / "pages"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    pages_out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(args.source_dir, pages_out)

    symbol_docs_dir = pages_out / "modding-sdk" / "generated"
    subprocess.run(
        [
            sys.executable,
            "modules/sdk/scripts/generate_symbol_docs.py",
            "--output-dir",
            str(symbol_docs_dir),
        ],
        check=True,
    )
    (output_dir / "zensical.toml").write_text(
        config_with_symbol_category_nav(args.config.read_text(), symbol_docs_dir)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
