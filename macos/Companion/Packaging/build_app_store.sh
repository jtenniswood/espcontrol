#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPOSITORY_DIR="$(cd "${PROJECT_DIR}/../.." && pwd)"
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_DIR}/.build/app-store}"
APP_NAME="EspControl Companion.app"
APP_DIR="${OUTPUT_DIR}/${APP_NAME}"
EXECUTABLE_NAME="EspControl Companion"
VERSION="${MARKETING_VERSION:-1.0.0}"
BUILD_NUMBER="${CURRENT_PROJECT_VERSION:-1}"
BUNDLE_IDENTIFIER="${PRODUCT_BUNDLE_IDENTIFIER:-io.espcontrol.companion}"
ALLOW_ADHOC="${ALLOW_ADHOC:-0}"
CREATE_PKG="${CREATE_PKG:-0}"

if [[ "${VERSION}" == 0.* ]]; then
    echo "Mac App Store builds must use a release version of 1.0.0 or later." >&2
    exit 2
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS because it uses SwiftPM, codesign, and productbuild." >&2
    exit 1
fi

if [[ "${ALLOW_ADHOC}" != "1" && -z "${CODE_SIGN_IDENTITY:-}" ]]; then
    echo "Set CODE_SIGN_IDENTITY to your Mac App Store distribution identity, or use ALLOW_ADHOC=1 for local verification only." >&2
    exit 2
fi

if [[ "${CREATE_PKG}" == "1" && -z "${INSTALLER_IDENTITY:-}" ]]; then
    echo "CREATE_PKG=1 requires INSTALLER_IDENTITY (3rd Party Mac Developer Installer)." >&2
    exit 2
fi

if [[ "${ALLOW_ADHOC}" != "1" && -z "${PROVISIONING_PROFILE:-}" ]]; then
    echo "App Store signing requires PROVISIONING_PROFILE pointing to the Mac App Store provisioning profile." >&2
    exit 2
fi

rm -rf "${APP_DIR}" "${OUTPUT_DIR}/AppIcon.iconset" "${OUTPUT_DIR}/generate_macos_icon"
mkdir -p "${APP_DIR}/Contents/MacOS" "${APP_DIR}/Contents/Resources"

echo "Building the App Store-safe Swift product…"
ESPCONTROL_APP_STORE=1 swift build \
    --package-path "${PROJECT_DIR}" \
    --configuration release \
    --product "${EXECUTABLE_NAME}" \
    -Xswiftc -warnings-as-errors

BUILD_BIN_PATH="$(ESPCONTROL_APP_STORE=1 swift build \
    --package-path "${PROJECT_DIR}" \
    --configuration release \
    --product "${EXECUTABLE_NAME}" \
    --show-bin-path)"
cp "${BUILD_BIN_PATH}/${EXECUTABLE_NAME}" "${APP_DIR}/Contents/MacOS/${EXECUTABLE_NAME}"
cp "${SCRIPT_DIR}/Info.plist" "${APP_DIR}/Contents/Info.plist"
cp "${SCRIPT_DIR}/PrivacyInfo.xcprivacy" "${APP_DIR}/Contents/Resources/PrivacyInfo.xcprivacy"

ICONSET_DIR="${OUTPUT_DIR}/AppIcon.iconset"
ICON_GENERATOR="${OUTPUT_DIR}/generate_macos_icon"
CLANG_MODULE_CACHE_PATH="${OUTPUT_DIR}/clang-module-cache" swiftc \
    "${REPOSITORY_DIR}/scripts/generate_macos_icon.swift" \
    -o "${ICON_GENERATOR}"
CLANG_MODULE_CACHE_PATH="${OUTPUT_DIR}/clang-module-cache" "${ICON_GENERATOR}" \
    "${SCRIPT_DIR}/AppIcon.svg" "${ICONSET_DIR}"
iconutil --convert icns --output "${APP_DIR}/Contents/Resources/AppIcon.icns" "${ICONSET_DIR}"

if [[ -n "${PROVISIONING_PROFILE:-}" ]]; then
    cp "${PROVISIONING_PROFILE}" "${APP_DIR}/Contents/embedded.provisionprofile"
fi

plutil -replace CFBundleIdentifier -string "${BUNDLE_IDENTIFIER}" "${APP_DIR}/Contents/Info.plist"
plutil -replace CFBundleShortVersionString -string "${VERSION}" "${APP_DIR}/Contents/Info.plist"
plutil -replace CFBundleVersion -string "${BUILD_NUMBER}" "${APP_DIR}/Contents/Info.plist"
plutil -remove NSAppleEventsUsageDescription "${APP_DIR}/Contents/Info.plist" 2>/dev/null || true
plutil -lint "${APP_DIR}/Contents/Info.plist"
plutil -lint "${APP_DIR}/Contents/Resources/PrivacyInfo.xcprivacy"
if [[ "$(plutil -extract LSApplicationCategoryType raw "${APP_DIR}/Contents/Info.plist")" != \
      "public.app-category.utilities" ]]; then
    echo "The App Store bundle must declare its Utilities category." >&2
    exit 1
fi

SIGNING_IDENTITY="${CODE_SIGN_IDENTITY:--}"
codesign --force --options runtime \
    --entitlements "${SCRIPT_DIR}/EspControlCompanion.app-store.entitlements" \
    --sign "${SIGNING_IDENTITY}" "${APP_DIR}"

APP_BINARY="${APP_DIR}/Contents/MacOS/${EXECUTABLE_NAME}"
if strings "${APP_BINARY}" | grep -E -q 'MediaRemote\.framework|MRMediaRemote'; then
    echo "The App Store build contains the private MediaRemote framework bridge." >&2
    exit 1
fi
if otool -L "${APP_BINARY}" | grep -q 'MediaRemote'; then
    echo "The App Store build links the private MediaRemote framework." >&2
    exit 1
fi

codesign --verify --deep --strict "${APP_DIR}"
echo "Built and verified: ${APP_DIR}"

if [[ "${CREATE_PKG}" == "1" ]]; then
    PKG_PATH="${OUTPUT_DIR}/EspControl-Companion-${VERSION}.pkg"
    rm -f "${PKG_PATH}"
    productbuild \
        --component "${APP_DIR}" /Applications \
        --sign "${INSTALLER_IDENTITY}" \
        "${PKG_PATH}"
    echo "Built installer package: ${PKG_PATH}"
fi
