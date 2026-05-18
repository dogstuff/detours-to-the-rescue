#!/usr/bin/env bash
set -euo pipefail

tracked_patterns=(
  '^modules/sdk/include/dttr_pcdogs\.h$'
  '^modules/sdk/include/dttr_pcdogs_unstable\.h$'
  '^modules/sdk/include/dttr_sdk\.h$'
  '^modules/sdk/src/pcdogs\.c$'
  '^modules/sdk/src/generated/'
  '^modules/loader/include/gen/'
  '^modules/sidecar/include/gen/'
)

tracked_generated="$({ git ls-files || true; } | grep -E "$(IFS='|'; echo "${tracked_patterns[*]}")" || true)"
if [ -n "$tracked_generated" ]; then
  echo "First-party generated artifacts are still tracked:" >&2
  echo "$tracked_generated" >&2
  exit 1
fi

source_paths=(
  modules/sdk/include/dttr_pcdogs.h
  modules/sdk/include/dttr_pcdogs_unstable.h
  modules/sdk/include/dttr_sdk.h
  modules/sdk/src/pcdogs.c
  modules/sdk/src/generated
  modules/loader/include/gen
  modules/sidecar/include/gen
)

present=()
for path in "${source_paths[@]}"; do
  if [ -e "$path" ]; then
    present+=("$path")
  fi
done

if [ "${#present[@]}" -gt 0 ]; then
  echo "First-party generated artifacts still exist in the source tree:" >&2
  printf '  %s\n' "${present[@]}" >&2
  exit 1
fi
