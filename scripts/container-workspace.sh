#!/usr/bin/env bash

dttr_prepare_container_workspace() {
  local root=$1
  local prefix=$2
  shift 2

  if [ ! -d "$root" ]; then
    echo "Missing repository root: $root" >&2
    exit 1
  fi

  DTTR_CONTAINER_WORKSPACE=$root
  DTTR_CONTAINER_STAGED=0

  # Stage through /tmp when host volume mounts cannot handle spaces.
  if [[ $root == *[[:space:]]* ]]; then
    DTTR_CONTAINER_STAGE_TMP=$(mktemp -d "${TMPDIR:-/tmp}/${prefix}.XXXXXX")
    trap 'rm -rf "$DTTR_CONTAINER_STAGE_TMP"' EXIT
    DTTR_CONTAINER_WORKSPACE="$DTTR_CONTAINER_STAGE_TMP/workspace"
    DTTR_CONTAINER_STAGED=1
    mkdir -p "$DTTR_CONTAINER_WORKSPACE"

    tar "$@" -C "$root" -cf - . \
      | tar -C "$DTTR_CONTAINER_WORKSPACE" -xf -
  fi
}

dttr_copy_staged_output_back() {
  local root=$1
  local output_path=$2

  if [ "${DTTR_CONTAINER_STAGED:-0}" -ne 1 ]; then
    return
  fi

  case "$output_path" in
    "" | /* | *".."*)
      echo "Unsafe staged output path: $output_path" >&2
      exit 1
      ;;
  esac

  if [ ! -d "$DTTR_CONTAINER_WORKSPACE/$output_path" ]; then
    echo "Missing staged output path: $output_path" >&2
    exit 1
  fi

  rm -rf "$root/$output_path"
  mkdir -p "$(dirname "$root/$output_path")"
  cp -R "$DTTR_CONTAINER_WORKSPACE/$output_path" "$root/$output_path"
}
