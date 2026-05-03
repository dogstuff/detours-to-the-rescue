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
