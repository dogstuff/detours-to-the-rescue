#!/usr/bin/env python3
"""Smoke-test generated PCDOGS symbol documentation."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]

SECTION_HEADING = re.compile(
    r"^## (?P<section>.+?) \{ \.pcdogs-section-heading \}$", re.MULTILINE
)
SYMBOL_HEADING = re.compile(r"^### `[^`]+` \{ #[a-z0-9_-]+ \}$", re.MULTILINE)
MARKDOWN_LINK = re.compile(r"\[[^\]]+\]\((?P<href>[^)]+)\)")
HTML_LINK = re.compile(r'href="(?P<href>[^"]+)"')


class Checks:
    def __init__(self) -> None:
        self.failures: list[str] = []

    def require(self, condition: bool, message: str) -> None:
        if not condition:
            self.failures.append(message)

    def require_file(self, path: Path, message_prefix: str) -> None:
        self.require(path.exists(), f"{message_prefix}: {path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generator", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def generate_docs(generator: Path, output_dir: Path) -> None:
    if output_dir.exists():
        shutil.rmtree(output_dir)
    subprocess.run(
        [sys.executable, str(generator), "--output-dir", str(output_dir)],
        check=True,
    )


def generated_markdown(output_dir: Path, checks: Checks) -> list[Path]:
    docs_root = output_dir / "pcdogs"
    checks.require_file(docs_root / "index.md", "missing generated index")
    checks.require_file(docs_root / "actor.md", "missing representative category page")

    for stale_path in (
        output_dir / "pcdogs-stable",
        output_dir / "pcdogs-unstable",
        output_dir / "pcdogs-stable-symbols.md",
        output_dir / "pcdogs-unstable-symbols.md",
    ):
        checks.require(
            not stale_path.exists(), f"stale split docs were generated: {stale_path}"
        )

    markdown_files = sorted(docs_root.glob("*.md"))
    checks.require(
        len(markdown_files) > 10, "expected many category pages to be generated"
    )
    return markdown_files


def check_page_shape(markdown_files: list[Path], checks: Checks) -> str:
    sections: set[str] = set()
    all_markdown: list[str] = []

    for path in markdown_files:
        text = path.read_text()
        all_markdown.append(text)

        checks.require(text.strip(), f"{path}: generated page is empty")
        checks.require(
            text.lstrip().startswith("# "),
            f"{path}: generated page lacks a top-level heading",
        )

        if path.name == "index.md":
            continue

        sections.update(
            match.group("section") for match in SECTION_HEADING.finditer(text)
        )
        checks.require(
            SYMBOL_HEADING.search(text) is not None,
            f"{path}: no symbol entries rendered",
        )
        checks.require(
            "[← Back to PCDOGS categories](index.md)" in text,
            f"{path}: missing category index link",
        )

    checks.require(
        {"Functions", "Data", "Types"}.issubset(sections),
        "generated docs must cover functions, data, and types",
    )
    return "\n".join(all_markdown)


def check_generated_features(all_markdown: str, checks: Checks) -> None:
    for marker in (
        '=== "C SDK Call"',
        '=== "C SDK Hook"',
        '=== "Read"',
        '=== "Write"',
        "Resolver Reference",
        "See Also",
        '<details class="pcdogs-symbol-builds',
    ):
        checks.require(
            marker in all_markdown,
            f"generated docs never rendered expected feature: {marker}",
        )

    checks.require(
        "Not yet documented." in all_markdown,
        "undocumented symbols must keep a standard fallback",
    )
    checks.require(
        "None" not in all_markdown,
        "generated docs should not expose Python None placeholders",
    )


def check_index_links(markdown_files: list[Path], checks: Checks) -> None:
    docs_root = markdown_files[0].parent
    index_text = (docs_root / "index.md").read_text()
    for category_page in markdown_files:
        if category_page.name == "index.md":
            continue
        checks.require(
            f"({category_page.name})" in index_text
            or f'href="{category_page.name}"' in index_text,
            f"index does not link generated category page: {category_page.name}",
        )


def normalize_doc_link(source: Path, href: str, generated_root: Path) -> Path | None:
    href = href.split("#", 1)[0]
    if not href or href.startswith(("http://", "https://", "mailto:")):
        return None
    if href.startswith("../"):
        slug = href.removeprefix("../").strip("/")
        if slug and "/" not in slug and not Path(slug).suffix:
            return (generated_root / f"{slug}.md").resolve()
    return (source.parent / href).resolve()


def check_internal_links(markdown_files: list[Path], checks: Checks) -> None:
    repo_docs = (REPO_ROOT / "docs/pages").resolve()
    generated_root = markdown_files[0].parent.resolve()

    for source in markdown_files:
        text = source.read_text()
        for match in (*MARKDOWN_LINK.finditer(text), *HTML_LINK.finditer(text)):
            target = normalize_doc_link(source, match.group("href"), generated_root)
            if target is None:
                continue
            if generated_root in target.parents or target.parent == generated_root:
                checks.require(
                    target.exists(),
                    f"{source}: broken generated link {match.group('href')}",
                )
            elif repo_docs in target.parents or target == repo_docs:
                checks.require(
                    target.exists(), f"{source}: broken docs link {match.group('href')}"
                )


def main() -> int:
    args = parse_args()
    checks = Checks()

    generate_docs(args.generator, args.output_dir)
    markdown_files = generated_markdown(args.output_dir, checks)
    if checks.failures:
        print("\n".join(checks.failures), file=sys.stderr)
        return 1

    all_markdown = check_page_shape(markdown_files, checks)
    check_generated_features(all_markdown, checks)
    check_index_links(markdown_files, checks)
    check_internal_links(markdown_files, checks)

    if checks.failures:
        print("\n".join(checks.failures), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
