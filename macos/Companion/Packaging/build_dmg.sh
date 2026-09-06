#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
APP_NAME="EspControl Companion.app"
APP_PATH="${1:-${PROJECT_DIR}/.build/standalone/${APP_NAME}}"
SKIP_FINDER_LAYOUT="${SKIP_FINDER_LAYOUT:-0}"
VOLUME_NAME="${VOLUME_NAME:-EspControl}"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS because it uses hdiutil and Finder." >&2
    exit 1
fi

for required_command in hdiutil osascript plutil ditto; do
    if ! command -v "${required_command}" >/dev/null 2>&1; then
        echo "Required command is missing: ${required_command}" >&2
        exit 1
    fi
done

if [[ ! -d "${APP_PATH}" ]]; then
    if [[ -n "${1:-}" ]]; then
        echo "Companion app bundle not found: ${APP_PATH}" >&2
        exit 2
    fi
    echo "Companion app bundle not found; building it first…"
    "${SCRIPT_DIR}/build_standalone.sh"
fi

APP_PATH="$(cd "$(dirname "${APP_PATH}")" && pwd)/$(basename "${APP_PATH}")"
APP_VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "${APP_PATH}/Contents/Info.plist")"
OUTPUT_DIR="${OUTPUT_DIR:-$(dirname "${APP_PATH}")}"
DMG_PATH="${DMG_PATH:-${OUTPUT_DIR}/EspControl Companion-${APP_VERSION}.dmg}"
RW_DMG="${OUTPUT_DIR}/.EspControl Companion-${APP_VERSION}.rw.dmg"
STAGING_DIR="$(mktemp -d -t espcontrol-companion-dmg)"
MOUNT_DIR="$(mktemp -d -t espcontrol-companion-mount)"
ATTACHED=0

cleanup() {
    if [[ "${ATTACHED}" == "1" ]]; then
        hdiutil detach "${MOUNT_DIR}" -force >/dev/null 2>&1 || true
    fi
    rm -f "${RW_DMG}"
    rm -rf "${STAGING_DIR}"
    rm -rf "${MOUNT_DIR}"
}
trap cleanup EXIT

mkdir -p "${OUTPUT_DIR}"
rm -f "${DMG_PATH}" "${RW_DMG}"
ditto --rsrc --extattr --acl "${APP_PATH}" "${STAGING_DIR}/${APP_NAME}"
ln -s /Applications "${STAGING_DIR}/Applications"

echo "Creating writable Companion disk image…"
hdiutil create \
    -volname "${VOLUME_NAME}" \
    -srcfolder "${STAGING_DIR}" \
    -fs APFS \
    -format UDRW \
    -ov "${RW_DMG}" >/dev/null
hdiutil attach "${RW_DMG}" \
    -readwrite \
    -noverify \
    -noautoopen \
    -mountpoint "${MOUNT_DIR}" >/dev/null
ATTACHED=1

if [[ "${SKIP_FINDER_LAYOUT}" != "1" ]]; then
    echo "Configuring Finder drag-to-Applications layout…"
    osascript <<APPLESCRIPT
tell application "Finder"
    set mountedFolder to POSIX file "${MOUNT_DIR}" as alias
    open mountedFolder
    delay 1
    set dmgWindow to front window
    set current view of dmgWindow to icon view
    set toolbar visible of dmgWindow to false
    set statusbar visible of dmgWindow to false
    set bounds of dmgWindow to {100, 100, 820, 560}
    set iconViewOptions to icon view options of dmgWindow
    set arrangement of iconViewOptions to not arranged
    set icon size of iconViewOptions to 96
    set text size of iconViewOptions to 14
    set position of item "${APP_NAME}" of mountedFolder to {210, 250}
    set position of item "Applications" of mountedFolder to {610, 250}
    close dmgWindow
    delay 1
end tell
APPLESCRIPT
fi

hdiutil detach "${MOUNT_DIR}" >/dev/null
ATTACHED=0

echo "Compressing Companion disk image…"
hdiutil convert "${RW_DMG}" \
    -format UDZO \
    -imagekey zlib-level=9 \
    -ov \
    -o "${DMG_PATH}" >/dev/null
hdiutil verify "${DMG_PATH}" >/dev/null
echo "Built and verified: ${DMG_PATH}"
