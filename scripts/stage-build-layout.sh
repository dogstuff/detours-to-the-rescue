#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:?build dir required}"
out_dir="${2:?output dir required}"

mkdir -p "$out_dir"
rm -f "$out_dir"/*.exe "$out_dir"/*.dll

cp -f "$build_dir/loader/dttr.exe" "$out_dir/dttr.exe"
cp -f "$build_dir/sidecar/libdttr_sidecar.dll" "$out_dir/libdttr_sidecar.dll"
cp -f "$build_dir/sidecar/SDL3.dll" "$out_dir/SDL3.dll"

echo "INFO: Staged runtime layout in $out_dir"
ls -1 "$out_dir" | sed 's/^/  - /'
