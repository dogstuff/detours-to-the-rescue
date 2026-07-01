#!/usr/bin/env bash
set -euo pipefail

fixtures_archive=${PCDOGS_FIXTURES_ARCHIVE:-gamefiles.tar.gz}
fixtures_dir=${PCDOGS_FIXTURES_DIR:-gamefiles}
secure_dir=.secure_files

case "$fixtures_dir" in
  "" | /* | *..*)
    echo "Unsafe PCDOGS_FIXTURES_DIR: $fixtures_dir" >&2
    exit 1
    ;;
esac

rm -rf "$secure_dir" "$fixtures_dir"
mkdir -p "$secure_dir" "$fixtures_dir"

glab auth login \
  --job-token "$CI_JOB_TOKEN" \
  --hostname "${CI_SERVER_FQDN:-gitlab.com}" \
  --api-protocol "${CI_SERVER_PROTOCOL:-https}"
glab -R "$CI_PROJECT_PATH" securefile download --all --output-dir="$secure_dir"

tar -xzf "$secure_dir/$fixtures_archive" -C "$fixtures_dir" --strip-components=1

test -n "$(find "$fixtures_dir" -type f -print -quit)" || {
  echo "No PCDOGS fixtures found in $fixtures_dir." >&2
  exit 1
}
