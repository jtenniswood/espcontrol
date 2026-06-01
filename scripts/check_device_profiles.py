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
    assert "inline void epaper_dashboard_set_order(const std::string &order_str)" in epaper, (
        "TRMNL must read the normal web editor button order instead of using fixed slot order"
    )
    assert "constexpr int EPAPER_DASHBOARD_PAGE_SLOTS = 12;" in epaper, (
        "TRMNL physical dashboard must match the normal 4x3 web editor slot count"
    )
    assert "constexpr int EPAPER_DASHBOARD_PAGES = 1;" in epaper, (
        "TRMNL must use the normal single dashboard instead of hidden e-paper-only pages"
    )
    assert "LV_GRID_ALIGN_STRETCH, col, col_span," in epaper and "LV_GRID_ALIGN_STRETCH, row, row_span" in epaper, (
        "TRMNL must apply normal wide/tall/large card spans on the physical e-paper grid"
    )
    assert "row_span > 1 ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_DOT" in epaper, (
        "TRMNL tall cards must wrap labels like normal device cards"
    )
    assert 'if (tile.type == "action") {\n    return epaper_dashboard_option_select_card(tile) ? tile.entity : "";' in epaper, (
        "TRMNL action cards must not subscribe to their action target unless they are option-select cards"
    )
    assert 'if (tile.type == "push" || tile.type == "webhook") return "";' in epaper, (
        "TRMNL push and webhook cards must stay static like normal command cards"
    )
    assert 'if (tile.type == "internal") return "";' in epaper, (
        "TRMNL internal relay cards must not subscribe to their relay key as a Home Assistant entity"
    )
    assert "std::string state_source = epaper_dashboard_state_source(tile);" in epaper, (
        "TRMNL state subscriptions must use card-aware state sources"
    )
    assert 'if (tile.type == "door_window" && !tile.label_configured) {\n    if (!tile.friendly_name.empty()) return tile.friendly_name;' in epaper, (
        "TRMNL door/window cards must use HA friendly names like normal cards when no custom label is set"
    )
    assert 'if (tile.type == "presence" && !tile.label_configured) {\n    if (!tile.friendly_name.empty()) return tile.friendly_name;' in epaper, (
        "TRMNL presence cards must use HA friendly names like normal cards when no custom label is set"
    )
    assert 'if (tile.type == "cover" &&\n      (tile.sensor == "toggle" || epaper_dashboard_cover_command_mode(tile.sensor)) &&\n      !tile.label_configured) {\n    if (!tile.friendly_name.empty()) return tile.friendly_name;' in epaper, (
        "TRMNL cover command/toggle cards must use HA friendly names like normal cards when no custom label is set"
    )
    assert 'if (tile.type == "alarm" && !tile.entity.empty()) return tile.entity;' in epaper, (
        "TRMNL alarm name-label cards must use HA friendly names like normal cards when no custom label is set"
    )
    assert 'if (tile.type == "climate") return "";' in epaper, (
        "TRMNL climate cards must keep the normal default Climate label instead of substituting HA friendly names"
    )
    assert "espcontrol::epaper_dashboard_set_order(id(button_order).state);" in (
        ROOT / "devices" / "trmnl-75-og" / "device" / "sensors.yaml"
    ).read_text(encoding="utf-8"), (
        "TRMNL dashboard sync must pass the web editor button_order into the e-paper renderer"
    )
    assert (
        'if ((tile.type == "fan_switch" || tile.type == "fan_oscillate") &&\n'
        '        !tile.label_configured && !tile.friendly_name.empty()) {\n'
        '      return tile.friendly_name;\n'
        '    }'
        in epaper
    ), "TRMNL fan switch/oscillation cards must use HA friendly names like normal cards when no custom label is set"
    assert "return epaper_dashboard_trim(tile.unit);" in epaper, (
        "TRMNL card units must be trimmed the same way normal LVGL cards trim unit labels"
    )
    assert epaper.count('if (tile.type == "internal") return "";') >= 2, (
        "TRMNL internal relay cards must not derive labels from the mode or subscribe to the relay key"
    )
    assert 'if (!tile.entity.empty()) return epaper_dashboard_sentence_cap_text(tile.entity);' in epaper, (
        "TRMNL internal relay fallback labels must match normal internal relay cards"
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
    assert 'if (end == value.c_str() || std::isnan(parsed) || parsed < 0) return "0:00";' in epaper, (
        "TRMNL media position cards must fall back to 0:00 for invalid positions like normal LVGL media cards"
    )
    assert 'if (end == tile.sensor_value.c_str() || std::isnan(parsed)) {\n    return "--";' in epaper, (
        "TRMNL media volume cards must not show arbitrary non-numeric text in the volume value"
    )
    assert 'return tile.sensor == "position" ? "0:00" : "--";' in epaper, (
        "TRMNL media cards must use the same empty placeholders as normal media cards"
    )
    assert (
        'if (tile.type == "sensor" || epaper_dashboard_toggle_numeric_sensor_card(tile) ||\n'
        '        !epaper_dashboard_sensor_source(tile).empty()) {\n'
        '      return "--";\n'
        '    }'
        in epaper
    ), "TRMNL sensor cards must use normal -- placeholders while waiting for values"
    fan_active = (
        'if (tile.type == "fan_oscillate") {\n'
        '    bool oscillating = false;\n'
        '    return !tile.state_unavailable &&\n'
        '           epaper_dashboard_bool_value(tile.sensor_value, oscillating) && oscillating;\n'
        '  }\n'
        '  if (tile.type == "fan_direction") {\n'
        '    return !tile.state_unavailable &&\n'
        '           epaper_dashboard_normalized_state_text(tile.sensor_value) == "reverse";\n'
        '  }\n'
        '  if (tile.type == "fan_preset") {\n'
        '    return !tile.state_unavailable &&\n'
        '           !tile.fan_preset_modes.empty() &&\n'
        '           epaper_dashboard_fan_preset_active(tile.sensor_value);\n'
        '  }'
    )
    assert fan_active in epaper, (
        "TRMNL fan attribute cards must not show active styling from stale attributes when the fan is unavailable"
    )
    assert (
        'if (epaper_dashboard_cover_command_mode(tile.sensor)) {\n'
        '      if (!tile.icon.empty() && tile.icon != "Auto") return find_icon(tile.icon.c_str());\n'
        '      return find_icon("Blinds");\n'
        '    }'
        in epaper
    ), "TRMNL cover command cards must use the same default/off icon as normal cover command cards"
    cover_icon = (
        'float cover_position = 0.0f;\n'
        '    bool open_icon = epaper_dashboard_cover_toggle_mode(tile.sensor)\n'
        '      ? epaper_dashboard_garage_state_uses_open_icon(tile.state)\n'
        '      : (epaper_dashboard_parse_float_value(tile.sensor_value, cover_position)\n'
        '           ? epaper_dashboard_clamp_percent(static_cast<int>(cover_position + 0.5f)) > 0\n'
        '           : (!tile.state_unavailable && epaper_dashboard_state_active(tile.state)));'
    )
    assert cover_icon in epaper, (
        "TRMNL cover icons must follow normal cover state/position icon selection"
    )
    garage_icon = (
        'bool open_icon = epaper_dashboard_garage_state_uses_open_icon(tile.state);\n'
        '    std::string garage_icon = open_icon && !tile.icon_on.empty() && tile.icon_on != "Auto"\n'
        '      ? tile.icon_on\n'
        '      : tile.icon;'
    )
    assert garage_icon in epaper, (
        "TRMNL garage state cards must choose open icons from open/opening state, not generic active styling"
    )
    calendar_source = (
        'if (!tile.state_unavailable && epaper_dashboard_parse_calendar_date(tile.state, day, month)) {\n'
        '    return true;\n'
        '  }\n'
        '  if (!tile.entity.empty()) return false;'
    )
    assert calendar_source in epaper, (
        "TRMNL calendar cards with a configured date source must not silently fall back to the local date"
    )
    assert 'if (tile.type == "light_brightness" || tile.type == "slider") return find_icon("Tune Vertical Variant");' in epaper, (
        "TRMNL light brightness and slider cards must use the same preview badge as the web editor"
    )
    assert 'if (tile.type == "cover") return find_icon("Blinds Horizontal");' in epaper, (
        "TRMNL cover cards must use the same preview badge as the web editor"
    )
    assert 'if (tile.type == "media" &&\n      (tile.sensor == "play_pause" || tile.sensor == "position") &&\n      tile.precision == "state") {' in epaper, (
        "TRMNL media state-display cards must put the playback state in the label like normal cards"
    )
    assert 'if (epaper_dashboard_action_state_icon_card(tile) ||\n        epaper_dashboard_action_state_text_card(tile)) show_value = false;' in epaper, (
        "TRMNL action icon/text state cards must keep the icon/label presentation used by normal cards"
    )
    assert 'if (epaper_dashboard_action_state_numeric_card(tile)) return "--";' in epaper, (
        "TRMNL action numeric state cards must use the normal numeric placeholder while waiting for values"
    )
    assert (
        'std::string state = epaper_dashboard_normalized_state_text(value);\n'
        '  return state.empty() || state == "unavailable" || state == "unknown";'
        in epaper
    ), "TRMNL unavailable-state detection must match normal cards for blank, unknown, and unavailable states"


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
