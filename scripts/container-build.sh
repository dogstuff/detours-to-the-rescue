#!/usr/bin/env bash
set -euo pipefail

[ "$#" -eq 8 ] || { echo "Usage: $0 <root> <docker> <image> <toolchain_dir> <build_dir> <version> <short_sha> <modding>" >&2; exit 1; }

root=$1
docker=$2
image=$3
toolchain_dir=$4
build_dir=$5
version=$6
short_sha=$7
modding=$8
workspace=$root

if [[ $root == *[[:space:]]* ]]; then
  tmp=$(mktemp -d "$HOME/dttr-container.XXXXXX")
  trap 'rm -rf "$tmp"' EXIT
  workspace="$tmp/workspace"
  mkdir -p "$workspace"
  tar --exclude='./.git' \
    --exclude='./build' \
    --exclude='./build-container' \
    --exclude='./.toolchain' \
    -C "$root" -cf - . \
    | tar -C "$workspace" -xf -
fi

"$docker" run --rm -it \
  -e "DTTR_TOOLCHAIN_DIR=$toolchain_dir" \
  -v "$workspace:/workspace" \
  -w /workspace \
  "$image" sh -lc \
  "rm -rf '$build_dir' && task build 'BUILD_DIR=$build_dir' 'DTTR_VERSION=$version' 'GIT_SHORT_SHA=$short_sha' DTTR_MODDING=$modding"

if [ "$workspace" != "$root" ]; then
  rm -rf "$root/$build_dir"
  cp -R "$workspace/$build_dir" "$root/$build_dir"
fi
