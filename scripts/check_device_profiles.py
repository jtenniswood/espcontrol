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


def test_trmnl_epaper_card_text_sizing() -> None:
    """Keep the TRMNL physical card text and generated preview sizing in step."""
    manifest = read_json(ROOT / "devices" / "manifest.json")
    trmnl = manifest["devices"]["trmnl-75-og"]
    btn = trmnl["web"]["layout"]["btn"]
    assert btn["valueSize"] >= 16.8, "TRMNL web preview sensor values regressed to smaller text"
    assert btn["labelSize"] >= 9, "TRMNL web preview card labels regressed to smaller text"

    tile = (ROOT / "devices" / "trmnl-75-og" / "device" / "trmnl_tile_widget.yaml").read_text(
        encoding="utf-8"
    )
    assert "id: trmnl_tile_${num}_value" in tile and "transform_scale: 2.8" in tile, (
        "TRMNL physical sensor value text regressed to a smaller scale"
    )
    assert "id: trmnl_tile_${num}_label" in tile and "transform_scale: 3.0" in tile, (
        "TRMNL physical card label text regressed to a smaller scale"
    )


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
    normal_config = (ROOT / "components" / "espcontrol" / "button_grid_config.h").read_text(encoding="utf-8")
    web_card_types = sorted({
        match.group(1)
        for path in (ROOT / "src" / "webserver" / "types").glob("*.js")
        for match in re.finditer(r'registerButtonType\("([^"]*)"', path.read_text(encoding="utf-8"))
    })
    epaper_card_type_markers = {
        "": "tile.type.empty()",
        "option_select": 'tile.type == "option_select"',
    }
    missing_epaper_card_types = [
        card_type
        for card_type in web_card_types
        if epaper_card_type_markers.get(card_type, f'tile.type == "{card_type}"') not in epaper
    ]
    assert not missing_epaper_card_types, (
        "TRMNL e-paper renderer lacks explicit handling for web card types: "
        + ", ".join(missing_epaper_card_types)
    )
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
    assert 'if (tile.type == "door_window" && !tile.label_configured) {\n    if (!tile.friendly_name.empty()) return tile.friendly_name;\n    return epaper_dashboard_window_card(tile) ? "Window" : "Door";' in epaper, (
        "TRMNL door/window cards must use HA friendly names before falling back to the normal/web default label"
    )
    assert 'if (tile.type == "presence" && !tile.label_configured) {\n    if (!tile.friendly_name.empty()) return tile.friendly_name;\n    return "Presence";' in epaper, (
        "TRMNL presence cards must use HA friendly names before falling back to the normal/web default label"
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
    assert (
        'if (tile.type == "alarm" &&\n'
        '      epaper_dashboard_option_value(tile.options, "label_display") != "name") {\n'
        '    if (tile.state.empty()) return "--";\n'
        '    return epaper_dashboard_alarm_label_for_state(tile.state);\n'
        '  }'
        in epaper
    ), "TRMNL alarm status labels must use the normal initial placeholder and raw state labels"
    assert (
        'if (tile.type == "weather" && !epaper_dashboard_weather_forecast_card(tile)) {\n'
        '    if (tile.state.empty()) return find_icon("Weather Cloudy");\n'
        '    return epaper_dashboard_weather_icon_for_state(tile.state);\n'
        '  }'
        in epaper
    ), "TRMNL current weather cards must use the normal/web cloudy startup icon before state arrives"
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
        '    if (tile.state.empty()) return "--";\n'
        '    return epaper_dashboard_pretty_state(tile.state);\n'
        '  }\n'
        '  if (tile.type == "garage" && !epaper_dashboard_garage_command_mode(tile.sensor) &&\n'
        '      !tile.state.empty() && !epaper_dashboard_garage_state_releases_label(tile.state)) {\n'
        '    return epaper_dashboard_pretty_state(tile.state);\n'
        '  }\n'
        '  if (tile.type == "garage" && !epaper_dashboard_garage_command_mode(tile.sensor)'
    )
    assert garage_status in epaper, (
        "TRMNL garage status cards must show transition/error statuses like normal cards"
    )
    assert (
        'inline bool epaper_dashboard_lock_state_releases_label(const std::string &state) {\n'
        '  return state == "locked" || state == "unlocked" || state == "open";\n'
        '}' in epaper and
        'if (tile.type == "lock" && !epaper_dashboard_lock_command_mode(tile.sensor) &&\n'
        '      !tile.state.empty() && !epaper_dashboard_lock_state_releases_label(tile.state)) {\n'
        '    return epaper_dashboard_pretty_state(tile.state);\n'
        '  }' in epaper
    ), "TRMNL lock status cards must show transition/error statuses like normal cards"
    assert (
        'if (tile.type == "lock") {\n'
        '    if (epaper_dashboard_lock_command_mode(tile.sensor)) {\n'
        '      if (!tile.icon.empty() && tile.icon != "Auto") return find_icon(tile.icon.c_str());\n'
        '      return find_icon(tile.sensor == "unlock" ? "Lock Open" : "Lock");\n'
        '    }\n'
        '    std::string lock_icon = active && !tile.icon_on.empty() && tile.icon_on != "Auto"\n'
        '      ? tile.icon_on\n'
        '      : tile.icon;\n'
        '    if (!lock_icon.empty() && lock_icon != "Auto") return find_icon(lock_icon.c_str());\n'
        '    return find_icon(active ? "Lock Open" : "Lock");\n'
        '  }'
        in epaper
    ), "TRMNL lock cards must use configured command and locked/unlocked icons like normal cards"
    assert (
        'inline bool epaper_dashboard_garage_state_releases_label(const std::string &state) {\n'
        '  return state == "open" || state == "closed";\n'
        '}' in epaper
    ), "TRMNL garage label release states must match normal cards"
    action_text = (
        'if (epaper_dashboard_action_state_text_card(tile)) {\n'
        '    if (!tile.sensor_value.empty()) {\n'
        '      return epaper_dashboard_text_sensor_display_text(\n'
        '          tile.sensor_value, EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN);\n'
        '    }\n'
        '    if (tile.sensor_unavailable) return "Unavailable";\n'
        '  }'
    )
    assert action_text in epaper, (
        "TRMNL action text-state cards must use the normal action text display limit and avoid blank unavailable labels"
    )
    assert (
        "constexpr size_t EPAPER_DASHBOARD_TEXT_SENSOR_STATE_MAX_LEN = 256;" in epaper and
        "size_t max_len = EPAPER_DASHBOARD_TEXT_SENSOR_STATE_MAX_LEN" in epaper and
        "if (ch == '\\r' && i + 1 < len && value[i + 1] == '\\n') continue;" in epaper
    ), "TRMNL text sensor cards must use the normal text sensor display limit"
    assert (
        'if (epaper_dashboard_toggle_text_sensor_card(tile) &&\n'
        '      epaper_dashboard_state_active(tile.state)) {\n'
        '    if (!tile.sensor_value.empty()) {\n'
        '      return epaper_dashboard_text_sensor_display_text(tile.sensor_value);\n'
        '    }\n'
        '    return "--";\n'
        '  }'
        in epaper
    ), "TRMNL switch text-state cards must show the normal -- placeholder while waiting for the text sensor"
    assert (
        "constexpr size_t EPAPER_DASHBOARD_SHORT_STATE_MAX_LEN = 32;" in epaper and
        "inline size_t epaper_dashboard_state_text_max_len(const EpaperDashboardTile &tile)" in epaper and
        "tile.state = epaper_dashboard_string_ref_limited(\n"
        "              state, epaper_dashboard_state_text_max_len(tile));" in epaper
    ), "TRMNL entity state cards must use card-aware normal state display limits"
    assert epaper.count(
        "? epaper_dashboard_string_ref_limited(state, EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN)"
    ) >= 2, "TRMNL media now-playing title and artist must use the normal metadata text limit"
    assert 'if (end == value.c_str() || std::isnan(parsed)) return "";' in epaper, (
        "TRMNL numeric cards must leave non-numeric values blank like normal LVGL numeric cards"
    )
    assert (
        'if ((tile.type == "sensor" || epaper_dashboard_toggle_numeric_sensor_card(tile)) &&\n'
        '      tile.precision != "text" && tile.sensor_unavailable) {\n'
        '    return "";\n'
        '  }'
        in epaper
    ), "TRMNL unavailable numeric sensor cards must hide units like normal LVGL cards"
    assert (
        'if (tile.type == "climate" && epaper_dashboard_value_replaces_icon(tile) && tile.unit.empty()) {\n'
        '    if (epaper_dashboard_climate_card_value(tile) == "--") return "";\n'
        '    return display_temperature_unit_symbol();\n'
        '  }'
        in epaper
    ), "TRMNL climate temperature cards must use the normal full temperature unit only when a value is shown"
    assert (
        'std::string unit = tile.unit.empty()\n'
        '        ? std::string(display_temperature_unit_symbol())\n'
        '        : epaper_dashboard_trim(tile.unit);\n'
        '      return value + unit;'
        in epaper
    ), "TRMNL climate actual/target labels must include the temperature unit like normal cards and the web preview"
    assert (
        'inline std::string epaper_dashboard_climate_actual_value(const EpaperDashboardTile &tile) {\n'
        '  if (tile.state_unavailable) return "--";' in epaper and
        'inline std::string epaper_dashboard_climate_target_value(const EpaperDashboardTile &tile) {\n'
        '  if (tile.state_unavailable) return "--";' in epaper
    ), "TRMNL climate cards must not show cached temperatures when the climate entity is unavailable"
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
        "uint32_t media_position_received_ms = 0;" in epaper and
        "position += (esphome::millis() - tile.media_position_received_ms) / 1000.0f;" in epaper and
        "tile.media_position_received_ms = esphome::millis();" in epaper
    ), "TRMNL media progress cards must advance from receive time when HA omits media_position_updated_at"
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
        '           epaper_dashboard_fan_preset_active(tile.sensor_value);\n'
        '  }'
    )
    assert fan_active in epaper, (
        "TRMNL fan attribute cards must match normal active styling from state and preset attributes"
    )
    assert (
        'inline const char *epaper_dashboard_fan_icon_name(const EpaperDashboardTile &tile,\n'
        '                                                  bool active) {\n'
        '  if (tile.type == "fan_switch" && active) {\n'
        '    if (!tile.icon_on.empty() && tile.icon_on != "Auto") return tile.icon_on.c_str();\n'
        '    return "Fan";\n'
        '  }\n'
        '  if (!tile.icon.empty() && tile.icon != "Auto") return tile.icon.c_str();\n'
        '  return epaper_dashboard_fan_default_icon_name(tile);\n'
        '}'
        in epaper and
        'tile.type == "fan_preset") return find_icon(epaper_dashboard_fan_icon_name(tile, active));'
        in epaper
    ), "TRMNL fan cards must honor configured icons and fan-switch on icons like normal cards"
    assert (
        'std::string icon = active && !tile.icon_on.empty() && tile.icon_on != "Auto" ? tile.icon_on : tile.icon;'
        in epaper and
        'if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());'
        in epaper
    ), "TRMNL icon-style cards must honor configured off/on icons before using default icons"
    assert (
        'if (tile.type == "light_brightness" || tile.type == "light_switch") {\n'
        '    if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());\n'
        '    return find_icon(active ? "Lightbulb" : "Lightbulb Outline");\n'
        '  }\n'
        '  if (tile.type == "light_temperature") {\n'
        '    if (!tile.icon.empty() && tile.icon != "Auto") return find_icon(tile.icon.c_str());\n'
        '    return find_icon("Lightbulb");\n'
        '  }'
        in epaper
    ), "TRMNL light cards must honor configured icons like normal light cards"
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
    assert "return 100 - epaper_dashboard_percent_value(tile.sensor_value);" in epaper, (
        "TRMNL cover position tracks must invert fill direction like normal cover sliders"
    )
    assert (
        'inline const std::string &epaper_dashboard_binary_card_state(const EpaperDashboardTile &tile) {\n'
        '  return !tile.state.empty() ? tile.state : tile.sensor_value;\n'
        '}'
        in epaper and
        'icon_active = !epaper_dashboard_binary_card_unavailable(tile) &&\n'
        '                    epaper_dashboard_state_active(epaper_dashboard_binary_card_state(tile));'
        in epaper and
        'const std::string &state = epaper_dashboard_binary_card_state(tile);\n'
        '      icon_active = !epaper_dashboard_binary_card_unavailable(tile) &&\n'
        '                    (epaper_dashboard_normalized_state_text(state) == "detected" ||'
        in epaper
    ), "TRMNL door/window and presence cards must render from their sensor-backed state"
    assert (
        'if (tile.type == "sensor" || tile.type == "door_window" || tile.type == "presence") {\n'
        '    return !tile.sensor.empty();\n'
        '  }'
        in epaper
    ), "TRMNL sensor-style cards must stay hidden until their required sensor entity is configured"
    assert (
        'if (tile.type == "door_window" && !tile.label_configured) {\n'
        '    if (!tile.friendly_name.empty()) return tile.friendly_name;\n'
        '    return epaper_dashboard_window_card(tile) ? "Window" : "Door";\n'
        '  }\n'
        '  if (tile.type == "presence" && !tile.label_configured) {\n'
        '    if (!tile.friendly_name.empty()) return tile.friendly_name;\n'
        '    return "Presence";\n'
        '  }'
        in epaper
    ), "TRMNL door/window and presence labels must use HA friendly names like normal cards"
    assert (
        'if (tile.type == "door_window") {\n'
        '    std::string closed_icon = !tile.icon.empty() && tile.icon != "Auto"\n'
        '      ? tile.icon\n'
        '      : (epaper_dashboard_window_card(tile) ? "Window Closed" : "Door");\n'
        '    std::string open_icon = !tile.icon_on.empty() && tile.icon_on != "Auto"\n'
        '      ? tile.icon_on\n'
        '      : (epaper_dashboard_window_card(tile) ? "Window Open" : "Door Open");\n'
        '    return find_icon((active ? open_icon : closed_icon).c_str());\n'
        '  }\n'
        '  if (tile.type == "presence") {\n'
        '    std::string clear_icon = !tile.icon.empty() && tile.icon != "Auto"\n'
        '      ? tile.icon\n'
        '      : "Motion Sensor Off";\n'
        '    std::string detected_icon = !tile.icon_on.empty() && tile.icon_on != "Auto"\n'
        '      ? tile.icon_on\n'
        '      : "Motion Sensor";\n'
        '    return find_icon((active ? detected_icon : clear_icon).c_str());\n'
        '  }\n'
        '  if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());'
        in epaper
    ), "TRMNL door/window and presence icons must use configured inactive/active icon pairs before generic icons"
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
    assert (
        'return tile.type == "clock" &&\n'
        '         epaper_dashboard_card_large_numbers(tile) &&\n'
        '         row_span == 1 && col_span == 2;'
        in epaper
    ), "TRMNL wide large-number date/time layout must only apply to Clock cards like the web editor and normal devices"
    assert '(tile.type == "calendar" && tile.precision == "datetime")' not in epaper, (
        "TRMNL Time & Date cards must not use Clock-only wide large-number handling"
    )
    epaper_badge_guards = {
        'if (tile.type.empty()) {\n    if (!tile.sensor.empty()) {\n      return tile.precision == "text" ? find_icon("Format Text") : find_icon("Gauge");\n    }\n    return find_icon("Toggle Switch Variant Off");\n  }': (
            "TRMNL switch cards must use the same switch/numeric/text badges as the web editor"
        ),
        'if (tile.type == "sensor") {\n    if (tile.precision == "icon") return find_icon("Toggle Switch");\n    if (tile.precision == "text") return find_icon("Format Text");\n    return find_icon("Gauge");\n  }': (
            "TRMNL sensor cards must use the same icon/numeric/text badges as the web editor"
        ),
        'if (tile.type == "door_window") return find_icon(tile.precision == "window" ? "Window Closed" : "Door");': (
            "TRMNL door/window cards must use the same subtype badges as the web editor"
        ),
        'if (tile.type == "presence") return find_icon("Motion Sensor");': (
            "TRMNL presence cards must use the same badge as the web editor"
        ),
        'if (tile.type == "weather") {\n    if (epaper_dashboard_weather_forecast_card(tile)) return find_icon("Weather Partly Cloudy");\n    return find_icon("Weather Cloudy");\n  }': (
            "TRMNL current and forecast weather cards must use the same badges as the web editor"
        ),
        'if (tile.type == "weather_forecast") return find_icon("Weather Partly Cloudy");': (
            "TRMNL weather forecast cards must use the same badge as the web editor"
        ),
        'if (tile.type == "calendar") return find_icon("Calendar Month");': (
            "TRMNL calendar cards must use the same badge as the web editor"
        ),
        'if (tile.type == "timezone") return find_icon("Map Clock");': (
            "TRMNL timezone cards must use the same badge as the web editor"
        ),
        'if (tile.type == "media") return find_icon("Speaker");': (
            "TRMNL media cards must use the same badge as the web editor"
        ),
        'if (tile.type == "climate") return find_icon("Thermostat");': (
            "TRMNL climate cards must use the same badge as the web editor"
        ),
        'if (tile.type == "garage") return find_icon("Garage");': (
            "TRMNL garage cards must use the same badge as the web editor"
        ),
        'if (tile.type == "lock") return find_icon("Lock");': (
            "TRMNL lock cards must use the same badge as the web editor"
        ),
        'if (tile.type == "push") return find_icon("Gesture Tap");': (
            "TRMNL push cards must use the same badge as the web editor"
        ),
        'if (tile.type == "webhook") return find_icon("Webhook");': (
            "TRMNL webhook cards must use the same badge as the web editor"
        ),
        'if (tile.type == "internal") {\n    return find_icon(epaper_dashboard_internal_push_mode(tile) ? "Gesture Tap" : "Power Plug");\n  }': (
            "TRMNL internal cards must use the same switch/push badges as the web editor"
        ),
        'if (tile.type == "light_brightness" || tile.type == "slider") return find_icon("Tune Vertical Variant");': (
            "TRMNL light brightness and slider cards must use the same preview badge as the web editor"
        ),
        'if (tile.type == "light_switch" || tile.type == "light_temperature") return find_icon("Lightbulb");': (
            "TRMNL light switch and temperature cards must use the same badge as the web editor"
        ),
        'if (tile.type == "fan_speed") return find_icon("Fan Speed 2");': (
            "TRMNL fan speed cards must use the same badge as the web editor"
        ),
        'if (tile.type == "fan_switch") return find_icon("Fan");': (
            "TRMNL fan switch cards must use the same badge as the web editor"
        ),
        'if (tile.type == "fan_oscillate") return find_icon("Sync");': (
            "TRMNL fan oscillation cards must use the same badge as the web editor"
        ),
        'if (tile.type == "fan_direction") return find_icon("Swap Horizontal");': (
            "TRMNL fan direction cards must use the same badge as the web editor"
        ),
        'if (tile.type == "fan_preset") return find_icon("Fan Auto");': (
            "TRMNL fan preset cards must use the same badge as the web editor"
        ),
        'if (tile.type == "cover") return find_icon("Blinds Horizontal");': (
            "TRMNL cover cards must use the same preview badge as the web editor"
        ),
    }
    for snippet, message in epaper_badge_guards.items():
        assert snippet in epaper, message
    assert (
        'if (tile.type == "cover" && epaper_dashboard_cover_toggle_mode(tile.sensor) &&\n'
        '      (tile.state == "opening" || tile.state == "closing")) {\n'
        '    return epaper_dashboard_pretty_state(tile.state);\n'
        '  }' in epaper
    ), "TRMNL cover toggle cards must show movement statuses like normal cards"
    assert 'if (tile.type == "media" &&\n      (tile.sensor == "play_pause" || tile.sensor == "position") &&\n      tile.precision == "state") {' in epaper, (
        "TRMNL media state-display cards must put the playback state in the label like normal cards"
    )
    assert (
        'if (tile.type == "media") {\n'
        '    return !tile.state_unavailable &&\n'
        '           (tile.sensor == "play_pause" ||\n'
        '            (tile.sensor == "now_playing" && tile.precision == "play_pause")) &&\n'
        '           tile.state == "playing";\n'
        '  }' in epaper
    ), "TRMNL media cards must only use active styling for play/pause modes when the state is exactly playing"
    assert 'if (epaper_dashboard_action_state_icon_card(tile) ||\n        epaper_dashboard_action_state_text_card(tile)) show_value = false;' in epaper, (
        "TRMNL action icon/text state cards must keep the icon/label presentation used by normal cards"
    )
    assert 'if (epaper_dashboard_action_state_numeric_card(tile)) return "--";' in epaper, (
        "TRMNL action numeric state cards must use the normal numeric placeholder while waiting for values"
    )
    assert (
        'if (epaper_dashboard_option_select_card(tile)) {\n'
        '    if (tile.state_unavailable) return "--";\n'
        '    if (!tile.state.empty()) return tile.state;\n'
        '    return "--";\n'
        '  }'
        in epaper
    ), "TRMNL option-select cards must put the selected option in the value area like normal cards"
    assert (
        'return epaper_dashboard_option_select_card(tile)\n'
        '           ? EPAPER_DASHBOARD_STATE_TEXT_MAX_LEN\n'
        '           : EPAPER_DASHBOARD_SHORT_STATE_MAX_LEN;'
        in epaper
    ), "TRMNL option-select values must use the normal full state text limit"
    assert (
        'if (epaper_dashboard_option_select_card(tile) && !tile.entity.empty()) return tile.entity;'
        in epaper
    ), "TRMNL option-select cards must use HA friendly names like normal cards when no custom label is set"
    assert (
        'if (epaper_dashboard_option_select_card(tile)) {\n'
        '    return false;\n'
        '  }'
        in epaper
    ), "TRMNL option-select cards must not use selected text as an active/on state"
    assert (
        'if (tile.type == "todo") {\n'
        '    return false;\n'
        '  }'
        in epaper
    ), "TRMNL todo counts must not invert the card when the count is 1"
    assert (
        "#if defined(ESPCONTROL_DISABLE_TODO) && ESPCONTROL_DISABLE_TODO\n"
        "  (void) tile;\n"
        "  return false;\n"
        "#else\n"
        '  return tile.type == "todo" &&' in epaper
    ), "TRMNL must not render disabled Todo cards as live count cards"
    assert (
        "#if defined(ESPCONTROL_DISABLE_TODO) && ESPCONTROL_DISABLE_TODO\n"
        '  if (tile.type == "todo") return "";\n'
        "#endif\n"
        "  return tile.entity;" in epaper
    ), "TRMNL disabled Todo cards must not subscribe to Todo entity state"
    assert (
        "#if defined(ESPCONTROL_DISABLE_TODO) && ESPCONTROL_DISABLE_TODO\n"
        '  if (tile.type == "todo") return "";\n'
        "#else\n"
        '  if (tile.type == "todo" && !tile.entity.empty()) return tile.entity;' in epaper and
        'if (tile.type == "todo" && tile.label.empty()) {\n'
        '#if defined(ESPCONTROL_DISABLE_TODO) && ESPCONTROL_DISABLE_TODO\n'
        '    return "Todo";' in epaper
    ), "TRMNL disabled Todo cards must keep the static normal-device label behavior"
    assert (
        'if (epaper_dashboard_option_select_card(tile)) {\n'
        '    return find_icon("Chevron Down");\n'
        '  }'
        in epaper
    ), "TRMNL option-select cards must keep the web preview chevron badge"
    assert (
        'if (tile.type == "action") {\n'
        '    if (epaper_dashboard_action_state_display_enabled(tile)) {\n'
        '      if (epaper_dashboard_action_state_icon_card(tile)) return find_icon("Toggle Switch");\n'
        '      if (epaper_dashboard_action_state_text_card(tile)) return find_icon("Format Text");\n'
        '      return find_icon("Gauge");\n'
        '    }\n'
        '    return find_icon("Flash");\n'
        '  }'
        in epaper
    ), "TRMNL action state-display cards must show the same state-mode badge as the web preview"
    assert (
        'std::string state = epaper_dashboard_normalized_state_text(value);\n'
        '  return state.empty() || state == "unavailable" || state == "unknown";'
        in epaper
    ), "TRMNL unavailable-state detection must match normal cards for blank, unknown, and unavailable states"
    assert (
        'inline bool epaper_dashboard_entity_accepts_unknown_state(const std::string &entity_id) {\n'
        '  return (entity_id.size() > 7 && entity_id.compare(0, 7, "button.") == 0) ||\n'
        '         (entity_id.size() > 13 && entity_id.compare(0, 13, "input_button.") == 0);\n'
        '}'
        in epaper and
        'tile.sensor_unavailable = !tile.action_state_entity.empty()\n'
        '              ? epaper_dashboard_entity_state_unavailable(\n'
        '                    tile.action_state_entity, tile.sensor_value)\n'
        '              : epaper_dashboard_state_unavailable(tile.sensor_value);'
        in epaper
    ), "TRMNL action state cards must allow unknown button/input_button states like normal cards"
    assert (
        'inline bool epaper_dashboard_state_active(const std::string &value) {\n'
        '  std::string state = epaper_dashboard_normalized_state_text(value);\n'
        '  return state == "on" || state == "true" || state == "1" ||'
        in epaper
    ), "TRMNL active-state detection must normalize state text like normal cards"
    assert (
        'if (epaper_dashboard_brightness_slider_type(tile.type)) {\n'
        '    tile.options.clear();\n'
        '  }'
        in epaper
    ), "TRMNL brightness and slider cards must discard unsupported saved options like normal cards"
    assert (
        'if ((tile.type == "light_brightness" || tile.type == "slider" ||\n'
        '       tile.type == "fan_speed") &&\n'
        '      !tile.state.empty() && !epaper_dashboard_state_active(tile.state)) return 0;'
        in epaper
    ), "TRMNL brightness and fan tracks must accept attribute values before the on/off state arrives"
    assert 'if (tile.type == "fan_speed") return 0;' in epaper, (
        "TRMNL fan speed cards must not show a made-up percentage before Home Assistant sends the fan percentage"
    )
    assert (
        'if (tile.type == "light_temperature" &&\n'
        '      !epaper_dashboard_state_active(tile.state)) return 0;'
        in epaper
        and 'if (tile.type == "light_temperature") return 0;' in epaper
    ), "TRMNL light-temperature tracks must wait for active state and a kelvin attribute like normal cards"
    assert (
        'if (!tile.type.empty() && tile.type != "action" && tile.type != "alarm" &&\n'
        '      tile.type != "alarm_action" && tile.type != "climate" &&\n'
        '      tile.type != "garage" && tile.type != "webhook" && tile.type != "todo" &&\n'
        '      tile.type != "sensor" && tile.type != "door_window" &&\n'
        '      tile.type != "presence" && tile.type != "media" &&\n'
        '      !epaper_dashboard_fan_card_type(tile.type) &&\n'
        '      !epaper_dashboard_card_large_numbers_supported(tile)) {\n'
        '    tile.options.clear();\n'
        '  }'
        in epaper
    ), "TRMNL must run the same final unsupported-options cleanup as normal cards"
    shared_normalization_guards = {
        "media options only keep volume_max for volume cards": (
            'if (mode != "volume" && mode != "position") return "";',
            'if (mode != "volume" && mode != "position") return "";',
        ),
        "switch large numbers require a numeric active-display sensor": (
            'if (!sensor.empty() && precision != "text" && cfg_option_token_present(options, "large_numbers"))',
            'if (!sensor.empty() && precision != "text" &&\n      epaper_dashboard_option_present(options, "large_numbers"))',
        ),
        "sensor text mode keeps state label translations": (
            'if (precision == "text" && cfg_option_token_present(options, SENSOR_STATE_LABELS_OPTION))',
            'if (precision == "text" && epaper_dashboard_option_present(options, "state_labels"))',
        ),
        "weather large numbers only apply to forecast modes": (
            'return (precision == "today" || precision == "tomorrow") &&\n         cfg_option_token_present(options, "large_numbers")',
            'return (precision == "today" || precision == "tomorrow") &&\n         epaper_dashboard_option_present(options, "large_numbers")',
        ),
        "todo large numbers require count display": (
            'if (show_count && cfg_option_token_present(options, "large_numbers"))',
            'if (show_count && epaper_dashboard_option_present(options, "large_numbers"))',
        ),
        "action state options are dropped without a state entity": (
            'std::string state_entity = cfg_option_value(options, "state_entity");\n  if (state_entity.empty()) return "";',
            'std::string state_entity = epaper_dashboard_option_value(options, "state_entity");\n  if (state_entity.empty()) return "";',
        ),
        "climate large numbers are removed in icon mode": (
            'if (number_display != "icon" && cfg_option_token_present(options, "large_numbers"))',
            'if (number_display != "icon" && epaper_dashboard_option_present(options, "large_numbers"))',
        ),
    }
    for rule, (normal_snippet, epaper_snippet) in shared_normalization_guards.items():
        assert normal_snippet in normal_config, f"normal device card normalisation guard missing: {rule}"
        assert epaper_snippet in epaper, f"TRMNL card normalisation guard missing: {rule}"
    assert (
        'if (tile.type.empty()) return !tile.sensor.empty() && tile.precision != "text";\n'
        '  if (tile.type == "action") {\n'
        '    if (epaper_dashboard_option_value(tile.options, "state_entity").empty()) return false;\n'
        '    std::string precision = epaper_dashboard_option_value(tile.options, "state_precision");\n'
        '    return precision == "0" || precision == "1" || precision == "2" ||\n'
        '           !epaper_dashboard_option_value(tile.options, "state_unit").empty();\n'
        '  }\n'
        '  if (tile.type == "media") return tile.sensor == "volume" || tile.sensor == "position";\n'
        '  if (tile.type == "climate") return epaper_dashboard_climate_number_mode(tile) != "icon";\n'
        '  if (tile.type == "todo") return epaper_dashboard_todo_card_show_count(tile);'
        in epaper
    ), "TRMNL large-number support rules must match normal device cards"
    assert (
        'return tile.type == "clock" &&\n'
        '         epaper_dashboard_card_large_numbers(tile) &&\n'
        '         row_span == 1 && col_span == 2;'
        in epaper and
        'lv_align_t value_align = epaper_dashboard_wide_large_date_time_card(tile, row_span, col_span)\n'
        '          ? LV_ALIGN_LEFT_MID' in epaper and
        'if (epaper_dashboard_wide_large_date_time_card(tile, row_span, col_span)) {\n'
        '        lv_obj_add_flag(slot.label, LV_OBJ_FLAG_HIDDEN);'
        in epaper
    ), "TRMNL wide large-number Clock cards must use the normal hidden-label left-middle layout"
    assert (
        "if (tile.label_configured && !tile.label.empty()) return tile.label;" in epaper
    ), "TRMNL weather forecast cards must honor explicitly configured labels like normal cards"
    assert (
        'const char *badge = epaper_dashboard_badge_icon(tile);\n'
        '      lv_obj_set_width(slot.label, badge ? lv_pct(80) : lv_pct(100));'
        in epaper
    ), "TRMNL cards without badges must allow full-width labels like normal cards"


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
