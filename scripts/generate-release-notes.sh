#!/bin/sh
set -eu

range="${1:-}"

if [ -z "${range}" ]; then
  prev_tag="$(git describe --tags --abbrev=0 HEAD^ 2>/dev/null || true)"
  if [ -n "${prev_tag}" ]; then
    range="${prev_tag}..HEAD"
  else
    range="HEAD"
  fi
fi

case "${range}" in
  *...*)
    previous_ref="${range%%...*}"
    release_ref="${range#*...}"
    previous_ref="${previous_ref:-HEAD}"
    ;;
  *..*)
    previous_ref="${range%%..*}"
    release_ref="${range#*..}"
    previous_ref="${previous_ref:-HEAD}"
    ;;
  *)
    release_ref="${range}"
    previous_ref="$(git describe --tags --abbrev=0 "${release_ref}^" 2>/dev/null || true)"
    ;;
esac
release_ref="${release_ref:-HEAD}"

read_abi_range() {
  git show "${1}:modules/sdk/include/dttr_versions.h.in" 2>/dev/null |
    awk '
      $1 == "#define" && $3 ~ /^[0-9]+u?$/ {
        sub(/u$/, "", $3)
        if ($2 == "DTTR_SDK_MIN_COMPATIBLE_ABI_VERSION") minimum = $3
        if ($2 == "DTTR_SDK_ABI_VERSION") maximum = $3
      }
      END {
        if (minimum != "" && maximum != "") printf "%s - %s", minimum, maximum
      }
    '
}

abi_range="$(read_abi_range "${release_ref}")"

if [ -z "${abi_range}" ]; then
  echo "Cannot read supported SDK ABI version range at ${release_ref}." >&2
  exit 1
fi

printf 'Supported SDK ABI versions: `%s`\n\n' "${abi_range}"

if [ -n "${previous_ref}" ]; then
  previous_abi_range="$(read_abi_range "${previous_ref}")"

  if [ -n "${previous_abi_range}" ] && [ "${previous_abi_range}" != "${abi_range}" ]; then
    printf '> **WARNING: Supported SDK ABI versions changed: `%s` → `%s`.**\n' "${previous_abi_range}" "${abi_range}"
    printf '> Mods built for an ABI outside this range are incompatible. Check for updated mods or rebuild them against this release’s SDK.\n\n'
  fi
fi

has_log_entries() {
  test -n "$(git log -1 --format=%H "$@")"
}

print_log_entries() {
  git log --format='- %s (%h)' "$@"
}

if has_log_entries --first-parent --merges "${range}"; then
  echo '## Merge Requests / Pull Requests'
  print_log_entries --first-parent --merges "${range}"
  echo
fi

if has_log_entries --no-merges "${range}"; then
  echo '## Commits'
  print_log_entries --no-merges "${range}"
else
  echo 'No commits found for this release.'
fi
