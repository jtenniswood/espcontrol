#!/usr/bin/env python3
"""Generate the expanded compatibility manifest from devices/catalog.json."""

from __future__ import annotations

import argparse
import json
import sys

from device_profiles import (
    DEVICE_CATALOG,
    DEVICE_MANIFEST,
    DeviceProfileError,
    load_catalog_data,
    rel,
    validate_manifest_data,
)


def render_manifest(data: dict) -> str:
    # JSON preserves catalog/profile insertion order. This is intentional: firmware
    # substitutions are emitted in their established order rather than alphabetically.
    return json.dumps(data, indent=2, ensure_ascii=False) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail when the committed manifest is stale")
    args = parser.parse_args(argv)

    try:
        data = load_catalog_data(DEVICE_CATALOG)
        errors = validate_manifest_data(data)
    except DeviceProfileError as exc:
        print(f"ERROR: {exc}")
        return 1
    if errors:
        print(f"ERROR: {rel(DEVICE_CATALOG)} failed validation:")
        for error in errors:
            print(f"  - {error}")
        return 1

    rendered = render_manifest(data)
    if args.check:
        current = DEVICE_MANIFEST.read_text(encoding="utf-8") if DEVICE_MANIFEST.exists() else None
        if current != rendered:
            print(
                f"ERROR: {rel(DEVICE_MANIFEST)} is stale; "
                "run python3 scripts/generate_device_manifest.py"
            )
            return 1
        print(f"{rel(DEVICE_MANIFEST)} matches {rel(DEVICE_CATALOG)}.")
        return 0

    DEVICE_MANIFEST.write_text(rendered, encoding="utf-8")
    print(f"Generated {rel(DEVICE_MANIFEST)} from {rel(DEVICE_CATALOG)}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
