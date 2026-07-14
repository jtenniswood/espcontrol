#!/usr/bin/env python3
"""Release confidence checks for generated public artifacts."""

from __future__ import annotations

import json
from pathlib import Path

import check_public_firmware
import device_matrix
from device_profiles import ROOT, load_device_profiles, public_device_capabilities


WEB_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"
DEVICE_CAPABILITIES_JSON = ROOT / "docs" / "public" / "device-profiles.json"
DEVICE_DOCS_DIR = ROOT / "docs" / "generated" / "screens"
CARD_CONTRACT_JSON = ROOT / "common" / "config" / "card_contract.json"
CARD_CAPABILITIES_DOC = ROOT / "docs" / "generated" / "cards" / "capabilities.md"


def read_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def docs_stem(capability: dict) -> str:
    return capability["docsPath"].rstrip("/").split("/")[-1]


def remove_suffix(value: str, suffix: str) -> str:
    if value.endswith(suffix):
        return value[:-len(suffix)]
    return value


def assert_same_slugs(expected: list[str], actual: list[str], label: str) -> None:
    assert actual == expected, f"{label} slugs differ: {actual} != {expected}"


def assert_unique_public_values(devices: list[dict], key: str, label: str) -> None:
    seen: dict[str, str] = {}
    for device in devices:
        slug = device.get("slug", "<unknown>")
        value = device.get(key)
        assert isinstance(value, str) and value, f"{slug}: public device profile missing {label}"
        assert value not in seen, f"{slug}: {label} {value!r} duplicates {seen[value]}"
        seen[value] = slug


def assert_same_names(expected: list[str], actual: list[str], label: str) -> None:
    assert sorted(actual) == sorted(expected), f"{label} names differ: {sorted(actual)} != {sorted(expected)}"


def test_public_device_profiles(profile_slugs: list[str]) -> list[dict]:
    capabilities = read_json(DEVICE_CAPABILITIES_JSON)
    devices = capabilities["devices"]
    assert_same_slugs(profile_slugs, [device["slug"] for device in devices], "public device profile")
    assert_unique_public_values(devices, "installSlug", "install slug")
    assert_unique_public_values(devices, "docsPath", "docs path")
    return devices


def test_web_bundles(profile_slugs: list[str]) -> None:
    actual_slugs = [
        path.name
        for path in WEB_OUTPUT_DIR.iterdir()
        if path.is_dir()
    ]
    assert_same_names(profile_slugs, actual_slugs, "generated setup page bundle directory")
    for slug in profile_slugs:
        bundle = WEB_OUTPUT_DIR / slug / "www.js"
        assert bundle.is_file(), f"{slug}: generated setup page bundle is missing"
        assert slug in bundle.read_text(encoding="utf-8"), f"{slug}: generated setup page bundle has wrong device id"


def test_firmware_release_matrix(profile_slugs: list[str]) -> None:
    matrices = {
        "release": device_matrix.release_matrix(load_device_profiles()),
        "nightly": device_matrix.nightly_matrix(load_device_profiles()),
        "pull request": device_matrix.pr_matrix(load_device_profiles()),
    }
    for label, matrix in matrices.items():
        assert_same_slugs(profile_slugs, [entry["slug"] for entry in matrix["include"]], f"{label} firmware matrix")


def test_public_firmware_manifest(profile_slugs: list[str]) -> None:
    public_slugs = check_public_firmware.load_slugs(ROOT / "devices" / "manifest.json")
    assert sorted(profile_slugs) == public_slugs, "public firmware slug list differs from device profiles"


def test_generated_device_docs(devices: list[dict]) -> None:
    expected_stems = [docs_stem(capability) for capability in devices]
    actual_grid_stems = sorted(remove_suffix(path.name, "-grid.md") for path in DEVICE_DOCS_DIR.glob("*-grid.md"))
    actual_install_stems = sorted(remove_suffix(path.name, "-install.md") for path in DEVICE_DOCS_DIR.glob("*-install.md"))
    assert_same_names(expected_stems, actual_grid_stems, "generated screen grid doc")
    assert_same_names(expected_stems, actual_install_stems, "generated screen install doc")
    for capability in devices:
        stem = docs_stem(capability)
        grid = DEVICE_DOCS_DIR / f"{stem}-grid.md"
        install = DEVICE_DOCS_DIR / f"{stem}-install.md"
        assert grid.is_file(), f"{stem}: generated grid docs are missing"
        assert install.is_file(), f"{stem}: generated install docs are missing"
        grid_text = grid.read_text(encoding="utf-8")
        install_text = install.read_text(encoding="utf-8")
        for value in (
            capability["slug"],
            capability["installSlug"],
            capability["chipFamily"],
            str(capability["slots"]),
            str(capability["grid"]["rows"]),
            str(capability["grid"]["cols"]),
        ):
            assert value in grid_text or value in install_text, f"{stem}: generated docs missing {value!r}"


def test_generated_card_docs() -> None:
    cards = read_json(CARD_CONTRACT_JSON)["cards"]
    text = CARD_CAPABILITIES_DOC.read_text(encoding="utf-8")
    for card_type, card in cards.items():
        type_name = card_type or "switch"
        assert f"| {card['label']} | {type_name} |" in text, f"{type_name}: generated card docs row is missing"


def main() -> int:
    profiles = load_device_profiles()
    profile_slugs = list(profiles.keys())
    source_devices = public_device_capabilities()["devices"]
    devices = test_public_device_profiles(profile_slugs)
    test_web_bundles(profile_slugs)
    test_firmware_release_matrix(profile_slugs)
    test_public_firmware_manifest(profile_slugs)
    test_generated_device_docs(source_devices)
    test_generated_card_docs()
    print("Release confidence checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
