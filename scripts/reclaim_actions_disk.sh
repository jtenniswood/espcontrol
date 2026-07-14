#!/usr/bin/env bash
set -euo pipefail

SCRIPT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")

assert_exists() {
  local path=$1
  local label=$2
  if [ ! -e "$path" ]; then
    echo "::error::self-test failed: ${label} should exist" >&2
    exit 1
  fi
}

assert_missing() {
  local path=$1
  local label=$2
  if [ -e "$path" ]; then
    echo "::error::self-test failed: ${label} should have been removed" >&2
    exit 1
  fi
}

run_self_test() {
  local tmp
  tmp=$(mktemp -d)
  SELF_TEST_TMP=$tmp
  trap 'rm -rf "$SELF_TEST_TMP"' EXIT

  local home="$tmp/home"
  local workspace="$tmp/workspace"
  local update_dir="$home/actions-runner-1/_work/_update"
  local legacy_cache="$home/.cache/espcontrol-actions/esphome"
  mkdir -p "$workspace" "$update_dir" "$legacy_cache"
  touch "$update_dir/stale.update" "$legacy_cache/legacy.cache"

  HOME="$home" \
    GITHUB_WORKSPACE="$workspace" \
    DOCKER=/bin/false \
    ESPCONTROL_DOCKER_LOCK_HELD=true \
    bash "$SCRIPT_PATH" >/dev/null

  assert_missing "$update_dir" "runner update cleanup"
  assert_missing "$legacy_cache" "legacy ESPHome cache cleanup"

  local disabled_home="$tmp/disabled-home"
  local disabled_update="$disabled_home/actions-runner-1/_work/_update"
  local disabled_legacy="$disabled_home/.cache/espcontrol-actions/esphome"
  mkdir -p "$disabled_update" "$disabled_legacy"
  touch "$disabled_update/stale.update" "$disabled_legacy/legacy.cache"

  HOME="$disabled_home" \
    GITHUB_WORKSPACE="$workspace" \
    DOCKER=/bin/false \
    ESPCONTROL_DOCKER_LOCK_HELD=true \
    ESPCONTROL_CLEAN_RUNNER_UPDATES=false \
    ESPCONTROL_CLEAN_LEGACY_ESPHOME_CACHE=false \
    bash "$SCRIPT_PATH" >/dev/null

  assert_exists "$disabled_update/stale.update" "disabled runner update cleanup"
  assert_exists "$disabled_legacy/legacy.cache" "disabled legacy ESPHome cache cleanup"

  echo "Runner disk cleanup self-tests passed."
}

if [ "${1:-}" = "--self-test" ]; then
  run_self_test
  exit 0
fi

DOCKER_COMMAND=${DOCKER:-docker}
KEEP_STORAGE=${ESPCONTROL_DOCKER_BUILD_CACHE_KEEP:-2GB}
LOCK_FILE=${ESPCONTROL_DOCKER_LOCK_FILE:-/tmp/espcontrol-esphome-image.lock}
PRUNE_ALL_IMAGES=${ESPCONTROL_DOCKER_PRUNE_ALL_IMAGES:-false}
PRUNE_VOLUMES=${ESPCONTROL_DOCKER_PRUNE_VOLUMES:-false}
CLEAN_RUNNER_UPDATES=${ESPCONTROL_CLEAN_RUNNER_UPDATES:-true}
CLEAN_LEGACY_ESPHOME_CACHE=${ESPCONTROL_CLEAN_LEGACY_ESPHOME_CACHE:-true}

if [ "${ESPCONTROL_DOCKER_LOCK_HELD:-false}" != "true" ] && command -v flock >/dev/null 2>&1; then
  export ESPCONTROL_DOCKER_LOCK_HELD=true
  exec flock "${LOCK_FILE}" bash "$0" "$@"
fi

disk_report() {
  local label=$1
  echo "${label}:"
  df -h "${GITHUB_WORKSPACE:-$PWD}" "$HOME" 2>/dev/null || true
}

disk_report "Disk before cleanup"

if ${DOCKER_COMMAND} info >/dev/null 2>&1; then
  ${DOCKER_COMMAND} container prune -f >/dev/null || true
  if [ "${PRUNE_ALL_IMAGES}" = "true" ]; then
    ${DOCKER_COMMAND} image prune -af >/dev/null || true
  else
    ${DOCKER_COMMAND} image prune -f >/dev/null || true
  fi
  ${DOCKER_COMMAND} builder prune -f --keep-storage "${KEEP_STORAGE}" >/dev/null || true
  if [ "${PRUNE_VOLUMES}" = "true" ]; then
    ${DOCKER_COMMAND} volume prune -f >/dev/null || true
  fi
  ${DOCKER_COMMAND} system df || true
else
  echo "Docker is not available; skipping Docker disk cleanup."
fi

if [ "${CLEAN_RUNNER_UPDATES}" = "true" ]; then
  for update_dir in "$HOME"/actions-runner*/_work/_update; do
    [ -d "${update_dir}" ] || continue
    echo "Removing stale runner update directory: ${update_dir}"
    rm -rf "${update_dir}" || sudo rm -rf "${update_dir}" || true
  done
fi

if [ "${CLEAN_LEGACY_ESPHOME_CACHE}" = "true" ]; then
  legacy_cache="${HOME}/.cache/espcontrol-actions/esphome"
  if [ -d "${legacy_cache}" ]; then
    echo "Removing legacy persistent ESPHome cache: ${legacy_cache}"
    rm -rf "${legacy_cache}" || sudo rm -rf "${legacy_cache}" || true
  fi
fi

disk_report "Disk after cleanup"
