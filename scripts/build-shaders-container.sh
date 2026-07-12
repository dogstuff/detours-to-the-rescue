#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 4 ]; then
  echo "Usage: $0 <root> <docker> <image> <platform>" >&2
  exit 1
fi

root=$1
docker=$2
image=$3
platform=$4

source "$(dirname "$0")/container-workspace.sh"

stage_excludes=(
  --exclude='./.cache'
  --exclude='./.direnv'
  --exclude='./.git'
  --exclude='./.worktrees'
  --exclude='./build*'
  --exclude='./docs/build'
  --exclude='./gamefiles'
  --exclude='./.toolchain'
  --exclude='./modules/sidecar/shaders/cache'
)

dttr_prepare_container_workspace "$root" "dttr-shader-container" "${stage_excludes[@]}"

"$docker" run --rm \
  --platform "$platform" \
  -v "$DTTR_CONTAINER_WORKSPACE:/workspace" \
  -w /workspace \
  "$image" sh -lc 'bash ./scripts/ci-build-shaders.sh'

dttr_copy_staged_output_back "$root" "modules/sidecar/shaders/cache/sdl3gpu"
