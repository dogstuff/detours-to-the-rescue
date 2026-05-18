#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 9 ]; then
  echo "Usage: $0 <root> <docker> <image> <platform> <toolchain_dir> <build_dir> <version> <short_sha> <modding>" >&2
  exit 1
fi

root=$1
docker=$2
image=$3
platform=$4
toolchain_dir=$5
build_dir=$6
version=$7
short_sha=$8
modding=$9

source "$(dirname "$0")/container-workspace.sh"

dttr_prepare_container_workspace "$root" "dttr-container" \
  --exclude='./.git' \
  --exclude='./build*' \
  --exclude='./.toolchain'

"$docker" run --rm \
  --platform "$platform" \
  -e "DTTR_TOOLCHAIN_DIR=$toolchain_dir" \
  -e "BUILD_DIR=$build_dir" \
  -e "DTTR_VERSION=$version" \
  -e "GIT_SHORT_SHA=$short_sha" \
  -e "DTTR_MODS_ENABLED=$modding" \
  -v "$DTTR_CONTAINER_WORKSPACE:/workspace" \
  -w /workspace \
  "$image" sh -lc 'rm -rf "$BUILD_DIR" && just build'

dttr_copy_staged_output_back "$root" "$build_dir"
