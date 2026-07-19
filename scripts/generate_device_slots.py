#!/usr/bin/env python3
"""Generate repeated ESPHome device YAML from devices/manifest.json."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from product_schema import slot_devices

ROOT = Path(__file__).resolve().parents[1]


PACKAGE_HEADER = """# =============================================================================
# PACKAGES - ESPHome include manifest
# =============================================================================
# Load order follows dependencies: device and assets first, then config,
# addons, and screens. Loading screen must be the first page so LVGL shows
# it at startup. Main page (lvgl.yaml) is included after setup screens.
# =============================================================================

"""


def package_data(device: dict) -> dict:
    package = device.get("package")
    if not package:
        raise ValueError(f"Missing firmware.package data for {device['slug']}")
    return package


def package_substitution_lines(device: dict) -> list[str]:
    package = package_data(device)
    lines = [
        '  cover_art_placeholder_file: "https://raw.githubusercontent.com/jtenniswood/espcontrol/main/common/assets/cover_art_placeholder.svg"',
        f'  device_slug: "{device["slug"]}"',
        f'  firmware_manifest_slug: "{device["slug"]}"',
    ]
    if package.get("firmwareVersion"):
        lines.append(f'  firmware_version: "{package["firmwareVersion"]}"')
    added_voice_substitutions = False
    for key, value in package["substitutions"].items():
        lines.append(f"  {key}: {value}")
        if key == "clock_bar_visual_gap":
            lines.extend(voice_substitution_lines(device))
            added_voice_substitutions = True
    if not added_voice_substitutions:
        lines.extend(voice_substitution_lines(device))
    if package.get("ethernetSelectable"):
        frequency = package["backlightPwmFrequency"]
        lines.extend(
            [
                '  network_transport: "wifi"',
                '  disable_updates: "false"',
                '  network_package_suffix: ${ "_ethernet" if network_transport == "ethernet" else "" }',
                '  firmware_update_package_suffix: ${ "_disabled" if disable_updates == "true" else "" }',
                '  esp32_c6_firmware_update_package_suffix: ${ "_disabled" if network_transport == "ethernet" else "" }',
                f'  backlight_pwm_frequency: ${{ "{frequency["ethernet"]}" if network_transport == "ethernet" else "{frequency["wifi"]}" }}',
            ]
        )
    lines.extend(cover_art_substitution_lines(device))
    return lines


def voice_substitution_lines(device: dict) -> list[str]:
    if not package_data(device).get("localVoiceServices"):
        return [
            '  voice_clock_bar_hide_code: ""',
            '  voice_clock_bar_apply_code: ""',
            "  navigate_voice_target_code: |-",
            '    ESP_LOGW("navigation", "Voice volume target is not available on this device");',
            '  voice_interaction_active_condition: "false"',
        ]
    return [
        "  voice_clock_bar_hide_code: |-",
        "    lv_obj_add_flag(id(voice_clock_bar_mute_button), LV_OBJ_FLAG_HIDDEN);",
        "  voice_clock_bar_apply_code: |-",
        "    if (id(voice_services_enabled).state) {",
        "      lv_obj_align(id(voice_clock_bar_mute_button), LV_ALIGN_TOP_RIGHT,",
        "                   -(clock_bar_right_x + clock_bar_item_width / 2), clock_bar_icon_y);",
        "      lv_obj_clear_flag(id(voice_clock_bar_mute_button), LV_OBJ_FLAG_HIDDEN);",
        "      const bool microphone_muted = id(master_mute_switch).state;",
        "      const bool output_muted = id(voice_media_player).is_muted();",
        "      lv_label_set_text(id(voice_clock_bar_mute_icon_label),",
        '                        microphone_muted ? "\\U000F036D" :',
        '                        output_muted ? "\\U000F04C4" : "\\U000F036C");',
        "      lv_obj_set_style_text_color(id(voice_clock_bar_mute_icon_label),",
        "                                  lv_color_hex(0xFFFFFF),",
        "                                  LV_PART_MAIN);",
        "    } else {",
        "      lv_obj_add_flag(id(voice_clock_bar_mute_button), LV_OBJ_FLAG_HIDDEN);",
        "    }",
        "  navigate_voice_target_code: |-",
        "    if (id(voice_services_enabled).state) {",
        "      id(open_device_volume_control).execute();",
        "    } else {",
        '      ESP_LOGW("navigation", "Voice volume target is not available while Voice Services are disabled");',
        "    }",
        '  voice_interaction_active_condition: "id(voice_interaction_active)"',
    ]


def cover_art_substitution_lines(device: dict) -> list[str]:
    layout = device.get("cover_art") or {}
    return [f'  {key}: "{value}"' for key, value in layout.items()]


def include_line(key: str, include: str) -> str:
    key_text = f"  {key}:"
    return key_text.ljust(19) + include if len(key_text) < 19 else f"{key_text} {include}"


def package_file_text(device: dict) -> str:
    package = package_data(device)
    network_suffix = "${network_package_suffix}" if package.get("ethernetSelectable") else ""
    network_screen_key = "screen_network" if package.get("ethernetSelectable") else "screen_wifi"
    network_screen_path = (
        "../../common/device/screen_${network_transport}_setup.yaml"
        if package.get("ethernetSelectable")
        else "../../common/device/screen_wifi_setup.yaml"
    )
    firmware_update_suffix = (
        "${firmware_update_package_suffix}" if package.get("ethernetSelectable") else ""
    )
    esp32_c6_firmware_update_suffix = (
        "${esp32_c6_firmware_update_package_suffix}"
        if package.get("ethernetSelectable")
        else ""
    )
    provisioning_suffix = (
        "${provisioning_package_suffix}" if package.get("provisioningSelectable") else ""
    )
    lines = [
        PACKAGE_HEADER.rstrip(),
        "",
    ]
    if package.get("provisioningSelectable"):
        lines.extend(
            [
                "defaults:",
                '  provisioning_package_suffix: "_deployed"',
                "",
            ]
        )
    lines.extend(
        [
            "substitutions:",
            *package_substitution_lines(device),
            "",
            "packages:",
            "  # ---------------------------------------------------------------------------",
            "  # Device, assets, and LVGL base",
            "  # ---------------------------------------------------------------------------",
            include_line("entity_names", "!include ../../common/config/entity_names.yaml"),
            include_line("device", "!include device/device.yaml"),
        ]
    )
    if package.get("touchscreenPackage"):
        lines.append(
            include_line("touchscreen", f"!include device/touchscreen{network_suffix}.yaml")
        )
    if package.get("networkCoprocessor"):
        lines.append(
            include_line(
                "network_coprocessor",
                f"!include device/network_coprocessor{network_suffix}.yaml",
            )
        )
    if package.get("esp32C6FirmwareUpdate"):
        lines.append(
            include_line(
                "esp32_c6_firmware_update",
                f"!include ../../common/device/esp32_c6_firmware_update{esp32_c6_firmware_update_suffix}.yaml",
            )
        )
    lines.extend(
        [
            include_line("icons", "!include ../../common/assets/icons.yaml"),
            include_line(
                package.get("deviceFontPackageKey", "fonts_device"),
                "!include device/fonts.yaml",
            ),
            include_line("button_theme", "!include ../../common/theme/button.yaml"),
            "  # ---------------------------------------------------------------------------",
            "  # Configuration (text/select/number components for web UI)",
            "  # ---------------------------------------------------------------------------",
            include_line("colors", "!include ../../common/config/colors.yaml"),
            include_line("button_order", "!include ../../common/config/button_order.yaml"),
            include_line("display_config", "!include ../../common/config/display.yaml"),
            button_package_block(device).rstrip(),
            "  # ---------------------------------------------------------------------------",
            "  # Addons",
            "  # ---------------------------------------------------------------------------",
            include_line(
                "connectivity",
                f"!include ../../common/addon/connectivity{network_suffix}{provisioning_suffix}.yaml",
            ),
        ]
    )
    if package.get("apiNavigateAction", True):
        lines.append(include_line("api_navigate", "!include ../../common/device/api_navigate.yaml"))
    lines.extend(
        [
            include_line("time_sync", "!include ../../common/addon/time.yaml"),
            include_line("backlight", "!include ../../common/addon/backlight.yaml"),
            include_line("bl_schedule", "!include ../../common/addon/backlight_schedule.yaml"),
            include_line("network", f"!include ../../common/addon/network{network_suffix}.yaml"),
            include_line("memory_diag", "!include ../../common/addon/memory_diagnostics.yaml"),
            include_line(
                "fw_update",
                f"!include ../../common/addon/firmware_update{firmware_update_suffix}.yaml",
            ),
            *[
                include_line(key, include)
                for key, include in (package.get("extraPackages") or {}).items()
            ],
            "",
            "  # ---------------------------------------------------------------------------",
            "  # Screens (loading must be first page for LVGL startup)",
            "  # ---------------------------------------------------------------------------",
            include_line(
                "screen_loading",
                f"!include ../../common/device/screen_loading{network_suffix}.yaml",
            ),
            include_line(network_screen_key, f"!include {network_screen_path}"),
            include_line("screen_ha", "!include ../../common/device/screen_ha_setup.yaml"),
            include_line("screen_ha_act", "!include ../../common/device/screen_ha_actions.yaml"),
            include_line("screen_setup", "!include ../../common/device/screen_button_setup.yaml"),
            include_line("screen_clock", "!include ../../common/device/screen_clock.yaml"),
            include_line("screen_art", "!include ../../common/device/screen_cover_art.yaml"),
            *(
                [
                    include_line(
                        "image_cards",
                        "!include ../../common/device/image_cards.yaml"
                        if int(device["image_slot_capacity"]) == 4
                        else f"!include ../../common/device/image_cards_{int(device['image_slot_capacity'])}.yaml",
                    )
                ]
                if int(device["image_slot_capacity"]) > 0
                else []
            ),
            "  # ---------------------------------------------------------------------------",
            "  # Main page and dynamic sensor subscriptions (after setup screens)",
            "  # ---------------------------------------------------------------------------",
            include_line("lvgl", "!include device/lvgl.yaml"),
            include_line("sensors", "!include device/sensors.yaml"),
            "",
        ]
    )
    return "\n".join(lines)


def button_package_block(device: dict) -> str:
    package = device.get("package") or {}
    subpage_chunks = int(package.get("subpageConfigChunks") or 8)
    template = "button_template_4chunk.yaml" if subpage_chunks == 4 else "button_template.yaml"
    lines = [
        "  # BEGIN GENERATED BUTTON PACKAGES",
        "  # Generated by scripts/generate_device_slots.py from devices/manifest.json.",
        f"  # Per-button entity/label/icon ({device['slots']} slots - {device['grid']} grid)",
    ]
    for num in range(1, device["slots"] + 1):
        key = f"  btn_{num}:".ljust(19)
        lines.append(
            f'{key}!include {{ file: ../../common/config/{template}, vars: {{ num: "{num}" }} }}'
        )
    lines.append("  # END GENERATED BUTTON PACKAGES")
    return "\n".join(lines) + "\n"


def replace_package_block(text: str, device: dict) -> str:
    block = button_package_block(device)
    marker = re.compile(
        r"(?ms)^  # BEGIN GENERATED BUTTON PACKAGES\n.*?^  # END GENERATED BUTTON PACKAGES\n"
    )
    if marker.search(text):
        return marker.sub(block, text, count=1)

    legacy = re.compile(
        r"(?ms)^  # Per-button entity/label/icon .*\n"
        r"(?:^  btn_\d+:\s+!include \{ file: ../../common/config/button_template.yaml, vars: \{ num: \"\d+\" \} \}\n)+"
    )
    next_text, count = legacy.subn(block, text, count=1)
    if count != 1:
        raise ValueError(f"Could not find button package block for {device['slug']}")
    return next_text


def button_slot_macro() -> str:
    return (
        "#define BTN_SLOT(n) { button_##n##_config, button_##n, button_##n##_icon_label, "
        "button_##n##_text_label, button_##n##_sensor_container, button_##n##_sensor_label, "
        "button_##n##_unit_label, button_##n##_subpage_label }"
    )


def macro_array(name: str, macro: str, slots: int, per_line: int = 4, indent: str = "            ") -> list[str]:
    lines = [f"{indent}esphome::text::Text *{name}[] = {{"]
    values = [f"{macro}({num})" for num in range(1, slots + 1)]
    for idx in range(0, len(values), per_line):
        chunk = ", ".join(values[idx : idx + per_line])
        lines.append(f"{indent}  {chunk},")
    lines.append(f"{indent}}};")
    return lines


def cfg_lines(device: dict) -> list[str]:
    image_card_count = int(device["image_slot_capacity"])
    lines = [
        "            GridConfig cfg = {};",
        f"            cfg.num_slots = {device['slots']};",
    ]
    if "portrait_cols" in device:
        lines.append('            bool portrait = id(screen_rotation_select).current_option() == "90" || id(screen_rotation_select).current_option() == "270";')
        lines.append(f"            cfg.cols = portrait ? {device['portrait_cols']} : {device['cols']};")
        if device.get("rotate_width_compensation", False):
            lines.append("            cfg.width_compensation_vertical = portrait;")
    else:
        lines.append(f"            cfg.cols = {device['cols']};")
    lines.append("            cfg.subpage_chevrons_enabled = id(subpage_chevrons_enabled).state;")
    if device.get("info_only"):
        lines.append("            cfg.info_only = true;")
    if device.get("subpage_chevron_x", 0) != 0:
        lines.append(f"            cfg.subpage_chevron_x = {device['subpage_chevron_x']};")
    if device.get("subpage_chevron_y", 2) != 2:
        lines.append(f"            cfg.subpage_chevron_y = {device['subpage_chevron_y']};")
    if device.get("subpage_chevron_text_width_percent", 94) != 94:
        lines.append(
            f"            cfg.subpage_chevron_text_width_percent = {device['subpage_chevron_text_width_percent']};"
        )
    if device["wrap_tall_labels"]:
        lines.append("            cfg.wrap_tall_labels = true;")
    if device.get("width_compensation_percent", 100) != 100:
        lines.append(f"            cfg.width_compensation_percent = {device['width_compensation_percent']};")
    if device.get("volume_width_compensation_percent", 100) != 100:
        lines.append(
            f"            cfg.volume_width_compensation_percent = {device['volume_width_compensation_percent']};"
        )
    if device.get("media_artwork_width_compensation_percent", 100) != 100:
        lines.append(
            f"            cfg.media_artwork_width_compensation_percent = {device['media_artwork_width_compensation_percent']};"
        )
    modal = device["modal"]
    modal_family = {
        "compact-square": "COMPACT_SQUARE",
        "large-square": "LARGE_SQUARE",
        "compact-portrait": "COMPACT_PORTRAIT",
        "wide-landscape": "WIDE_LANDSCAPE",
        "large-landscape": "LARGE_LANDSCAPE",
    }[modal["layoutFamily"]]
    modal_density = {
        "compact": "COMPACT",
        "comfortable": "COMFORTABLE",
        "spacious": "SPACIOUS",
    }[modal["density"]]
    modal_memory_tier = {
        "standard": "STANDARD",
        "constrained": "CONSTRAINED",
    }[modal["memoryTier"]]
    lines.append(
        "            cfg.modal_profile.layout_family = "
        f"DisplayModalLayoutFamily::{modal_family};"
    )
    lines.append(
        "            cfg.modal_profile.density = "
        f"DisplayModalDensity::{modal_density};"
    )
    lines.append(
        "            cfg.modal_profile.memory_tier = "
        f"DisplayModalMemoryTier::{modal_memory_tier};"
    )
    lines.append(
        "            cfg.modal_profile.base_touch_target = "
        f"{modal['baseTouchTarget']};"
    )
    if device.get("color_correction"):
        correction = device["color_correction"]
        lines.append(f"            cfg.color_correction_red_percent = {correction['red']};")
        lines.append(f"            cfg.color_correction_green_percent = {correction['green']};")
        lines.append(f"            cfg.color_correction_blue_percent = {correction['blue']};")
    lines.append(f"            cfg.icon_font = id({device['icon_font']})->get_lv_font();")
    lines.append(f"            cfg.sp_sensor_font = id({device['sensor_font']})->get_lv_font();")
    lines.append(f"            cfg.sp_large_sensor_font = id({device['large_sensor_font']})->get_lv_font();")
    lines.append(f"            cfg.large_sensor_unit_offset_percent = {device['large_sensor_unit_offset_percent']};")
    lines.append(f"            cfg.media_title_font = id({device['media_title_font']})->get_lv_font();")
    if device.get("media_control_title_font"):
        lines.append(
            f"            cfg.media_control_title_font = id({device['media_control_title_font']})->get_lv_font();"
        )
    if device.get("media_cover_art_title_font"):
        lines.append(
            f"            cfg.media_cover_art_title_font = id({device['media_cover_art_title_font']})->get_lv_font();"
        )
    if device.get("media_cover_art_artist_font"):
        lines.append(
            f"            cfg.media_cover_art_artist_font = id({device['media_cover_art_artist_font']})->get_lv_font();"
        )
    lines.append(f"            cfg.volume_number_font = id({device['volume_number_font']})->get_lv_font();")
    lines.append(f"            cfg.volume_label_font = id({device['volume_label_font']})->get_lv_font();")
    if device.get("climate_card_icon_font"):
        lines.append(
            f"            cfg.climate_card_icon_font = id({device['climate_card_icon_font']})->get_lv_font();"
        )
    if device.get("subpage_chevron_font"):
        lines.append(
            f"            cfg.subpage_chevron_font = id({device['subpage_chevron_font']})->get_lv_font();"
        )
    if device.get("climate_option_title_font"):
        lines.append(
            f"            cfg.climate_option_title_font = id({device['climate_option_title_font']})->get_lv_font();"
        )
    if device.get("climate_option_value_font"):
        lines.append(
            f"            cfg.climate_option_value_font = id({device['climate_option_value_font']})->get_lv_font();"
        )
    lines.append("            cfg.temperature_unit = id(temperature_unit_select).current_option();")
    lines.append("            cfg.timezone = id(timezone_select).current_option();")
    lines.append("            cfg.begin_display_takeover = [](espcontrol::DisplayTakeoverKind kind) {")
    lines.append("              id(display_takeover_begin).execute(static_cast<int>(kind));")
    lines.append("            };")
    lines.append("            cfg.end_display_takeover = [](espcontrol::DisplayTakeoverKind kind) {")
    lines.append("              id(display_takeover_end).execute(static_cast<int>(kind));")
    lines.append("            };")
    if package_data(device).get("alarmDelayAudio"):
        lines.extend(
            [
                "            cfg.alarm_delay_audio.enabled = []() {",
                "              return id(alarm_delay_audio_enabled).state;",
                "            };",
                "            cfg.alarm_delay_audio.tts_enabled = []() {",
                "              return id(alarm_delay_tts_enabled).state && id(voice_services_enabled).state;",
                "            };",
                "            cfg.alarm_delay_audio.final_countdown_seconds = []() {",
                "              return static_cast<int>(id(alarm_delay_final_countdown_seconds).state);",
                "            };",
                "            cfg.alarm_delay_audio.play_beep = [](AlarmDelayAudioMode mode) {",
                "              id(play_alarm_delay_beep).execute(mode == AlarmDelayAudioMode::ENTRY);",
                "            };",
                "            cfg.alarm_delay_audio.announce = [](AlarmDelayAudioMode mode) {",
                "              id(announce_alarm_delay).execute(mode == AlarmDelayAudioMode::ENTRY);",
                "            };",
                "            cfg.alarm_delay_audio.stop = []() {",
                "              id(stop_alarm_delay_audio).execute();",
                "            };",
            ]
        )
    if image_card_count > 0:
        lines.append("            static esphome::artwork_image::ArtworkImage *image_card_downloaders[] = {")
        for num in range(1, image_card_count + 1):
            lines.append(f"              id(image_card_download_{num}),")
        lines.append("            };")
        lines.append("            cfg.image_card_images = image_card_downloaders;")
        lines.append("            cfg.image_card_modal_image = id(image_card_modal_download_1);")
        lines.append(f"            cfg.image_card_image_count = {image_card_count};")
    if device.get("image_card_diagnostics"):
        lines.append("            cfg.image_card_diagnostics = true;")
    lines.append("            cfg.home_assistant_base_url = []() {")
    lines.append("              std::string base = id(cover_art_home_assistant_base_url);")
    lines.append("              while (!base.empty() && base.back() == '/') base.pop_back();")
    lines.append("              return base;")
    lines.append("            };")
    lines.append("            register_webhook_sender([](const std::string &url, const std::string &method, const std::string &body, const std::vector<esphome::http_request::Header> &headers) {")
    lines.append("              auto response = id(http_req).start(url, method, body, headers);")
    lines.append("              if (response == nullptr) {")
    lines.append("                ESP_LOGW(\"webhook\", \"Webhook request failed to start: %s\", url.c_str());")
    lines.append("                return false;")
    lines.append("              }")
    lines.append("              int status = response->status_code;")
    lines.append("              response->end();")
    lines.append("              if (!esphome::http_request::is_success(status)) {")
    lines.append("                ESP_LOGW(\"webhook\", \"Webhook %s returned HTTP %d\", url.c_str(), status);")
    lines.append("                return false;")
    lines.append("              }")
    lines.append("              ESP_LOGI(\"webhook\", \"Webhook %s returned HTTP %d\", url.c_str(), status);")
    lines.append("              return true;")
    lines.append("            });")
    lines.append("            set_width_compensation_vertical_axis(cfg.width_compensation_vertical);")
    lines.append("            apply_width_compensation(id(display_time), cfg.width_compensation_percent);")
    lines.append("            apply_width_compensation(id(temperatures), cfg.width_compensation_percent);")
    lines.append("            apply_width_compensation(id(clock_label), cfg.width_compensation_percent);")
    return lines


def phase1_block(device: dict) -> str:
    lines = [
        "            // BEGIN GENERATED PHASE 1 GRID WIRING",
        "            // Generated by scripts/generate_device_slots.py from devices/manifest.json.",
        f"            {button_slot_macro()}",
        "            BtnSlot slots[] = {",
    ]
    for num in range(1, device["slots"] + 1):
        lines.append(f"              BTN_SLOT({num}),")
    lines.extend(
        [
            "            };",
            "            #undef BTN_SLOT",
            *cfg_lines(device),
            "            // END GENERATED PHASE 1 GRID WIRING",
        ]
    )
    return "\n".join(lines)


def refresh_block(device: dict) -> str:
    lines = [
        "          // BEGIN GENERATED REFRESH GRID WIRING",
        "          // Generated by scripts/generate_device_slots.py from devices/manifest.json.",
        f"          {button_slot_macro()}",
        "          BtnSlot slots[] = {",
    ]
    for num in range(1, device["slots"] + 1):
        lines.append(f"            BTN_SLOT({num}),")
    lines.extend(
        [
            "          };",
            "          #undef BTN_SLOT",
            "          if (!id(screen_rotation_ready)) return;",
            *["          " + line[12:] if line.startswith("            ") else line for line in cfg_lines(device)],
            "          // END GENERATED REFRESH GRID WIRING",
        ]
    )
    return "\n".join(lines)


def refresh_subpage_arrays(device: dict) -> list[str]:
    package = device.get("package") or {}
    subpage_chunks = int(package.get("subpageConfigChunks") or 8)
    indent = "          "
    lines = [
        "          #define SP_CFG(n) subpage_##n##_config",
        "          #define SP_EXT(n) subpage_##n##_config_ext",
        "          #define SP_EXT2(n) subpage_##n##_config_ext_2",
        "          #define SP_EXT3(n) subpage_##n##_config_ext_3",
    ]
    if subpage_chunks >= 8:
        lines.extend(
            [
                "          #define SP_EXT4(n) subpage_##n##_config_ext_4",
                "          #define SP_EXT5(n) subpage_##n##_config_ext_5",
                "          #define SP_EXT6(n) subpage_##n##_config_ext_6",
                "          #define SP_EXT7(n) subpage_##n##_config_ext_7",
            ]
        )
    lines.extend(macro_array("sp_cfgs", "SP_CFG", device["slots"], indent=indent))
    lines.extend(macro_array("sp_ext", "SP_EXT", device["slots"], indent=indent))
    lines.extend(macro_array("sp_ext2", "SP_EXT2", device["slots"], indent=indent))
    lines.extend(macro_array("sp_ext3", "SP_EXT3", device["slots"], indent=indent))
    if subpage_chunks >= 8:
        lines.extend(macro_array("sp_ext4", "SP_EXT4", device["slots"], indent=indent))
        lines.extend(macro_array("sp_ext5", "SP_EXT5", device["slots"], indent=indent))
        lines.extend(macro_array("sp_ext6", "SP_EXT6", device["slots"], indent=indent))
        lines.extend(macro_array("sp_ext7", "SP_EXT7", device["slots"], indent=indent))
    lines.extend(
        [
            "          #undef SP_CFG",
            "          #undef SP_EXT",
            "          #undef SP_EXT2",
            "          #undef SP_EXT3",
        ]
    )
    if subpage_chunks >= 8:
        lines.extend(
            [
                "          #undef SP_EXT4",
                "          #undef SP_EXT5",
                "          #undef SP_EXT6",
                "          #undef SP_EXT7",
            ]
        )
    return lines


def phase2_block(device: dict) -> str:
    package = device.get("package") or {}
    subpage_chunks = int(package.get("subpageConfigChunks") or 8)
    lines = [
        "            // BEGIN GENERATED PHASE 2 GRID WIRING",
        "            // Generated by scripts/generate_device_slots.py from devices/manifest.json.",
        f"            {button_slot_macro()}",
        "            #define SP_CFG(n) subpage_##n##_config",
        "            #define SP_EXT(n) subpage_##n##_config_ext",
        "            #define SP_EXT2(n) subpage_##n##_config_ext_2",
        "            #define SP_EXT3(n) subpage_##n##_config_ext_3",
    ]
    if subpage_chunks >= 8:
        lines.extend(
            [
                "            #define SP_EXT4(n) subpage_##n##_config_ext_4",
                "            #define SP_EXT5(n) subpage_##n##_config_ext_5",
                "            #define SP_EXT6(n) subpage_##n##_config_ext_6",
                "            #define SP_EXT7(n) subpage_##n##_config_ext_7",
            ]
        )
    lines.append("            BtnSlot slots[] = {")
    for num in range(1, device["slots"] + 1):
        lines.append(f"              BTN_SLOT({num}),")
    lines.append("            };")
    lines.extend(macro_array("sp_cfgs", "SP_CFG", device["slots"]))
    lines.extend(macro_array("sp_ext", "SP_EXT", device["slots"]))
    lines.extend(macro_array("sp_ext2", "SP_EXT2", device["slots"]))
    lines.extend(macro_array("sp_ext3", "SP_EXT3", device["slots"]))
    if subpage_chunks >= 8:
        lines.extend(macro_array("sp_ext4", "SP_EXT4", device["slots"]))
        lines.extend(macro_array("sp_ext5", "SP_EXT5", device["slots"]))
        lines.extend(macro_array("sp_ext6", "SP_EXT6", device["slots"]))
        lines.extend(macro_array("sp_ext7", "SP_EXT7", device["slots"]))
    lines.extend(
        [
            "            #undef BTN_SLOT",
            "            #undef SP_CFG",
            "            #undef SP_EXT",
            "            #undef SP_EXT2",
            "            #undef SP_EXT3",
            *cfg_lines(device),
            "            // END GENERATED PHASE 2 GRID WIRING",
        ]
    )
    if subpage_chunks >= 8:
        insert_at = lines.index("            #undef SP_EXT3") + 1
        lines[insert_at:insert_at] = [
            "            #undef SP_EXT4",
            "            #undef SP_EXT5",
            "            #undef SP_EXT6",
            "            #undef SP_EXT7",
        ]
    return "\n".join(lines)


def script_block(device: dict) -> str:
    after_refresh = ["      - script.execute: clock_bar_apply"]
    if device.get("refresh_rebuilds_subpages"):
        package = device.get("package") or {}
        subpage_chunks = int(package.get("subpageConfigChunks") or 8)
        phase2_call = [
            "          grid_phase2(slots, cfg, sp_cfgs, sp_ext, sp_ext2, sp_ext3, sp_ext4, sp_ext5, sp_ext6, sp_ext7,"
            if subpage_chunks >= 8
            else "          grid_phase2(slots, cfg, sp_cfgs, sp_ext, sp_ext2, sp_ext3,",
            "            id(button_order).state,",
            "            id(button_on_color).state,",
            "            id(main_page)->obj);",
        ]
        return "\n".join(
            [
                "script:",
                "  - id: refresh_button_grid",
                "    mode: restart",
                "    then:",
                "      - delay: 3s",
                "      - lambda: |-",
                refresh_block(device),
                *refresh_subpage_arrays(device),
                "          grid_refresh_layout(slots, cfg,",
                "            id(button_order).state,",
                "            id(main_page)->obj);",
                "          navigation_return_home(id(main_page)->obj);",
                *phase2_call,
                *after_refresh,
                "",
            ]
        )
    return "\n".join(
        [
            "script:",
            "  - id: refresh_button_grid",
            "    mode: restart",
            "    then:",
            "      - delay: 3s",
            "      - lambda: |-",
            refresh_block(device),
            "          grid_refresh_layout(slots, cfg,",
            "            id(button_order).state,",
            "            id(main_page)->obj);",
            *after_refresh,
            "",
        ]
    )


def replace_phase(text: str, phase: int, block: str, call: str, slug: str) -> str:
    marker = re.compile(
        rf"(?ms)^            // BEGIN GENERATED PHASE {phase} GRID WIRING\n"
        rf".*?^            // END GENERATED PHASE {phase} GRID WIRING"
    )
    if marker.search(text):
        return marker.sub(block, text, count=1)

    legacy = re.compile(
        rf"(?ms)(        # Phase {phase}:.*?\n        - lambda: \|-\n)(.*?)(            {call}\()"
    )
    next_text, count = legacy.subn(lambda m: m.group(1) + block + "\n" + m.group(3), text, count=1)
    if count != 1:
        raise ValueError(f"Could not find phase {phase} wiring block for {slug}")
    return next_text


def replace_script_block(text: str, device: dict) -> str:
    block = script_block(device)
    marker = re.compile(r"(?ms)^script:\n.*?(?=^esphome:)")
    if marker.search(text):
        return marker.sub(block + "\n", text, count=1)
    insert_at = text.find("\nesphome:")
    if insert_at < 0:
        raise ValueError(f"Could not find esphome block for {device['slug']}")
    return text[: insert_at + 1] + block + text[insert_at + 1 :]


def replace_sensor_blocks(text: str, device: dict) -> str:
    text = replace_script_block(text, device)
    text = replace_phase(text, 1, phase1_block(device), "grid_phase1", device["slug"])
    text = replace_phase(text, 2, phase2_block(device), "grid_phase2", device["slug"])
    text = re.sub(
        r"(?m)^              id\(button_on_color\)\.state,\n"
        r"              id\(button_off_color\)\.state,\n"
        r"              id\(sensor_card_color\)\.state,\n"
        r"              id\(main_page\)->obj\);$",
        "              id(button_on_color).state,\n              id(main_page)->obj);",
        text,
    )
    text = re.sub(
        r"(?m)^            id\(button_on_color\)\.state,\n"
        r"            id\(button_off_color\)\.state,\n"
        r"            id\(sensor_card_color\)\.state,\n"
        r"            id\(main_page\)->obj\);$",
        "            id(button_on_color).state,\n            id(main_page)->obj);",
        text,
    )
    text = re.sub(
        r"(?m)^(              temperature_labels,\n)              6,",
        r"\1              1,",
        text,
    )
    return text


def update(path: Path, new_text: str, check: bool, changed: list[Path]) -> None:
    old_text = path.read_text(encoding="utf-8")
    if old_text == new_text:
        return
    changed.append(path)
    if not check:
        path.write_text(new_text, encoding="utf-8")


def assert_marker_pair(text: str, path: Path, start: str, end: str) -> None:
    start_count = text.count(start)
    end_count = text.count(end)
    if start_count != 1 or end_count != 1 or text.find(start) > text.find(end):
        rel = path.relative_to(ROOT)
        raise ValueError(f"{rel} must contain one ordered generated block: {start} / {end}")


def assert_optional_marker_pair(text: str, path: Path, start: str, end: str) -> None:
    if start not in text and end not in text:
        return
    assert_marker_pair(text, path, start, end)


def assert_generated_block_markers(package_path: Path, sensor_path: Path) -> None:
    package_text = package_path.read_text(encoding="utf-8")
    sensor_text = sensor_path.read_text(encoding="utf-8")
    assert_marker_pair(package_text, package_path, "BEGIN GENERATED BUTTON PACKAGES", "END GENERATED BUTTON PACKAGES")
    assert_optional_marker_pair(sensor_text, sensor_path, "BEGIN GENERATED REFRESH GRID WIRING", "END GENERATED REFRESH GRID WIRING")
    assert_marker_pair(sensor_text, sensor_path, "BEGIN GENERATED PHASE 1 GRID WIRING", "END GENERATED PHASE 1 GRID WIRING")
    assert_marker_pair(sensor_text, sensor_path, "BEGIN GENERATED PHASE 2 GRID WIRING", "END GENERATED PHASE 2 GRID WIRING")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail if generated YAML is stale")
    args = parser.parse_args()

    changed: list[Path] = []
    for device in slot_devices():
        slug = device["slug"]
        package_path = ROOT / "devices" / slug / "packages.yaml"
        sensor_path = ROOT / "devices" / slug / "device" / "sensors.yaml"
        try:
            assert_generated_block_markers(package_path, sensor_path)
        except ValueError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 1

        update(package_path, package_file_text(device), args.check, changed)

        sensor_text = sensor_path.read_text(encoding="utf-8")
        update(sensor_path, replace_sensor_blocks(sensor_text, device), args.check, changed)

    if args.check and changed:
        for path in changed:
            print(f"Generated YAML is stale: {path.relative_to(ROOT)}", file=sys.stderr)
        return 1

    if changed:
        for path in changed:
            print(f"updated {path.relative_to(ROOT)}")
    else:
        print("Device YAML is up to date.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
