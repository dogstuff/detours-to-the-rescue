#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 4 ]; then
  echo "Usage: $0 <root> <podman> <image> <shader_output_dir>" >&2
  exit 1
fi

root=$1
podman=$2
image=$3
shader_output_dir=$4

source "$root/scripts/container-workspace.sh"

dttr_prepare_container_workspace "$root" "dttr-shaders" \
  --exclude='./.git' \
  --exclude='./build' \
  --exclude='./build-container' \
  --exclude='./.toolchain'

"$podman" run --rm \
  -e "SHADER_OUTPUT_DIR=$shader_output_dir" \
  -v "$DTTR_CONTAINER_WORKSPACE:/work" \
  -w /work \
  "$image" bash -lc '
    set -euo pipefail
    nix --extra-experimental-features "nix-command flakes" \
      shell .#shader-tools \
      -c bash -lc '\''
        set -euo pipefail
        bash ./modules/sidecar/scripts/build-shaders.sh "$SHADER_OUTPUT_DIR"
        bash ./modules/sidecar/scripts/build-opengl-shaders.sh "$SHADER_OUTPUT_DIR"
      '\''
  '

dttr_copy_staged_output_back "$root" "$shader_output_dir"
