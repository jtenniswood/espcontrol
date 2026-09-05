#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPOSITORY_DIR="$(cd "${PROJECT_DIR}/../.." && pwd)"
OUTPUT_DIR="${OUTPUT_DIR:-${PROJECT_DIR}/.build/standalone}"
APP_NAME="EspControl Companion.app"
APP_DIR="${OUTPUT_DIR}/${APP_NAME}"
EXECUTABLE_NAME="EspControl Companion"
VERSION="${MARKETING_VERSION:-1.0.0}"
BUILD_NUMBER="${CURRENT_PROJECT_VERSION:-1}"
PRODUCT_BUNDLE_IDENTIFIER="${PRODUCT_BUNDLE_IDENTIFIER:-io.espcontrol.companion}"
ALLOW_ADHOC="${ALLOW_ADHOC:-0}"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS because it uses SwiftPM and codesign." >&2
    exit 1
fi

if [[ "${ALLOW_ADHOC}" != "1" && -z "${CODE_SIGN_IDENTITY:-}" ]]; then
    echo "Set CODE_SIGN_IDENTITY to a Developer ID Application identity, or use ALLOW_ADHOC=1 for local verification only." >&2
    exit 2
fi

rm -rf "${APP_DIR}" "${OUTPUT_DIR}/AppIcon.iconset" "${OUTPUT_DIR}/generate_macos_icon"
mkdir -p "${APP_DIR}/Contents/MacOS" "${APP_DIR}/Contents/Resources"

echo "Building the standalone Companion product…"
swift build \
    --package-path "${PROJECT_DIR}" \
    --configuration release \
    --product "${EXECUTABLE_NAME}" \
    -Xswiftc -warnings-as-errors

BUILD_BIN_PATH="$(swift build \
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

plutil -replace CFBundleIdentifier -string "${PRODUCT_BUNDLE_IDENTIFIER}" "${APP_DIR}/Contents/Info.plist"
plutil -replace CFBundleShortVersionString -string "${VERSION}" "${APP_DIR}/Contents/Info.plist"
plutil -replace CFBundleVersion -string "${BUILD_NUMBER}" "${APP_DIR}/Contents/Info.plist"
plutil -lint "${APP_DIR}/Contents/Info.plist"
plutil -lint "${APP_DIR}/Contents/Resources/PrivacyInfo.xcprivacy"

SIGNING_IDENTITY="${CODE_SIGN_IDENTITY:--}"
codesign --force --options runtime \
    --entitlements "${SCRIPT_DIR}/EspControlCompanion.entitlements" \
    --sign "${SIGNING_IDENTITY}" "${APP_DIR}"

if codesign --display --entitlements - "${APP_DIR}" 2>&1 | grep -q 'com.apple.security.app-sandbox'; then
    echo "The standalone bundle must not contain the App Sandbox entitlement." >&2
    exit 1
fi

codesign --verify --deep --strict "${APP_DIR}"
echo "Built and verified: ${APP_DIR}"
