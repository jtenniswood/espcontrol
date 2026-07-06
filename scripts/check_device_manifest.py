#!/usr/bin/env python3
"""Validate devices/manifest.json before generators consume it."""

from __future__ import annotations

import sys
from typing import Any

from device_profiles import DEVICE_MANIFEST, DeviceProfileError, load_manifest_data, rel, validate_manifest_data


def validate_unique_docs_paths(data: dict[str, Any]) -> list[str]:
    devices = data.get("devices")
    if not isinstance(devices, dict):
        return []

    seen: dict[str, str] = {}
    errors: list[str] = []
    for slug, device in sorted(devices.items()):
        if not isinstance(device, dict):
            continue
        public = device.get("public")
        if not isinstance(public, dict):
            continue
        docs_path = public.get("docsPath")
        if not isinstance(docs_path, str) or not docs_path:
            continue
        previous = seen.get(docs_path)
        if previous is not None:
            errors.append(f"{slug}: public.docsPath duplicates {previous}: {docs_path}")
            continue
        seen[docs_path] = slug
    return errors


def self_test() -> None:
    data = {
        "devices": {
            "alpha": {"public": {"docsPath": "/screens/shared"}},
            "beta": {"public": {"docsPath": "/screens/shared"}},
            "gamma": {"public": {"docsPath": "/screens/gamma"}},
        }
    }
    errors = validate_unique_docs_paths(data)
    assert errors == ["beta: public.docsPath duplicates alpha: /screens/shared"], errors
    assert validate_unique_docs_paths({"devices": {}}) == []
    print("Device manifest self-test passed.")


def main() -> int:
    if sys.argv[1:] == ["--self-test"]:
        self_test()
        return 0

    try:
        data = load_manifest_data(DEVICE_MANIFEST)
    except DeviceProfileError as exc:
        print(f"ERROR: {exc}")
        return 1

    errors = validate_manifest_data(data)
    errors.extend(validate_unique_docs_paths(data))
    if errors:
        print(f"ERROR: {rel(DEVICE_MANIFEST)} failed validation:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"{rel(DEVICE_MANIFEST)} passed validation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
