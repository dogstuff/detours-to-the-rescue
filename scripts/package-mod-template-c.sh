#!/usr/bin/env bash
set -euo pipefail

version=$1
archive=${2:-dttr-mod-template-c.zip}
template_dir=examples/mod-template-c
package_dir=dttr-mod-template-c

if [[ ! $version =~ ^[A-Za-z0-9._-]+$ ]]; then
  echo "Invalid version: $version" >&2
  exit 1
fi

if [[ $archive == *$'\n'* || $archive == *$'\r'* ]]; then
  echo "Invalid archive path" >&2
  exit 1
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
stage_dir=$tmp/$package_dir

mkdir -p "$stage_dir"

git ls-files -z -- "$template_dir" | while IFS= read -r -d '' path; do
  rel=${path#"$template_dir"/}
  mkdir -p "$stage_dir/$(dirname "$rel")"
  cp -p "$path" "$stage_dir/$rel"
done

printf '%s
' "$version" > "$stage_dir/dttr-version.txt"
archive_path=$archive
if [[ $archive != /* ]]; then
  archive_path=$PWD/$archive
fi

rm -f "$archive_path"
(cd "$tmp" && zip -qr "$archive_path" "$package_dir")
