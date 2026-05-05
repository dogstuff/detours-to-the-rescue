#!/usr/bin/env bash
set -euo pipefail

page='docs/pages/index.md'
project='dogstuff/detours-to-the-rescue'
release="https://gitlab.com/$project/-/releases/permalink/latest"
dynamic_url="$release/downloads/dttr-release.zip"
page_tmp="${page}.tmp"

sed -E "s|$release/downloads/dttr-v[^)]*-release\.zip|$dynamic_url|" "$page" > "$page_tmp"
mv "$page_tmp" "$page"

echo "Using dynamic latest normal release: $dynamic_url"
