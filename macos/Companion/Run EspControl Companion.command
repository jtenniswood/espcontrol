#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" && pwd)"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "EspControl Companion can only run on macOS."
  exit 1
fi

if ! command -v swift >/dev/null 2>&1; then
  echo "Swift is unavailable. Install Xcode from Apple's developer tools, open it once, then try again."
  exit 1
fi

cd "$SCRIPT_DIR"
echo "Building and starting EspControl Companion…"
echo "Leave this Terminal window open while testing. Stop the app with Control-C."
swift run "EspControl Companion"
