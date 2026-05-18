#!/usr/bin/env bash
set -euo pipefail

archive=${DTTR_GAMEFILES_SECURE_FILE:-gamefiles.tar.gz}
fixture_dir=${DTTR_PCDOGS_FIXTURE_DIR:-gamefiles}
secure_dir=.secure_files

case "$fixture_dir" in
  "" | /* | *..*)
    echo "Unsafe DTTR_PCDOGS_FIXTURE_DIR: $fixture_dir" >&2
    exit 1
    ;;
esac

rm -rf "$secure_dir" "$fixture_dir"
mkdir -p "$secure_dir" "$fixture_dir"

glab auth login \
  --job-token "$CI_JOB_TOKEN" \
  --hostname "${CI_SERVER_FQDN:-gitlab.com}" \
  --api-protocol "${CI_SERVER_PROTOCOL:-https}"
glab -R "$CI_PROJECT_PATH" securefile download --all --output-dir="$secure_dir"

tar -xzf "$secure_dir/$archive" -C "$fixture_dir" --strip-components=1

test -n "$(find "$fixture_dir" -type f -print -quit)" || {
  echo "No gamefiles found in $fixture_dir." >&2
  exit 1
}
