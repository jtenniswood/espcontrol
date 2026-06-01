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
BUTTON_GRID_CARDS = ROOT / "components" / "espcontrol" / "button_grid_cards.h"
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
        fonts_path = ROOT / "devices" / slug / "device" / "fonts.yaml"
        lvgl_path = ROOT / "devices" / slug / "device" / "lvgl.yaml"
        sensor_path = ROOT / "devices" / slug / "device" / "sensors.yaml"
        tile_path = ROOT / "devices" / slug / "device" / "trmnl_tile_widget.yaml"
        package = package_path.read_text(encoding="utf-8")
        device = device_path.read_text(encoding="utf-8")
        sensors = sensor_path.read_text(encoding="utf-8")
        assert f'device_slug: "{slug}"' in package, f"{slug}: packages.yaml missing device slug"
        assert f'firmware_manifest_slug: "{slug}"' in package, f"{slug}: packages.yaml missing manifest slug"
        if (profile["firmware"].get("display") or {}).get("mode") == "monochrome":
            assert f"cfg.num_slots = {profile['slots']};" in sensors, f"{slug}: sensors.yaml missing shared grid slot count"
            assert "grid_phase1(slots, cfg," in sensors, f"{slug}: sensors.yaml missing shared grid visual setup"
            assert "grid_phase2(slots, cfg," in sensors, f"{slug}: sensors.yaml missing shared grid HA bindings"
            assert lvgl_path.is_file(), f"{slug}: LVGL page definition is missing"
            lvgl = lvgl_path.read_text(encoding="utf-8")
            assert "lvgl:" in lvgl, f"{slug}: LVGL page definition missing lvgl root"
            assert "main_page" in lvgl, f"{slug}: LVGL page definition missing dashboard page"
            assert "trmnl_wifi_setup_page" in lvgl, f"{slug}: LVGL page definition missing WiFi setup page"
            assert "displays: epaper" in device, f"{slug}: device.yaml does not bind LVGL to e-paper display"
            display_block = device.split("display:", 1)[1].split("\nlvgl:", 1)[0]
            assert "lambda: |-" not in display_block, f"{slug}: e-paper display still uses direct drawing lambda"
            assert "espcontrol_epaper" not in device, f"{slug}: device.yaml still uses the separate e-paper renderer"
            assert "epaper_dashboard_" not in device + sensors, f"{slug}: firmware still references e-paper dashboard helpers"
            if slug == "trmnl-75-og":
                assert "model: 7.50inv2p\n" in display_block, f"{slug}: display model must support partial refresh"
                assert "trmnl_start_display_refreshes" in device, f"{slug}: boot must defer the first e-paper refresh"
                assert "trmnl_display_refresh_enabled" in device, f"{slug}: display refresh gate is missing"
                assert "delay: 75s" in sensors, f"{slug}: first e-paper refresh should wait until services have started"
                assert 'name: "Refresh Display"' in device, f"{slug}: web UI should expose a manual e-paper refresh"
                assert "Finished e-paper refresh" in sensors, f"{slug}: display refresh completion log is missing"
                assert "trmnl_dashboard_config_changed" in sensors, f"{slug}: card config changes must refresh e-paper"
                assert "trmnl_dashboard_content_changed" in sensors, f"{slug}: HA card content changes must refresh e-paper"
                assert "set_dashboard_content_changed_callback" in sensors, (
                    f"{slug}: shared weather/card updates must be wired to e-paper refreshes"
                )
                assert "lv_obj_set_style_bg_color(lv_scr_act(), page_bg, LV_PART_MAIN);" in sensors, (
                    f"{slug}: TRMNL theme must paint the full e-paper screen background"
                )
                assert (
                    "id: trmnl_topbar_separator" in lvgl
                    and "y: 60" in lvgl
                    and "height: 1" in lvgl
                    and "lv_obj_set_style_bg_color(id(trmnl_topbar_separator), lv_color_hex(0x000000), LV_PART_MAIN)" in sensors
                ), f"{slug}: TRMNL top bar must match the web preview divider"
                assert (
                    "align: top_left\n      x: 10\n      y: 68" in lvgl
                    and "pad_row: 6\n        pad_column: 6" in lvgl
                    and "width: 780\n      height: 402" in lvgl
                    and "pad_all: 0" in lvgl
                ), f"{slug}: LVGL card grid must match the generated web preview margins"
                assert "cfg.temperature_unit = id(temperature_unit_select).current_option();" in sensors, (
                    f"{slug}: weather cards must use the configured temperature unit"
                )
                assert "id: temperature_unit_select" in device, (
                    f"{slug}: device.yaml must expose the temperature unit setting used by weather cards"
                )
                assert "id: timezone_select" in device, (
                    f"{slug}: device.yaml must expose the timezone setting used by Auto temperature units"
                )
                assert "cfg.timezone = id(timezone_select).current_option();" in sensors, (
                    f"{slug}: automatic temperature units must use the configured timezone"
                )
                assert "id(font_trmnl_value_large_80)->get_lv_font()" in sensors, (
                    f"{slug}: weather large-number cards must use the TRMNL web preview large-number font"
                )
                assert "cfg.label_lines = 2;" in sensors and "cfg.label_lines_tall = 3;" in sensors, (
                    f"{slug}: TRMNL card labels must clamp to the generated web preview line counts"
                )
                grid_header = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
                assert "lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 0, 0);" in grid_header, (
                    f"{slug}: clamped card labels must stay bottom-aligned like the web preview"
                )
                setup_match = re.search(r"inline void setup_card_visual\([\s\S]*?if \(is_text_sensor_card", grid_header)
                assert (
                    setup_match
                    and "lv_obj_set_style_text_font(s.sensor_lbl, display_sensor_font(display), LV_PART_MAIN)" in setup_match.group(0)
                ), f"{slug}: normal weather values must reset after large-number card layouts"
                phase1_match = re.search(r"inline void grid_phase1\([\s\S]*?ESP_LOGI\(\"sensors\", \"Phase 1: done", grid_header)
                assert (
                    phase1_match
                    and "setup_card_visual(s, p, cfg, palette, row_span, col_span);" in phase1_match.group(0)
                    and "refresh_card_layout(s, p, cfg, row_span);" in phase1_match.group(0)
                ), f"{slug}: initial TRMNL weather render must apply the same shared layout refresh as later updates"
                assert "id(font_trmnl_value_32)->get_lv_font()" in sensors, (
                    f"{slug}: normal weather cards must use the TRMNL web preview value font"
                )
                assert "id: font_trmnl_value_32\n    size: 32" in fonts_path.read_text(encoding="utf-8"), (
                    f"{slug}: normal weather value font must match the TRMNL web preview metric"
                )
                assert "id: font_trmnl_value_large_80\n    size: 80" in fonts_path.read_text(encoding="utf-8"), (
                    f"{slug}: large-number font must match the TRMNL web preview metric"
                )
                trmnl_fonts = fonts_path.read_text(encoding="utf-8")
                assert "id: font_trmnl_label_14\n    size: 16\n    glyphs: \" !\\\"%()+,-./0123456789:°" in trmnl_fonts, (
                    f"{slug}: weather forecast unit font must include the degree symbol"
                )
                assert (
                    "id: font_trmnl_mdi_topbar_18_icons\n    size: 18" in trmnl_fonts
                    and '- "\\U000F0200"' in trmnl_fonts
                    and '- "\\U000F0928"' in trmnl_fonts
                    and "id: network_status_button\n          align: top_right\n          x: -8\n          y: 0\n          width: 24\n          height: 60" in lvgl
                    and "id: network_status_icon_label\n                text: \"\\U000F0928\"\n                text_font: font_trmnl_mdi_topbar_18_icons" in lvgl
                ), f"{slug}: top bar network icon must match the generated web preview scale"
                tile = tile_path.read_text(encoding="utf-8")
                assert (
                    "flex_flow: row\n          flex_align_cross: end\n          pad_column: 0" in tile
                    and "id: button_${num}_unit_label\n              text: \"\"\n              text_font: font_trmnl_label_14\n              text_color: 0x000000\n              pad_bottom: 0" in tile
                ), (
                    f"{slug}: weather forecast unit label must align like the web preview"
                )
                assert "radius: 2" in tile and "lv_obj_set_style_radius(slot.btn, 2, LV_PART_MAIN)" in sensors, (
                    f"{slug}: TRMNL card corner radius must match the nearly-square web preview cards"
                )
                assert "pad_all: 9" in tile and "lv_obj_set_style_pad_all(slot.btn, 9, LV_PART_MAIN)" in sensors, (
                    f"{slug}: TRMNL card padding must match the generated web preview spacing"
                )
                assert "set_display_temperature_unit(id(temperature_unit_select).current_option(),\n                                         id(timezone_select).current_option())" in device, (
                    f"{slug}: temperature unit and timezone changes must update the shared display unit helper"
                )
                assert "apply_registered_ha_control_availability(true);" in device, (
                    f"{slug}: Home Assistant-backed weather cards must become available on connect"
                )
                assert "apply_registered_ha_control_availability(false);" in device, (
                    f"{slug}: Home Assistant-backed weather cards must show unavailable on disconnect"
                )
                assert "weather_forecast_cancel_pending_requests();" in device, (
                    f"{slug}: pending forecast callbacks must be cancelled on Home Assistant disconnect"
                )
        else:
            assert f"cfg.num_slots = {profile['slots']};" in sensors, f"{slug}: sensors.yaml missing slot count"


def test_setup_icon_glyphs() -> None:
    glyphs = (ROOT / "common" / "assets" / "icon_glyphs.yaml").read_text(encoding="utf-8")
    for glyph, icon_name in REQUIRED_SETUP_ICON_GLYPHS.items():
        assert glyph in glyphs, f"shared icon font missing {icon_name} for OTA update screen"


def test_trmnl_epaper_icon_literals() -> None:
    shared_glyphs = (ROOT / "common" / "assets" / "icon_glyphs.yaml").read_text(encoding="utf-8")
    trmnl_yaml = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "devices" / "trmnl-75-og" / "device").glob("*.yaml")
    )
    local_glyphs = set(re.findall(r'"(\\U[0-9A-Fa-f]{8})"', trmnl_yaml))
    missing_glyphs = sorted({
        glyph for glyph in re.findall(r'\\U[0-9A-Fa-f]{8}', trmnl_yaml)
        if f'"{glyph.upper()}"' not in shared_glyphs and glyph.upper() not in local_glyphs
    })
    assert not missing_glyphs, f"TRMNL hard-coded icon glyphs missing from icon font: {', '.join(missing_glyphs)}"


def test_weather_card_visual_matches_preview() -> None:
    cards = BUTTON_GRID_CARDS.read_text(encoding="utf-8")
    styles = (ROOT / "src" / "webserver" / "modules" / "styles.js").read_text(encoding="utf-8")
    assert ".sp-type-badge{display:none}" in styles, "web preview type badges should remain visually hidden"
    assert "set_weather_card_badge" not in cards, (
        "device weather cards should not show the hidden web preview type badge"
    )
    assert 'set_weather_card_badge(s, "Weather Cloudy")' not in cards, (
        "current weather device card should not render a visible weather badge"
    )
    assert 'lv_label_set_text(s.text_lbl, "Cloudy")' in cards, (
        "current weather device card should render the same label as the web preview"
    )
    assert 'set_weather_card_badge(s, "Weather Partly Cloudy")' not in cards, (
        "forecast weather device card should not render a visible forecast badge"
    )
    assert 'lv_label_set_text(s.unit_lbl, display_temperature_unit_symbol())' in cards, (
        "forecast weather placeholder should show the configured unit like the web preview"
    )
    config = (ROOT / "components" / "espcontrol" / "button_grid_config.h").read_text(encoding="utf-8")
    assert 'lv_label_set_text(ref.unit_lbl, normalized_unit.c_str())' in config, (
        "forecast weather unavailable state should keep showing the configured unit"
    )
    for state, icon_name, label in (
        ("dust", "Weather Dust", "Dust"),
        ("hazy", "Weather Hazy", "Hazy"),
        ("hurricane", "Weather Hurricane", "Hurricane"),
        ("snowy-heavy", "Weather Snowy Heavy", "Heavy Snow"),
        ("sunny-alert", "Weather Sunny Alert", "Sunny Alert"),
        ("tornado", "Weather Tornado", "Tornado"),
    ):
        assert f'if (state == "{state}") return find_icon("{icon_name}");' in config, (
            f"current weather device card should map {state} to the matching web weather icon"
        )
        assert f'if (state == "{state}") return "{label}";' in config, (
            f"current weather device card should label {state} like the web preview"
        )


def test_weather_card_mode_visibility_reset() -> None:
    cards = BUTTON_GRID_CARDS.read_text(encoding="utf-8")
    match = re.search(
        r"inline void setup_weather_card\(BtnSlot &s,[\s\S]*?\n\}",
        cards,
    )
    assert match, "current weather setup is missing"
    body = match.group(0)
    assert "lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN)" in body, (
        "current weather cards must restore the icon after forecast mode hid it"
    )
    assert "lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN)" in body, (
        "current weather cards must hide the forecast sensor row"
    )


def test_grid_phase2_uses_cleaned_spanned_layout() -> None:
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    match = re.search(
        r"inline void grid_phase2\([\s\S]*?ESP_LOGI\(\"sensors\", \"Phase 2: done",
        grid,
    )
    assert match, "shared grid phase 2 is missing"
    body = match.group(0)
    assert "OrderResult parsed, order;" in body and "clear_spanned_cells(parsed, NS, COLS, order);" in body, (
        "phase 2 must bind weather/card state using the same cleaned spanned layout as the preview"
    )
    assert "int idx = order.positions[pos];" in body, (
        "phase 2 must skip grid cells covered by larger cards"
    )


def test_temperature_unit_changes_refresh_weather_cards() -> None:
    config = (ROOT / "components" / "espcontrol" / "button_grid_config.h").read_text(encoding="utf-8")
    match = re.search(
        r"inline void refresh_temperature_unit_labels\(\)[\s\S]*?\n\}",
        config,
    )
    assert match, "temperature unit label refresh helper is missing"
    body = match.group(0)
    assert "notify_dashboard_content_changed()" in body, (
        "temperature unit changes must refresh e-paper weather cards"
    )


def test_current_weather_state_updates_availability() -> None:
    subscriptions = (ROOT / "components" / "espcontrol" / "button_grid_subscriptions.h").read_text(encoding="utf-8")
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    match = re.search(
        r"inline void subscribe_weather_state\([\s\S]*?\n\}",
        subscriptions,
    )
    assert match, "current weather state subscription is missing"
    body = match.group(0)
    assert "apply_control_availability(btn_ptr, btn_ptr, !unavailable, false)" in body, (
        "current weather cards must clear unavailable styling when Home Assistant sends a valid state"
    )
    assert "notify_dashboard_content_changed()" in body, (
        "current weather state changes must refresh TRMNL e-paper"
    )
    assert "uint32_t generation = ha_subscription_generation();" in body and "generation != ha_subscription_generation()" in body, (
        "current weather callbacks must ignore stale subscriptions after dashboard reconfiguration"
    )
    assert "bump_ha_subscription_generation();" in grid, (
        "dashboard reconfiguration must invalidate stale current weather subscriptions"
    )
    assert "weather_forecast_cancel_pending_requests();" in grid, (
        "dashboard reconfiguration must cancel stale weather forecast action responses"
    )


def test_trmnl_weather_forecast_queue_drains() -> None:
    device = (ROOT / "devices" / "trmnl-75-og" / "device" / "device.yaml").read_text(encoding="utf-8")
    assert "weather_forecast_cancel_stale_requests();" in device, (
        "TRMNL must cancel stale Home Assistant forecast requests"
    )
    assert "weather_forecast_send_next_queued();" in device, (
        "TRMNL must send queued forecast requests after Home Assistant actions become ready"
    )
    assert "refresh_weather_forecast_cards();" in device, (
        "TRMNL must periodically refresh dynamic weather forecast card values"
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
    test_weather_card_visual_matches_preview()
    test_weather_card_mode_visibility_reset()
    test_grid_phase2_uses_cleaned_spanned_layout()
    test_temperature_unit_changes_refresh_weather_cards()
    test_current_weather_state_updates_availability()
    test_trmnl_weather_forecast_queue_drains()
    test_firmware_matrices(profile_slugs)
    test_public_firmware_slugs(profile_slugs)
    print("Device profile cross-checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
