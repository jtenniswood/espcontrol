#!/usr/bin/env python3
"""Cross-check generated device profile outputs against devices/manifest.json."""

from __future__ import annotations

import json
import re
from pathlib import Path

import device_matrix
import generate_device_slots
from device_profiles import ROOT, load_device_profiles, public_device_capabilities, web_config
import check_public_firmware


WEB_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"
DEVICE_CAPABILITIES_JSON = ROOT / "docs" / "public" / "device-profiles.json"
DEVICE_DOCS_DIR = ROOT / "docs" / "generated" / "screens"
COMPAT_FIXTURES = ROOT / "compatibility" / "fixtures" / "product_compatibility.json"
BUTTON_GRID_CARDS = ROOT / "components" / "espcontrol" / "button_grid_cards.h"
BUTTON_GRID_WEATHER_FORECAST = ROOT / "components" / "espcontrol" / "button_grid_weather_forecast.h"
REQUIRED_SETUP_ICON_GLYPHS = {
    r'"\U000F012C"': "mdi-check",
    r'"\U000F0996"': "mdi-progress-clock",
}
REQUIRED_LIGHT_CONTROL_ICON_GLYPHS = {
    r'"\U000F0425"': "mdi-power",
    r'"\U000F0766"': "mdi-circle-outline",
}
REQUIRED_CLIMATE_CARD_ICON_NAMES = {
    "Air Filter",
    "Fan",
    "Fire",
    "Power",
    "Snowflake",
    "Swap Horizontal",
    "Thermometer",
    "Thermostat",
    "Thermostat Auto",
    "Water",
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


def image_slot_capacity(profile: dict) -> int:
    return int(profile["capabilities"]["imageSlots"])


def test_zero_image_capacity_disables_all_image_card_pickers(profiles: dict[str, dict]) -> None:
    for slug, profile in profiles.items():
        if image_slot_capacity(profile) != 0:
            continue
        disabled = set(web_config(profile).get("disabledCardTypes", []))
        assert {"image", "media_cover_art"} <= disabled, (
            f"{slug}: zero image capacity must disable Image and Media Cover Art cards"
        )


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
        image_capacity_text = (
            "Not supported"
            if capability["imageSlots"] == 0
            else f'Up to {capability["imageSlots"]} simultaneous Image or Media Cover Art cards'
        )
        assert image_capacity_text in grid, f"{stem}: grid snippet missing image capacity"
        assert f'`{capability["installSlug"]}`' in grid, f"{stem}: grid snippet missing install slug"
        relay_text = "No built-in relays" if capability["relays"] == 0 else f"{capability['relays']} built-in relay"
        assert relay_text in grid, f"{stem}: grid snippet missing relay availability"
        ethernet_text = "Yes, manual ESPHome install only" if capability["ethernetManualInstall"] else "No"
        assert ethernet_text in grid, f"{stem}: grid snippet missing Ethernet support"
        assert f'slug="{capability["installSlug"]}"' in install, f"{stem}: install snippet missing slug"


def test_generated_web(profiles: dict[str, dict]) -> None:
    path = WEB_OUTPUT_DIR / "www.js"
    assert path.is_file(), "shared generated web bundle is missing"
    text = path.read_text(encoding="utf-8")

    for slug, profile in profiles.items():
        assert slug in text, f"{slug}: shared generated web bundle is missing the device profile"
        loader_path = WEB_OUTPUT_DIR / slug / "www.js"
        loader = loader_path.read_text(encoding="utf-8")
        assert len(loader) < 1024, f"{slug}: compatibility loader unexpectedly contains a full web bundle"
        assert 'new URL("../www.js"' in loader and slug in loader, (
            f"{slug}: compatibility loader does not launch the shared bundle"
        )
        capacity = image_slot_capacity(profile)
        assert f"imageSlotCapacity:{capacity}" in text or f'"imageSlotCapacity":{capacity}' in text, (
            f"{slug}: generated web bundle has wrong image slot capacity"
        )

    core = (ROOT / "common" / "device" / "core_infra.yaml").read_text(encoding="utf-8")
    assert "webserver/www.js?device=${device_slug}" in core, "hosted web URL does not select a shared profile"
    assert 'ESPCONTROL_DEVICE_SLUG=\\"${device_slug}\\"' in core, "firmware build does not expose its profile slug"
    server = (ROOT / "components" / "web_server_idf" / "web_server_idf.cpp").read_text(encoding="utf-8")
    assert '\\"device_slug\\"' in server and "ESPCONTROL_DEVICE_PROFILE" in server, (
        "firmware metadata endpoint does not expose the shared web profile"
    )
    for slug in profiles:
        for suffix in (".yaml", ".factory.yaml"):
            build = (ROOT / "builds" / f"{slug}{suffix}").read_text(encoding="utf-8")
            assert 'docs/public/webserver/www.js"' in build, f"{slug}{suffix}: firmware does not embed shared web bundle"


def test_generated_yaml(profiles: dict[str, dict]) -> None:
    for slug, profile in profiles.items():
        package_path = ROOT / "devices" / slug / "packages.yaml"
        device_path = ROOT / "devices" / slug / "device" / "device.yaml"
        sensor_path = ROOT / "devices" / slug / "device" / "sensors.yaml"
        package = package_path.read_text(encoding="utf-8")
        device_path.read_text(encoding="utf-8")
        sensors = sensor_path.read_text(encoding="utf-8")
        assert f'device_slug: "{slug}"' in package, f"{slug}: packages.yaml missing device slug"
        assert f'firmware_manifest_slug: "{slug}"' in package, f"{slug}: packages.yaml missing manifest slug"
        assert f"cfg.num_slots = {profile['slots']};" in sensors, f"{slug}: sensors.yaml missing slot count"
        capacity = image_slot_capacity(profile)
        if capacity > 0:
            package_name = "image_cards.yaml" if capacity == 4 else f"image_cards_{capacity}.yaml"
            assert package_name in package, f"{slug}: packages.yaml missing {package_name}"
            assert f"cfg.image_card_image_count = {capacity};" in sensors, (
                f"{slug}: sensors.yaml missing image-card downloader count"
            )
            assert f"id(image_card_download_{capacity})," in sensors, (
                f"{slug}: sensors.yaml missing final image-card tile downloader"
            )
            assert f"id(image_card_modal_download_{capacity})," in sensors, (
                f"{slug}: sensors.yaml missing final image-card modal downloader"
            )
        else:
            assert "image_cards:" not in package, f"{slug}: zero image-card profile should not include image cards"
            assert "cfg.image_card_image_count" not in sensors, (
                f"{slug}: zero image-card profile should not wire image-card downloaders"
            )
        if profile["firmware"].get("display", {}).get("infoOnly"):
            assert "cfg.info_only = true;" in sensors, f"{slug}: sensors.yaml missing info-only grid flag"


def test_upgrades_do_not_reset_saved_panel_config() -> None:
    display = (ROOT / "common" / "config" / "display.yaml").read_text(encoding="utf-8")
    generator = (ROOT / "scripts" / "generate_device_slots.py").read_text(encoding="utf-8")
    assert "panel_device_settings_reset_version" not in display, (
        "firmware upgrades must not add a stored reset marker for panel config"
    )
    assert "reset_existing_panel_settings" not in generator, (
        "generated device YAML must not include a boot-time panel config reset script"
    )

    for sensor_path in sorted((ROOT / "devices").glob("*/device/sensors.yaml")):
        text = sensor_path.read_text(encoding="utf-8")
        rel = sensor_path.relative_to(ROOT)
        assert "reset_existing_panel_settings" not in text, f"{rel}: must not reset saved panel config on boot"
        assert "id(button_order).publish_state(\"\")" not in text, f"{rel}: must not clear saved button order"
        assert not re.search(r"id\((?:button|subpage)_\d+_config(?:_ext(?:_\d+)?)?\)\.publish_state\(\"\"\)", text), (
            f"{rel}: must not clear saved button or subpage config"
        )


def test_local_voice_generation_uses_capability() -> None:
    voice_device = {
        "slug": "semantic-voice-test",
        "package": {"localVoiceServices": True},
    }
    standard_device = {
        "slug": "esp32-p4-86",
        "package": {"firmwareVersion": "dev"},
    }
    assert "open_device_volume_control" in "\n".join(
        generate_device_slots.voice_substitution_lines(voice_device)
    ), "local voice generation must follow the semantic capability"
    assert "open_device_volume_control" not in "\n".join(
        generate_device_slots.voice_substitution_lines(standard_device)
    ), "the device slug alone must not enable local voice generation"


def test_square_s3_reapplies_clock_bar_after_screen_changes() -> None:
    slug = "guition-esp32-s3-4848s040"
    sensors = (ROOT / "devices" / slug / "device" / "sensors.yaml").read_text(encoding="utf-8")
    device = (ROOT / "devices" / slug / "device" / "device.yaml").read_text(encoding="utf-8")
    assert (
        "grid_refresh_layout(slots, cfg,\n"
        "            id(button_order).state,\n"
        "            id(main_page)->obj);\n"
        "      - script.execute: clock_bar_apply"
    ) in sensors, "S3 grid refresh must reapply the fixed clock bar like the working square profile"
    assert (
        "grid_phase2(slots, cfg, sp_cfgs, sp_ext, sp_ext2, sp_ext3,\n"
        "              id(button_order).state,\n"
        "              id(button_on_color).state,\n"
        "              id(main_page)->obj);\n"
        "        - script.execute: clock_bar_apply"
    ) in sensors, "S3 boot setup must reapply the fixed clock bar after subpages are created"
    assert (
        "- script.execute: apply_screen_rotation\n"
        "        - script.execute: clock_bar_apply"
    ) in device, "S3 restored rotation must reapply the fixed clock bar"
    assert (
        "- script.execute: apply_screen_rotation\n"
        "              - script.execute: clock_bar_apply"
    ) in device, "S3 rotation changes must reapply the fixed clock bar"


def test_p4_43_rotation_refresh_rebuilds_subpages() -> None:
    slug = "guition-esp32-p4-jc4880p443"
    sensors = (ROOT / "devices" / slug / "device" / "sensors.yaml").read_text(encoding="utf-8")
    assert (
        "grid_refresh_layout(slots, cfg,\n"
        "            id(button_order).state,\n"
        "            id(main_page)->obj);\n"
        "          navigation_return_home(id(main_page)->obj);"
    ) in sensors, (
        "4.3-inch P4 rotation refresh must refresh the home grid before rebuilding subpages"
    )
    assert "grid_phase2(slots, cfg, sp_cfgs, sp_ext, sp_ext2, sp_ext3, sp_ext4, sp_ext5, sp_ext6, sp_ext7," in sensors, (
        "4.3-inch P4 rotation refresh must rebuild subpage grids with the current column count"
    )
    assert "id(button_on_color).state" in sensors and "id(button_off_color).state" not in sensors, (
        "4.3-inch P4 subpage rebuild must keep the configured primary color only"
    )


def web_screen_width_percent(profile: dict) -> float:
    width = str(profile["web"]["screen"]["width"]).strip()
    assert width.endswith("%"), f"{profile['public']['name']}: web screen width must be a percentage"
    return float(width[:-1])


def parse_resolution(profile: dict) -> tuple[int, int]:
    resolution = str(profile["public"]["resolution"]).strip()
    match = re.fullmatch(r"([1-9]\d*)\s*x\s*([1-9]\d*)", resolution)
    assert match, f"{profile['slug']}: public resolution must look like '1024 x 600'"
    return int(match.group(1)), int(match.group(2))


def parse_aspect(profile: dict, key_path: str, value: str) -> tuple[int, int]:
    match = re.fullmatch(r"([1-9]\d*)/([1-9]\d*)", str(value).strip())
    assert match, f"{profile['slug']}: {key_path} must look like '1024/600'"
    return int(match.group(1)), int(match.group(2))


def orientation_for(width: int, height: int) -> str:
    if width == height:
        return "Square"
    return "Landscape" if width > height else "Portrait"


def assert_same_ratio(slug: str, label: str, left: tuple[int, int], right: tuple[int, int]) -> None:
    assert left[0] * right[1] == left[1] * right[0], (
        f"{slug}: {label} must match the public screen resolution"
    )


def test_web_screen_aspect_matches_public_resolution() -> None:
    profiles = load_device_profiles()
    for slug, profile in profiles.items():
        resolution = parse_resolution(profile)
        assert profile["public"]["orientation"] == orientation_for(*resolution), (
            f"{slug}: public orientation must match public resolution"
        )
        screen = parse_aspect(profile, "web.screen.aspect", profile["web"]["screen"]["aspect"])
        assert_same_ratio(slug, "web.screen.aspect", screen, resolution)

        portrait = profile["web"].get("portrait")
        if portrait:
            portrait_screen = parse_aspect(
                profile,
                "web.portrait.screen.aspect",
                portrait["screen"]["aspect"],
            )
            assert_same_ratio(
                slug,
                "web.portrait.screen.aspect",
                portrait_screen,
                (resolution[1], resolution[0]),
            )


def test_web_grid_spacing_matches_across_screen_sizes() -> None:
    profiles = load_device_profiles()
    expected = None
    for slug, profile in profiles.items():
        grid = profile["web"]["grid"]
        rendered_gap = float(grid["gap"]) * web_screen_width_percent(profile) / 100.0
        if expected is None:
            expected = rendered_gap
        assert abs(rendered_gap - expected) <= 0.01, (
            f"{slug}: web preview grid spacing must match the other generated screen layouts"
        )


def test_setup_icon_glyphs() -> None:
    glyphs = (ROOT / "common" / "assets" / "icon_glyphs.yaml").read_text(encoding="utf-8")
    for glyph, icon_name in REQUIRED_SETUP_ICON_GLYPHS.items():
        assert glyph in glyphs, f"shared icon font missing {icon_name} for OTA update screen"
    for glyph, icon_name in REQUIRED_LIGHT_CONTROL_ICON_GLYPHS.items():
        assert glyph in glyphs, f"shared icon font missing {icon_name} for light control modal"


def test_climate_card_icon_glyphs() -> None:
    icons = read_json(ROOT / "common" / "assets" / "icons.json")
    icon_by_name = {icon["name"]: icon for icon in icons["icons"]}
    glyphs = (ROOT / "common" / "assets" / "climate_card_icon_glyphs.yaml").read_text(encoding="utf-8")

    for icon_name in sorted(REQUIRED_CLIMATE_CARD_ICON_NAMES):
        icon = icon_by_name[icon_name]
        glyph = rf'"\U{icon["codepoint"]:>08s}"'
        assert glyph in glyphs, f"climate card icon font missing {icon_name}"

    for font_path in sorted((ROOT / "devices").glob("*/device/fonts.yaml")):
        text = font_path.read_text(encoding="utf-8")
        rel = font_path.relative_to(ROOT)
        card_match = re.search(
            r"id: font_icon_card\n\s*size: \d+\n\s*bpp: \d+\n\s*glyphs: !include (.+)",
            text,
        )
        assert card_match, f"{rel}: missing font_icon_card"
        assert card_match.group(1).strip().endswith("common/assets/climate_card_icon_glyphs.yaml"), (
            f"{rel}: font_icon_card should use climate_card_icon_glyphs.yaml"
        )


def test_weather_card_visual_matches_preview() -> None:
    cards = BUTTON_GRID_CARDS.read_text(encoding="utf-8")
    styles = (ROOT / "src" / "webserver" / "application" / "styles.ts").read_text(encoding="utf-8")
    subpages = (ROOT / "components" / "espcontrol" / "button_grid_subpages.h").read_text(encoding="utf-8")
    weather_forecast = BUTTON_GRID_WEATHER_FORECAST.read_text(encoding="utf-8")
    assert ".sp-type-badge{display:none}" in styles, "web preview type badges should remain visually hidden"
    assert "set_weather_card_badge" not in cards, (
        "device weather cards should not show the hidden web preview type badge"
    )
    assert 'set_weather_card_badge(s, "Weather Cloudy")' not in cards, (
        "current weather device card should not render a visible weather badge"
    )
    assert 'lv_label_set_text(s.text_lbl, espcontrol_i18n("Cloudy"))' in cards, (
        "current weather device card should render the same label as the web preview"
    )
    assert 'set_weather_card_badge(s, "Weather Partly Cloudy")' not in cards, (
        "forecast weather device card should not render a visible forecast badge"
    )
    assert '"HA Actions"' not in weather_forecast, (
        "forecast weather errors should keep the configured/default label like the web preview"
    )
    assert 'lv_label_set_text(s.unit_lbl, display_temperature_unit_symbol())' in cards, (
        "forecast weather placeholder should show the configured unit like the web preview"
    )
    assert 'lv_label_set_text(ref.unit_lbl, normalized_unit.c_str())' in weather_forecast, (
        "forecast weather unavailable state should keep showing the configured unit"
    )
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    setup_match = re.search(r"inline void setup_card_visual\([\s\S]*?if \(is_text_sensor_card", grid)
    assert (
        "inline void reset_card_slot_dynamic_children" in grid
        and "lv_obj_del(child);" in grid
        and "lv_obj_set_user_data(s.sensor_container, nullptr);" in grid
        and "lv_obj_clear_state(s.btn, LV_STATE_CHECKED);" in grid
        and "lv_obj_clear_state(s.btn, LV_STATE_DISABLED);" in grid
        and "lv_obj_set_style_opa(s.btn, LV_OPA_COVER, LV_PART_MAIN);" in grid
        and setup_match
        and "reset_card_slot_dynamic_children(s);" in setup_match.group(0)
    ), "weather cards must clear stale widget children, active states, and opacity before rendering"
    assert (
        setup_match
        and "lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);" in setup_match.group(0)
        and "lv_obj_align(s.sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);" in setup_match.group(0)
        and "lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);" in setup_match.group(0)
    ), "weather cards must reset inherited icon, value, and label placement before rendering"
    assert "inline std::string normalize_weather_state" in weather_forecast, (
        "current weather device cards should normalize equivalent weather state spellings before mapping icons"
    )
    assert 'if (normalized == "partly-cloudy") return "partlycloudy";' in weather_forecast, (
        "current weather device cards should accept the dashed partly-cloudy spelling"
    )
    assert 'if (normalized.compare(0, 8, "weather-") == 0) normalized = normalized.substr(8);' in weather_forecast, (
        "current weather device cards should accept web weather icon names as state aliases"
    )
    assert 'if (normalized.compare(0, 4, "mdi-") == 0) normalized = normalized.substr(4);' in weather_forecast, (
        "current weather device cards should accept web Material Design weather class names as state aliases"
    )
    assert 'if (normalized == "night") return "clear-night";' in weather_forecast, (
        "current weather device cards should map the web Weather Night icon name to clear night"
    )
    assert 'normalized == "night-cloudy"' in weather_forecast and 'return "night-partly-cloudy";' in weather_forecast, (
        "current weather device cards should accept night cloudy aliases for the web weather icon"
    )
    assert 'normalized == "sunny-off"' in weather_forecast and 'return "unavailable";' in weather_forecast, (
        "current weather device cards should map the web unavailable weather icon name"
    )
    assert 'normalized == "unknown"' in weather_forecast and 'return "unavailable";' in weather_forecast, (
        "current weather device cards should render unknown states with the unavailable weather icon"
    )
    assert 'if (b.type == "weather" && !card_runtime_weather_forecast_precision(b.precision))' in subpages, (
        "subpage weather cards must normalize invalid weather modes like main grid cards"
    )
    for alias, state in (
        ("blizzard", "snowy-heavy"),
        ("broken-clouds", "cloudy"),
        ("clear", "sunny"),
        ("clear-day", "sunny"),
        ("drizzle", "rainy"),
        ("few-clouds", "partlycloudy"),
        ("foggy", "fog"),
        ("freezing-rain", "snowy-rainy"),
        ("heavy-rain", "pouring"),
        ("heavy-showers", "pouring"),
        ("heavy-snow", "snowy-heavy"),
        ("light-rain", "rainy"),
        ("mostly-clear", "sunny"),
        ("mostly-clear-night", "clear-night"),
        ("mostly-cloudy", "cloudy"),
        ("mostly-sunny", "sunny"),
        ("night-clear", "clear-night"),
        ("overcast", "cloudy"),
        ("partly-cloudy-day", "partlycloudy"),
        ("cloudy-night", "night-partly-cloudy"),
        ("few-clouds-night", "night-partly-cloudy"),
        ("mostly-cloudy-night", "night-partly-cloudy"),
        ("partly-cloudy-night", "night-partly-cloudy"),
        ("partly-sunny", "partlycloudy"),
        ("possibly-rainy-day", "rainy"),
        ("possibly-rainy-night", "rainy"),
        ("possibly-sleet-day", "snowy-rainy"),
        ("possibly-sleet-night", "snowy-rainy"),
        ("possibly-snow-day", "snowy"),
        ("possibly-snow-night", "snowy"),
        ("possibly-thunderstorm-day", "lightning-rainy"),
        ("possibly-thunderstorm-night", "lightning-rainy"),
        ("rain", "rainy"),
        ("sleet", "snowy-rainy"),
        ("snow", "snowy"),
        ("scattered-clouds", "cloudy"),
        ("showers", "rainy"),
        ("storm", "lightning"),
        ("stormy", "lightning"),
        ("thunderstorm", "lightning"),
        ("thunderstorms", "lightning"),
    ):
        assert f'if (normalized == "{alias}") return "{state}";' in weather_forecast or (
            f'normalized == "{alias}"' in weather_forecast and f'return "{state}";' in weather_forecast
        ), f"current weather device cards should normalize provider alias {alias} to {state}"
    for state, icon_name, label in (
        ("cloudy-alert", "Weather Cloudy Alert", "Cloudy Alert"),
        ("dust", "Weather Dust", "Dust"),
        ("hazy", "Weather Hazy", "Hazy"),
        ("hurricane", "Weather Hurricane", "Hurricane"),
        ("night-partly-cloudy", "Weather Night Cloudy", "Partly Cloudy Night"),
        ("partly-lightning", "Weather Partly Lightning", "Partly Lightning"),
        ("partly-rainy", "Weather Partly Rainy", "Partly Rainy"),
        ("partly-snowy", "Weather Partly Snowy", "Partly Snowy"),
        ("partly-snowy-rainy", "Weather Partly Snowy Rainy", "Partly Snow And Rain"),
        ("snowy-heavy", "Weather Snowy Heavy", "Heavy Snow"),
        ("sunny-alert", "Weather Sunny Alert", "Sunny Alert"),
        ("sunset", "Weather Sunset", "Sunset"),
        ("sunset-down", "Weather Sunset Down", "Sunset Down"),
        ("sunset-up", "Weather Sunset Up", "Sunset Up"),
        ("tornado", "Weather Tornado", "Tornado"),
    ):
        assert f'if (normalized == "{state}") return find_icon("{icon_name}");' in weather_forecast, (
            f"current weather device card should map {state} to the matching web weather icon"
        )
        assert f'if (normalized == "{state}") return espcontrol_i18n(std::string("{label}"));' in weather_forecast, (
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


def test_spanned_cards_refresh_after_clock_bar_padding_changes() -> None:
    clock_bar = (ROOT / "components" / "espcontrol" / "clock_bar.h").read_text(encoding="utf-8")
    layout = (ROOT / "components" / "espcontrol" / "button_grid_layout.h").read_text(encoding="utf-8")
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    assert "struct ClockBarResponsiveGridCard" in clock_bar, (
        "spanned card dimensions must be tracked outside the one-time grid placement pass"
    )
    assert "clock_bar_refresh_responsive_grid_cards();" in clock_bar, (
        "clock-bar padding changes must resize registered wide/tall/large cards"
    )
    assert "clock_bar_register_responsive_grid_card(" in layout, (
        "wide/tall/large cards must register their measured grid span"
    )
    assert "clock_bar_clear_responsive_grid_cards(main_page_obj);" in grid, (
        "main-grid refreshes must replace old responsive card registrations"
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
        "temperature unit changes must refresh weather cards"
    )


def test_current_weather_state_keeps_normal_card_visuals() -> None:
    subscriptions = (ROOT / "components" / "espcontrol" / "button_grid_subscriptions.h").read_text(encoding="utf-8")
    grid = (ROOT / "components" / "espcontrol" / "button_grid_grid.h").read_text(encoding="utf-8")
    match = re.search(
        r"inline void subscribe_weather_state\([\s\S]*?\n\}",
        subscriptions,
    )
    assert match, "current weather state subscription is missing"
    body = match.group(0)
    assert "apply_control_availability" not in body, (
        "current weather cards must not dim or disable themselves for unavailable entity states"
    )
    assert "notify_dashboard_content_changed()" in body, "current weather state changes must notify the dashboard"
    assert "uint32_t generation = ha_subscription_generation();" in body and "generation != ha_subscription_generation()" in body, (
        "current weather callbacks must ignore stale subscriptions after dashboard reconfiguration"
    )
    assert "bump_ha_subscription_generation();" in grid, (
        "dashboard reconfiguration must invalidate stale current weather subscriptions"
    )
    assert "weather_forecast_cancel_pending_requests();" in grid, (
        "dashboard reconfiguration must cancel stale weather forecast action responses"
    )
    assert (
        "if (bind_basic_sensor_card(sub_slot, sb_cfg, context, palette)) continue;" in grid
        and "if (bind_passive_card_sources(sub_slot, sb_cfg)) continue;" in grid
    ), "subpage weather cards must use the same passive weather binding path as main-grid weather cards"
    assert (
        "if (p.type == \"weather\")" in grid
        and "subscribe_weather_state(s.icon_lbl, s.text_lbl, p.entity)" in grid
    ), "subpage weather cards must use the same weather binding as main-grid weather cards"


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
    test_generated_web(profiles)
    test_zero_image_capacity_disables_all_image_card_pickers(profiles)
    test_generated_yaml(profiles)
    test_upgrades_do_not_reset_saved_panel_config()
    test_local_voice_generation_uses_capability()
    test_square_s3_reapplies_clock_bar_after_screen_changes()
    test_p4_43_rotation_refresh_rebuilds_subpages()
    test_web_screen_aspect_matches_public_resolution()
    test_web_grid_spacing_matches_across_screen_sizes()
    test_setup_icon_glyphs()
    test_weather_card_visual_matches_preview()
    test_weather_card_mode_visibility_reset()
    test_grid_phase2_uses_cleaned_spanned_layout()
    test_spanned_cards_refresh_after_clock_bar_padding_changes()
    test_temperature_unit_changes_refresh_weather_cards()
    test_current_weather_state_keeps_normal_card_visuals()
    test_firmware_matrices(profile_slugs)
    test_public_firmware_slugs(profile_slugs)
    print("Device profile cross-checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
