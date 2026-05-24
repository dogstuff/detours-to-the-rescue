#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "Usage: $0 <package_registry_url> <job_token> <archive>..." >&2
  exit 1
fi

package_registry_url=$1
job_token=$2
shift 2

if [ -z "$package_registry_url" ] || [ -z "$job_token" ]; then
  echo "Missing package registry URL or job token." >&2
  exit 1
fi

for archive in "$@"; do
  if [ ! -f "$archive" ]; then
    echo "Missing package archive: $archive" >&2
    exit 1
  fi

  curl --fail \
    --header "JOB-TOKEN: ${job_token}" \
    --upload-file "$archive" \
    "${package_registry_url}/${archive}"
done
