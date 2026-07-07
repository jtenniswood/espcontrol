#!/usr/bin/env python3
"""Validate devices/manifest.json before generators consume it."""

from __future__ import annotations

import argparse
import copy
import sys

from device_profiles import (
    DEVICE_MANIFEST,
    DeviceProfileError,
    common_font_ids,
    load_manifest_data,
    public_docs_stem,
    rel,
    validate_manifest_data,
)


def expect_error(errors: list[str], expected: str) -> None:
    if not any(expected in error for error in errors):
        raise AssertionError(f"expected error containing {expected!r}, got {errors!r}")


def first_device(data: dict) -> tuple[str, dict]:
    slug = next(iter(data["devices"]))
    return slug, data["devices"][slug]


def run_self_test() -> int:
    data = load_manifest_data(DEVICE_MANIFEST)
    fonts = common_font_ids()
    assert validate_manifest_data(data, fonts) == []

    expect_error(validate_manifest_data([], fonts), "must contain a JSON object")
    expect_error(validate_manifest_data({"devices": {}}, fonts), "non-empty devices object")

    invalid_slots = copy.deepcopy(data)
    slug, device = first_device(invalid_slots)
    device["slots"] = device["slots"] + 1
    expect_error(
        validate_manifest_data(invalid_slots, fonts),
        f"{slug}: slots must equal layout.cols * layout.rows",
    )

    invalid_font = copy.deepcopy(data)
    slug, device = first_device(invalid_font)
    device["firmware"]["fonts"]["icon"] = "missing_font_id"
    expect_error(
        validate_manifest_data(invalid_font, fonts),
        f"{slug}: firmware.fonts.icon references unknown font id",
    )

    invalid_chip = copy.deepcopy(data)
    slug, device = first_device(invalid_chip)
    device["firmware"]["build"]["chip"] = "ESP32-C3"
    expect_error(
        validate_manifest_data(invalid_chip, fonts),
        f"{slug}: firmware.build.chip must be one of",
    )

    invalid_web = copy.deepcopy(data)
    slug, device = first_device(invalid_web)
    device["web"]["dragMode"] = "free"
    expect_error(
        validate_manifest_data(invalid_web, fonts),
        f"{slug}: web.dragMode must be swap or displace",
    )

    duplicate_docs_path = copy.deepcopy(data)
    slugs = sorted(duplicate_docs_path["devices"])
    assert len(slugs) >= 2, "self-test needs at least two devices"
    first, second = slugs[:2]
    first_stem = public_docs_stem(duplicate_docs_path["devices"][first]["public"]["docsPath"])
    duplicate_docs_path["devices"][second]["public"]["docsPath"] = f"/screens/archive/{first_stem}/"
    expect_error(
        validate_manifest_data(duplicate_docs_path, fonts),
        f"{second}: public.docsPath stem duplicates {first}: {first_stem}",
    )

    print("Device manifest self-test passed.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run validator self-tests")
    args = parser.parse_args(argv)

    if args.self_test:
        return run_self_test()

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
