#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:?build dir required}"
out_dir="${2:?output dir required}"

stage_runtime_file() {
  local src_rel dst_name

  src_rel="$1"
  dst_name="$2"

  cp -f "$build_dir/$src_rel" "$out_dir/$dst_name"
}

mkdir -p "$out_dir"
rm -f "$out_dir"/*.exe "$out_dir"/*.dll

stage_runtime_file "loader/dttr.exe" "dttr.exe"
stage_runtime_file "sidecar/libdttr_sidecar.dll" "libdttr_sidecar.dll"
stage_runtime_file "sidecar/SDL3.dll" "SDL3.dll"

printf 'INFO: Staged runtime layout in %s\n' "$out_dir"
ls -1 "$out_dir" | sed 's/^/  - /'
