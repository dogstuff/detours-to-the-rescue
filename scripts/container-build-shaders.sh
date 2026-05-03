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
workspace=$root
copy_back=0

if [ ! -d "$root" ]; then
  echo "Missing repository root: $root" >&2
  exit 1
fi

# Podman volume mounts are fragile with spaces on some hosts (notably podman-machine
# on macOS), so stage through a temporary workspace when needed and copy the generated
# headers back to the real checkout.
if [[ $root == *[[:space:]]* ]]; then
  tmp=$(mktemp -d "${TMPDIR:-/tmp}/dttr-shaders.XXXXXX")
  trap 'rm -rf "$tmp"' EXIT
  workspace="$tmp/workspace"
  mkdir -p "$workspace"
  copy_back=1

  tar --exclude='./.git' \
    --exclude='./build' \
    --exclude='./build-container' \
    --exclude='./.toolchain' \
    --exclude='./.omx' \
    -C "$root" -cf - . \
    | tar -C "$workspace" -xf -
fi

"$podman" run --rm \
  -e "SHADER_OUTPUT_DIR=$shader_output_dir" \
  -v "$workspace:/work" \
  -w /work \
  "$image" bash -lc '
    set -euo pipefail
    nix --extra-experimental-features "nix-command flakes" \
      shell .#shader-tools \
      -c bash -lc '\''
        set -euo pipefail
        bash ./scripts/build-shaders.sh "$SHADER_OUTPUT_DIR"
        bash ./scripts/build-opengl-shaders.sh "$SHADER_OUTPUT_DIR"
      '\''
  '

if [ "$copy_back" -eq 1 ]; then
  rm -rf "$root/$shader_output_dir"
  mkdir -p "$(dirname "$root/$shader_output_dir")"
  cp -R "$workspace/$shader_output_dir" "$root/$shader_output_dir"
fi
