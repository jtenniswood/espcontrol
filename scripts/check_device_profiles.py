#!/usr/bin/env python3
"""Cross-check generated device profile outputs against devices/manifest.json."""

from __future__ import annotations

import json
import re
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
        if capability.get("subpages", True):
            assert "[Subpage](/features/subpages)" in grid, f"{stem}: grid snippet missing subpage support"
        else:
            assert "Touch subpages are not available" in grid, f"{stem}: grid snippet missing no-subpage note"
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
        device_path = ROOT / "devices" / slug / "device" / "device.yaml"
        lvgl_path = ROOT / "devices" / slug / "device" / "lvgl.yaml"
        sensor_path = ROOT / "devices" / slug / "device" / "sensors.yaml"
        package = package_path.read_text(encoding="utf-8")
        device = device_path.read_text(encoding="utf-8")
        sensors = sensor_path.read_text(encoding="utf-8")
        assert f'device_slug: "{slug}"' in package, f"{slug}: packages.yaml missing device slug"
        assert f'firmware_manifest_slug: "{slug}"' in package, f"{slug}: packages.yaml missing manifest slug"
        if (profile["firmware"].get("display") or {}).get("mode") == "monochrome":
            assert "epaper_dashboard_set_config" in sensors, f"{slug}: sensors.yaml missing e-paper dashboard config"
            assert lvgl_path.is_file(), f"{slug}: LVGL page definition is missing"
            lvgl = lvgl_path.read_text(encoding="utf-8")
            assert "lvgl:" in lvgl, f"{slug}: LVGL page definition missing lvgl root"
            assert "main_page" in lvgl, f"{slug}: LVGL page definition missing dashboard page"
            assert "trmnl_wifi_setup_page" in lvgl, f"{slug}: LVGL page definition missing WiFi setup page"
            assert "displays: epaper" in device, f"{slug}: device.yaml does not bind LVGL to e-paper display"
            display_block = device.split("display:", 1)[1].split("\nlvgl:", 1)[0]
            assert "lambda: |-" not in display_block, f"{slug}: e-paper display still uses direct drawing lambda"
            assert "epaper_dashboard_render" not in device, f"{slug}: device.yaml still references direct renderer"
        else:
            assert f"cfg.num_slots = {profile['slots']};" in sensors, f"{slug}: sensors.yaml missing slot count"


def test_setup_icon_glyphs() -> None:
    glyphs = (ROOT / "common" / "assets" / "icon_glyphs.yaml").read_text(encoding="utf-8")
    for glyph, icon_name in REQUIRED_SETUP_ICON_GLYPHS.items():
        assert glyph in glyphs, f"shared icon font missing {icon_name} for OTA update screen"


def test_trmnl_epaper_icon_literals() -> None:
    icons = (ROOT / "components" / "espcontrol" / "icons.h").read_text(encoding="utf-8")
    icon_names = set(re.findall(r'\{"([^"]+)",\s+"\\U[0-9A-Fa-f]+"\}', icons))
    epaper = (ROOT / "components" / "espcontrol_epaper" / "epaper_dashboard.h").read_text(encoding="utf-8")
    missing = sorted({
        name for name in re.findall(r'find_icon\("([^"]+)"\)', epaper)
        if name != "Auto" and name not in icon_names
    })
    assert not missing, f"TRMNL e-paper renderer references missing icon names: {', '.join(missing)}"

    glyphs = (ROOT / "common" / "assets" / "icon_glyphs.yaml").read_text(encoding="utf-8")
    trmnl_yaml = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "devices" / "trmnl-75-og" / "device").glob("*.yaml")
    )
    missing_glyphs = sorted({
        glyph for glyph in re.findall(r'\\U[0-9A-Fa-f]{8}', trmnl_yaml)
        if f'"{glyph.upper()}"' not in glyphs
    })
    assert not missing_glyphs, f"TRMNL hard-coded icon glyphs missing from icon font: {', '.join(missing_glyphs)}"


def test_trmnl_epaper_card_parity_guards() -> None:
    epaper = (ROOT / "components" / "espcontrol_epaper" / "epaper_dashboard.h").read_text(encoding="utf-8")
    assert "bool show_track = configured && epaper_dashboard_slider_visual_card(tile);" in epaper, (
        "TRMNL media now-playing play/pause cards must keep their track/background visible"
    )
    assert (
        'if (tile.type == "fan_switch" || tile.type == "fan_oscillate") return "";'
        in epaper
    ), "TRMNL fan switch/oscillation cards must keep normal default labels instead of HA friendly names"
    assert "return epaper_dashboard_trim(tile.unit);" in epaper, (
        "TRMNL card units must be trimmed the same way normal LVGL cards trim unit labels"
    )
    garage_status = (
        'if (tile.type == "garage" &&\n'
        '      epaper_dashboard_option_value(tile.options, "label_display") == "status") {\n'
        '    if (tile.state_unavailable || tile.state.empty()) return "--";\n'
        '    return epaper_dashboard_pretty_state(tile.state);\n'
        '  }\n'
        '  if (tile.type == "garage" && !epaper_dashboard_garage_command_mode(tile.sensor)'
    )
    assert garage_status in epaper, (
        "TRMNL garage status cards must show the status before falling back to the default label"
    )
    action_text = (
        'if (epaper_dashboard_action_state_text_card(tile)) {\n'
        '    if (!tile.sensor_value.empty()) return epaper_dashboard_text_sensor_display_text(tile.sensor_value);\n'
        '    if (tile.sensor_unavailable) return "";\n'
        '  }'
    )
    assert action_text in epaper, (
        "TRMNL action text-state cards must show the raw text state like normal LVGL action cards"
    )
    assert 'if (end == value.c_str() || std::isnan(parsed)) return "";' in epaper, (
        "TRMNL numeric cards must leave non-numeric values blank like normal LVGL numeric cards"
    )


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
    test_setup_icon_glyphs()
    test_trmnl_epaper_icon_literals()
    test_trmnl_epaper_card_parity_guards()
    test_firmware_matrices(profile_slugs)
    test_public_firmware_slugs(profile_slugs)
    print("Device profile cross-checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
