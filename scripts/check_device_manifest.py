#!/usr/bin/env python3
"""Validate the composed device catalog before generators consume it."""

from __future__ import annotations

import argparse
import copy
import sys

from device_profiles import (
    DEVICE_CATALOG,
    DeviceProfileError,
    common_font_ids,
    compose_catalog_data,
    load_catalog_data,
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
    data = load_catalog_data(DEVICE_CATALOG)
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

    invalid_image_capacity = copy.deepcopy(data)
    slug, device = first_device(invalid_image_capacity)
    device["capabilities"]["imageSlots"] = 7
    expect_error(
        validate_manifest_data(invalid_image_capacity, fonts),
        f"{slug}: capabilities.imageSlots must be an integer from 0 to 6",
    )

    legacy_image_capacity = copy.deepcopy(data)
    slug, device = first_device(legacy_image_capacity)
    device["firmware"]["display"]["imageCardDownloaders"] = 6
    expect_error(
        validate_manifest_data(legacy_image_capacity, fonts),
        f"{slug}: firmware.display.imageCardDownloaders has moved to capabilities.imageSlots",
    )

    catalog = {
        "settings": {},
        "profiles": {
            "platform": {"p4": {"firmware": {"build": {"chip": "ESP32-P4"}}}},
            "display": {"panel": {"layout": {"cols": 4}, "tags": ["display"]}},
            "fonts": {},
            "network": {},
            "artwork": {},
            "audio": {},
            "input": {},
        },
        "devices": {
            "example": {
                "profiles": {"platform": "p4", "display": ["panel"]},
                "config": {"slots": 8},
                "overrides": {"layout": {"cols": 5}, "tags": ["override"]},
            }
        },
    }
    composed = compose_catalog_data(catalog)
    assert list(composed["devices"]) == ["example"]
    assert list(composed["devices"]["example"]) == ["slots", "layout", "firmware", "tags"]
    assert composed["devices"]["example"]["layout"]["cols"] == 5
    assert composed["devices"]["example"]["tags"] == ["override"]

    def expect_catalog_error(mutator, expected: str) -> None:
        invalid = copy.deepcopy(catalog)
        mutator(invalid)
        try:
            compose_catalog_data(invalid)
        except DeviceProfileError as exc:
            if expected not in str(exc):
                raise AssertionError(f"expected {expected!r}, got {str(exc)!r}") from exc
        else:
            raise AssertionError(f"expected catalog error containing {expected!r}")

    expect_catalog_error(
        lambda value: value["devices"]["example"]["profiles"].update({"hardware": "p4"}),
        "unknown categories",
    )
    expect_catalog_error(
        lambda value: value["devices"]["example"]["profiles"].update({"fonts": "missing"}),
        "missing profile reference",
    )
    expect_catalog_error(
        lambda value: value["devices"]["example"]["config"].update({"layout": {"cols": 4}}),
        "config collides",
    )
    expect_catalog_error(
        lambda value: value["devices"]["example"]["overrides"].update({"newField": True}),
        "override must replace",
    )
    expect_catalog_error(
        lambda value: value["profiles"]["display"]["panel"].update({"bad": None}),
        "null values are not allowed",
    )
    conflicting = copy.deepcopy(catalog)
    conflicting["profiles"]["network"]["wifi"] = {"firmware": {"build": {"chip": "ESP32-S3"}}}
    conflicting["devices"]["example"]["profiles"]["network"] = "wifi"
    try:
        compose_catalog_data(conflicting)
    except DeviceProfileError as exc:
        assert "conflicting profile values" in str(exc)
        assert "profiles.platform.p4" in str(exc) and "profiles.network.wifi" in str(exc)
    else:
        raise AssertionError("expected conflicting profile leaves to fail")

    ordered = copy.deepcopy(catalog)
    ordered["devices"] = {
        "second": ordered["devices"]["example"],
        "first": copy.deepcopy(ordered["devices"]["example"]),
    }
    assert list(compose_catalog_data(ordered)["devices"]) == ["second", "first"]

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
        data = load_catalog_data(DEVICE_CATALOG)
    except DeviceProfileError as exc:
        print(f"ERROR: {exc}")
        return 1

    errors = validate_manifest_data(data)
    if errors:
        print(f"ERROR: {rel(DEVICE_CATALOG)} failed validation:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"{rel(DEVICE_CATALOG)} passed validation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
