#!/usr/bin/env python3
import os
from pathlib import Path

PAGE = Path("docs/pages/index.md")
RELEASES = "https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases"
START = "<!-- docs-download:start -->"
END = "<!-- docs-download:end -->"


def download_block(tag: str) -> str:
    if not tag:
        return ""

    return (
        f"[Vanilla · {tag}]({RELEASES}/{tag}/downloads/dttr-release.zip)"
        "{ .md-button .md-button--primary }\n"
        f"[Modding Enabled · {tag}]({RELEASES}/{tag}/downloads/dttr-modding-release.zip)"
        "{ .md-button }\n"
    )


def render_page(text: str, tag: str) -> str:
    if START not in text or END not in text:
        raise SystemExit(f"{PAGE} is missing docs download markers")

    before, rest = text.split(START, 1)
    _, after = rest.split(END, 1)
    return f"{before}{START}\n{download_block(tag)}{END}{after}"


PAGE.write_text(render_page(PAGE.read_text(), os.environ.get("CI_COMMIT_TAG", "")))
