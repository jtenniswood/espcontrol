#!/usr/bin/env python3
"""Validate devices/manifest.json before generators consume it."""

from __future__ import annotations

import sys

from device_profiles import (
    DEVICE_MANIFEST,
    DeviceProfileError,
    load_manifest_data,
    public_docs_stem,
    rel,
    validate_manifest_data,
)


def self_test() -> None:
    data = load_manifest_data(DEVICE_MANIFEST)
    slugs = sorted(data["devices"])
    assert len(slugs) >= 2, "self-test needs at least two devices"
    first, second = slugs[:2]
    first_stem = public_docs_stem(data["devices"][first]["public"]["docsPath"])
    data["devices"][second]["public"]["docsPath"] = f"/screens/archive/{first_stem}/"
    errors = validate_manifest_data(data)
    expected = f"{second}: public.docsPath stem duplicates {first}: {first_stem}"
    assert expected in errors, errors
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
    if errors:
        print(f"ERROR: {rel(DEVICE_MANIFEST)} failed validation:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"{rel(DEVICE_MANIFEST)} passed validation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
