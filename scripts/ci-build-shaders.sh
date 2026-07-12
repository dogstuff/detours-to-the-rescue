#!/usr/bin/env bash
set -euo pipefail

cache_dir="${1:-modules/sidecar/shaders/cache/sdl3gpu}"

rm -rf "$cache_dir"
bash ./modules/sidecar/scripts/build-shaders.sh "$cache_dir"

expected=(
  sdl3gpu_shaders.h
  shaders/basic.frag.dxbc
  shaders/basic.frag.dxil
  shaders/basic.frag.spv
  shaders/basic.vert.dxbc
  shaders/basic.vert.dxil
  shaders/basic.vert.spv
)

for artifact in "${expected[@]}"; do
  test -s "$cache_dir/$artifact"
done
