#!/usr/bin/env bash
set -euo pipefail

page='docs/pages/index.md'
project='dogstuff/detours-to-the-rescue'
project_api='dogstuff%2Fdetours-to-the-rescue'
release="https://gitlab.com/$project/-/releases/permalink/latest"
downloads="$release/downloads"
api="https://gitlab.com/api/v4/projects/$project_api/releases/permalink/latest"
page_tmp="${page}.tmp"

tag="$(
  curl --fail --silent --show-error --location "$api" \
    | sed -n 's/.*"tag_name":"\([^"]*\)".*/\1/p'
)" || exit 0

[[ -n "$tag" ]] || exit 0

url="$downloads/dttr-$tag-release.zip"
sed -E "s|$release(/downloads/dttr-[^)]*-release\\.zip)?|$url|" "$page" > "$page_tmp"
mv "$page_tmp" "$page"

echo "Using latest normal release: $url"
