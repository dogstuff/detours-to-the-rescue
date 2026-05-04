#!/usr/bin/env bash
set -euo pipefail

page=docs/pages/index.md
fallback='https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest'
downloads='https://gitlab.com/dogstuff/detours-to-the-rescue/-/releases/permalink/latest/downloads'
api='https://gitlab.com/api/v4/projects/dogstuff%2Fdetours-to-the-rescue/releases/permalink/latest'

tag=$(curl --fail --silent --show-error --location "$api" \
  | sed -n 's/.*"tag_name":"\([^"]*\)".*/\1/p') || exit 0

[[ -n "$tag" ]] || exit 0

url="$downloads/dttr-$tag-release.zip"
sed "s|$fallback|$url|" "$page" > "$page.tmp"
mv "$page.tmp" "$page"

echo "Using latest normal release: $url"
