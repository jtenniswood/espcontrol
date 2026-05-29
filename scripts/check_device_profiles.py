#!/usr/bin/env python3
"""Cross-check generated device profile outputs against devices/manifest.json."""

from __future__ import annotations

import json
from pathlib import Path

import device_matrix
from device_profiles import ROOT, load_device_profiles, public_device_capabilities
import check_public_firmware


WEB_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"
DEVICE_CAPABILITIES_JSON = ROOT / "docs" / "public" / "device-profiles.json"
DEVICE_DOCS_DIR = ROOT / "docs" / "generated" / "screens"
COMPAT_FIXTURES = ROOT / "compatibility" / "fixtures" / "product_compatibility.json"
REQUIRED_SETUP_ICON_GLYPHS = {
    r'"\U000F012C"': "mdi-check",
    r'"\U000F0996"': "mdi-progress-clock",
}


def read_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def compatibility_required_slugs() -> list[str]:
    fixture = read_json(COMPAT_FIXTURES)
    return fixture["current"]["deviceProfiles"]["requiredSlugs"]


def docs_stem(capability: dict) -> str:
    return capability["docsPath"].rstrip("/").split("/")[-1]


def assert_profile_slugs(profile_slugs: list[str], values: list[str], label: str) -> None:
    assert values == profile_slugs, f"{label} slugs differ: {values} != {profile_slugs}"


def test_public_device_capabilities(profile_slugs: list[str]) -> None:
    expected = public_device_capabilities()
    actual = read_json(DEVICE_CAPABILITIES_JSON)
    assert actual == expected, "public device capability JSON is stale"
    assert_profile_slugs(profile_slugs, [device["slug"] for device in actual["devices"]], "public capability")

    for capability in actual["devices"]:
        stem = docs_stem(capability)
        grid = (DEVICE_DOCS_DIR / f"{stem}-grid.md").read_text(encoding="utf-8")
        install = (DEVICE_DOCS_DIR / f"{stem}-install.md").read_text(encoding="utf-8")
        assert f'**{capability["slots"]} card slots**' in grid, f"{stem}: grid snippet missing slot count"
        assert f'{capability["grid"]["rows"]}-row x {capability["grid"]["cols"]}-column' in grid, (
            f"{stem}: grid snippet missing grid shape"
        )
        assert capability["screenSize"] in grid, f"{stem}: grid snippet missing screen size"
        assert capability["resolution"] in grid, f"{stem}: grid snippet missing resolution"
        assert capability["chipFamily"] in grid, f"{stem}: grid snippet missing chip family"
        assert f'`{capability["installSlug"]}`' in grid, f"{stem}: grid snippet missing install slug"
        relay_text = "No built-in relays" if capability["relays"] == 0 else f"{capability['relays']} built-in relay"
        assert relay_text in grid, f"{stem}: grid snippet missing relay availability"
        ethernet_text = "Yes, manual ESPHome install only" if capability["ethernetManualInstall"] else "No"
        assert ethernet_text in grid, f"{stem}: grid snippet missing Ethernet support"
        assert f'slug="{capability["installSlug"]}"' in install, f"{stem}: install snippet missing slug"


def test_generated_web(profile_slugs: list[str]) -> None:
    for slug in profile_slugs:
        path = WEB_OUTPUT_DIR / slug / "www.js"
        assert path.is_file(), f"{slug}: generated web bundle is missing"
        text = path.read_text(encoding="utf-8")
        assert slug in text, f"{slug}: generated web bundle has wrong device id"


def test_generated_yaml(profiles: dict[str, dict]) -> None:
    for slug, profile in profiles.items():
        package_path = ROOT / "devices" / slug / "packages.yaml"
        sensor_path = ROOT / "devices" / slug / "device" / "sensors.yaml"
        package = package_path.read_text(encoding="utf-8")
        sensors = sensor_path.read_text(encoding="utf-8")
        assert f'device_slug: "{slug}"' in package, f"{slug}: packages.yaml missing device slug"
        assert f'firmware_manifest_slug: "{slug}"' in package, f"{slug}: packages.yaml missing manifest slug"
        assert f"cfg.num_slots = {profile['slots']};" in sensors, f"{slug}: sensors.yaml missing slot count"


def test_setup_icon_fonts(profile_slugs: list[str]) -> None:
    for slug in profile_slugs:
        fonts_path = ROOT / "devices" / slug / "device" / "fonts.yaml"
        fonts = fonts_path.read_text(encoding="utf-8")
        for glyph, icon_name in REQUIRED_SETUP_ICON_GLYPHS.items():
            assert glyph in fonts, f"{slug}: setup icon font missing {icon_name} for OTA update screen"


def test_firmware_matrices(profile_slugs: list[str]) -> None:
    profiles = load_device_profiles()
    release = device_matrix.release_matrix(profiles)
    nightly = device_matrix.nightly_matrix(profiles)
    pr = device_matrix.pr_matrix(profiles)
    assert_profile_slugs(profile_slugs, [entry["slug"] for entry in release["include"]], "release matrix")
    assert_profile_slugs(profile_slugs, [entry["slug"] for entry in nightly["include"]], "nightly matrix")
    assert_profile_slugs(profile_slugs, [entry["slug"] for entry in pr["include"]], "PR matrix")


def test_public_firmware_slugs(profile_slugs: list[str]) -> None:
    assert sorted(profile_slugs) == check_public_firmware.load_slugs(ROOT / "devices" / "manifest.json")


def main() -> int:
    profiles = load_device_profiles()
    profile_slugs = list(profiles.keys())
    assert profile_slugs == compatibility_required_slugs(), "current compatibility device slug fixture is stale"
    test_public_device_capabilities(profile_slugs)
    test_generated_web(profile_slugs)
    test_generated_yaml(profiles)
    test_setup_icon_fonts(profile_slugs)
    test_firmware_matrices(profile_slugs)
    test_public_firmware_slugs(profile_slugs)
    print("Device profile cross-checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
