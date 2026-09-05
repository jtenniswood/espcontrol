#!/bin/bash
set -euo pipefail

APP_PATH="${1:-}"
if [[ -z "${APP_PATH}" || ! -d "${APP_PATH}" ]]; then
    echo "Usage: $0 /path/to/EspControl\\ Companion.app" >&2
    exit 2
fi

: "${NOTARY_KEY_PATH:?Set NOTARY_KEY_PATH to the App Store Connect API .p8 key}"
: "${NOTARY_KEY_ID:?Set NOTARY_KEY_ID to the App Store Connect API key ID}"
: "${NOTARY_ISSUER_ID:?Set NOTARY_ISSUER_ID to the App Store Connect issuer ID}"

APP_DIR="$(cd "$(dirname "${APP_PATH}")" && pwd)"
APP_NAME="$(basename "${APP_PATH}")"
APP_VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "${APP_PATH}/Contents/Info.plist")"
SUBMISSION_ZIP="${APP_DIR}/${APP_NAME%.app}-submission.zip"
FINAL_ZIP="${APP_DIR}/${APP_NAME%.app}-${APP_VERSION}.zip"

rm -f "${SUBMISSION_ZIP}" "${FINAL_ZIP}"
ditto -c -k --keepParent "${APP_PATH}" "${SUBMISSION_ZIP}"

echo "Submitting ${APP_NAME} to Apple notarization…"
xcrun notarytool submit "${SUBMISSION_ZIP}" \
    --key "${NOTARY_KEY_PATH}" \
    --key-id "${NOTARY_KEY_ID}" \
    --issuer "${NOTARY_ISSUER_ID}" \
    --wait

echo "Stapling and verifying Apple’s notarization ticket…"
xcrun stapler staple "${APP_PATH}"
xcrun stapler validate "${APP_PATH}"
codesign --verify --deep --strict --verbose=2 "${APP_PATH}"
spctl --assess --type execute --verbose=4 "${APP_PATH}"

ditto -c -k --keepParent "${APP_PATH}" "${FINAL_ZIP}"
rm -f "${SUBMISSION_ZIP}"
echo "Built notarized release: ${FINAL_ZIP}"
