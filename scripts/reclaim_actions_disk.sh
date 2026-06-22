#!/usr/bin/env bash
set -euo pipefail

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
