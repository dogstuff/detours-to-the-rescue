#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 3 ] || [ "$#" -gt 5 ]; then
  echo "Usage: $0 <version> <archive_id> <debug|release> [package_registry_url] [job_token]" >&2
  exit 1
fi

version=$1
archive_id=$2
mode=$3
package_registry_url=${4:-}
job_token=${5:-}
build_dir=${BUILD_DIR:-build}

if [[ ! $archive_id =~ ^[A-Za-z0-9._-]+$ ]]; then
  echo "Invalid archive_id: $archive_id" >&2
  exit 1
fi

package_build() {
  local archive=$1
  local dist_name=$2

  local archive_path
  archive_path="${PWD}/${archive}"

  rm -f "$archive_path"
  (cd "${build_dir}/dist" && zip -r "$archive_path" "${dist_name}/")
}

upload_artifacts() {
  if [ -z "$package_registry_url" ] || [ -z "$job_token" ]; then
    return
  fi

  scripts/upload-package-artifacts.sh "$package_registry_url" "$job_token" "$@"
}

build_and_package() {
  local recipe=$1
  local modding=$2
  local archive=$3
  local dist_name=$4

  DTTR_VERSION="$version" DTTR_MODS_ENABLED="$modding" just "$recipe"
  package_build "$archive" "$dist_name"
}

case "$mode" in
  debug)
    package_matrix=(
      "build OFF dttr debug debug"
      "build ON dttr-modding debug-modding debug"
    )
    ;;
  release)
    package_matrix=(
      "build OFF dttr debug debug"
      "build-release OFF dttr release release"
      "build ON dttr-modding debug-modding debug"
      "build-release ON dttr-modding release-modding release"
    )
    ;;
  *)
    echo "Unknown package mode: $mode" >&2
    exit 1
    ;;
esac


archives=()
for package_entry in "${package_matrix[@]}"; do
  read -r recipe modding archive_prefix dist_name archive_kind <<<"$package_entry"
  archive="${archive_prefix}-${archive_id}-${archive_kind}.zip"
  build_and_package \
    "$recipe" \
    "$modding" \
    "$archive" \
    "$dist_name"
  archives+=("$archive")
done

if [ "$mode" = "release" ]; then
  template_archive="dttr-mod-template-c.zip"
  scripts/package-mod-template-c.sh "$version" "$template_archive"
  archives+=("$template_archive")
fi

upload_artifacts "${archives[@]}"
