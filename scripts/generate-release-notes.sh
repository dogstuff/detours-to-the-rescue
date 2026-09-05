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

read_abi_version() {
  git show "${1}:modules/sdk/include/dttr_versions.h.in" 2>/dev/null |
    sed -n 's/^#define[[:space:]]\{1,\}DTTR_SDK_ABI_VERSION[[:space:]]\{1,\}\([0-9]\{1,\}\)u\{0,1\}[[:space:]]*$/\1/p'
}

abi_version="$(read_abi_version "${release_ref}")"

if [ -z "${abi_version}" ]; then
  echo "Cannot read SDK ABI version at ${release_ref}." >&2
  exit 1
fi

printf 'SDK ABI version: `%s`\n\n' "${abi_version}"

if [ -n "${previous_ref}" ]; then
  previous_abi_version="$(read_abi_version "${previous_ref}")"

  if [ -n "${previous_abi_version}" ] && [ "${previous_abi_version}" != "${abi_version}" ]; then
    printf '> **WARNING: SDK ABI version changed: `%s` → `%s`.**\n' "${previous_abi_version}" "${abi_version}"
    printf '> Mods built for an older ABI may be incompatible. Check for updated mods or rebuild them against this release’s SDK.\n\n'
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
