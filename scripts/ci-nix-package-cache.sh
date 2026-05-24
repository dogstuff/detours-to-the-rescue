#!/usr/bin/env bash
set -euo pipefail

log() {
  printf 'ci-nix-package-cache: %s\n' "$*" >&2
}

hash_file() {
  local line

  if command -v sha256sum >/dev/null 2>&1; then
    line="$(sha256sum "$1")"
  elif command -v shasum >/dev/null 2>&1; then
    line="$(shasum -a 256 "$1")"
  else
    log "sha256sum/shasum unavailable; skipping package registry cache"
    return 1
  fi

  printf '%s\n' "${line%%[[:space:]]*}"
}

required_env_present() {
  local missing=0

  for name in CI_API_V4_URL CI_PROJECT_ID CI_JOB_TOKEN CI_PROJECT_DIR; do
    if [ -z "${!name:-}" ]; then
      log "$name is not set; skipping package registry cache"
      missing=1
    fi
  done

  [ "$missing" -eq 0 ]
}

cache_url() {
  local cache_key lock_hash system

  cache_key="${CI_NIX_CACHE_KEY:-ci-shaders}"
  system="${CI_NIX_SYSTEM:-x86_64-linux}"
  lock_hash="$(hash_file "${CI_PROJECT_DIR}/flake.lock")" || return 1

  printf '%s/projects/%s/packages/generic/nix-cache-%s/%s-%s/cache.tar.gz' \
    "$CI_API_V4_URL" \
    "$CI_PROJECT_ID" \
    "$cache_key" \
    "$system" \
    "$lock_hash"
}

have_tools() {
  local missing=0

  for tool in curl gzip mktemp tar; do
    if ! command -v "$tool" >/dev/null 2>&1; then
      log "$tool is unavailable; skipping package registry cache"
      missing=1
    fi
  done

  [ "$missing" -eq 0 ]
}

restore_cache() {
  required_env_present || return 0
  have_tools || return 0

  local tmp url
  url="$(cache_url)" || return 0
  tmp="$(mktemp -t ci-nix-package-cache.XXXXXX)"
  trap 'rm -f "$tmp"' RETURN

  log "checking package registry cache: $url"
  if ! curl --fail --location --silent --show-error \
    --header "JOB-TOKEN: ${CI_JOB_TOKEN}" \
    --output "$tmp" \
    "$url"; then
    log "package registry cache miss or unavailable; continuing without it"
    return 0
  fi

  rm -rf "${CI_PROJECT_DIR}/.nix-cache"
  if ! tar -C "$CI_PROJECT_DIR" -xzf "$tmp" || [ ! -f "${CI_PROJECT_DIR}/.nix-cache/nix-cache-info" ]; then
    log "package registry cache archive is invalid; ignoring it"
    rm -rf "${CI_PROJECT_DIR}/.nix-cache"
    return 0
  fi

  log "restored package registry cache"
}

trusted_ref() {
  [ -n "${CI_DEFAULT_BRANCH:-}" ] && [ "${CI_COMMIT_REF_NAME:-}" = "$CI_DEFAULT_BRANCH" ] && return 0
  [ "${CI_COMMIT_REF_PROTECTED:-}" = "true" ]
}

save_cache() {
  required_env_present || return 0
  have_tools || return 0

  if [ "${CI_JOB_STATUS:-}" != "success" ]; then
    log "skipping package registry cache save: job status is ${CI_JOB_STATUS:-unset}"
    return 0
  fi
  if [ "${CI_NIX_PACKAGE_CACHE_SAVE:-false}" != "true" ]; then
    log "skipping package registry cache save: CI_NIX_PACKAGE_CACHE_SAVE is not true"
    return 0
  fi
  if ! trusted_ref; then
    log "skipping package registry cache save: ref is not default branch or protected"
    return 0
  fi
  if [ ! -e "${CI_NIX_PROFILE:-}" ]; then
    log "skipping package registry cache save: CI_NIX_PROFILE does not exist"
    return 0
  fi
  if [ ! -f "${CI_PROJECT_DIR}/.nix-cache/nix-cache-info" ]; then
    log "skipping package registry cache save: .nix-cache is missing or invalid"
    return 0
  fi

  local tmp url
  url="$(cache_url)" || return 0
  tmp="$(mktemp -t ci-nix-package-cache.XXXXXX)"
  trap 'rm -f "$tmp"' RETURN

  tar -C "$CI_PROJECT_DIR" -czf "$tmp" .nix-cache
  log "uploading package registry cache: $url"
  if curl --fail --silent --show-error \
    --request PUT \
    --header "JOB-TOKEN: ${CI_JOB_TOKEN}" \
    --upload-file "$tmp" \
    "$url"; then
    log "uploaded package registry cache"
  else
    log "package registry cache upload failed; continuing"
  fi
}

case "${1:-}" in
  restore) restore_cache ;;
  save) save_cache ;;
  *)
    printf 'usage: %s {restore|save}\n' "$0" >&2
    exit 2
    ;;
esac
