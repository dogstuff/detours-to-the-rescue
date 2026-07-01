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

  if [[ $root != *[[:space:]]* ]]; then
    return
  fi

  # Container mounts can choke on whitespace paths; stage those through tmp.
  DTTR_CONTAINER_STAGE_TMP=$(mktemp -d "${TMPDIR:-/tmp}/${prefix}.XXXXXX")
  trap 'rm -rf "$DTTR_CONTAINER_STAGE_TMP"' EXIT
  DTTR_CONTAINER_WORKSPACE="$DTTR_CONTAINER_STAGE_TMP/workspace"
  DTTR_CONTAINER_STAGED=1
  mkdir -p "$DTTR_CONTAINER_WORKSPACE"

  tar "$@" -C "$root" -cf - . \
    | tar -C "$DTTR_CONTAINER_WORKSPACE" -xf -
}

dttr_copy_staged_output_back() {
  local root=$1
  local output_path=$2
  local host_output
  local staged_output

  if [ "${DTTR_CONTAINER_STAGED:-0}" -ne 1 ]; then
    return
  fi

  case "$output_path" in
    "" | /* | *".."*)
      echo "Unsafe staged output path: $output_path" >&2
      exit 1
      ;;
  esac

  host_output=$root/$output_path
  staged_output=$DTTR_CONTAINER_WORKSPACE/$output_path

  if [ ! -d "$staged_output" ]; then
    echo "Missing staged output path: $output_path" >&2
    exit 1
  fi

  rm -rf "$host_output"
  mkdir -p "$(dirname "$host_output")"
  cp -R "$staged_output" "$host_output"
}
