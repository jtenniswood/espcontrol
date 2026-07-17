#!/usr/bin/env python3
"""Guard firmware card metadata access behind button_grid_card_runtime.h."""

from __future__ import annotations

import argparse
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = ROOT / "components" / "espcontrol"

CARD_RUNTIME_BOUNDARY_FILES = {
    "button_grid_card_runtime.h",
    "button_grid_contract_generated.h",
}
CONTRACT_INCLUDE_ALLOWLIST = {
    "button_grid_card_runtime.h",
}

# Existing alarm UI code keeps a few local button-order arrays. The runtime
# guard still blocks direct generated-contract access in that file.
MODE_ARRAY_ALLOWLIST = CARD_RUNTIME_BOUNDARY_FILES | {
    "button_grid_alarm.h",
    # Generated from the same contract and compiled only by the parity test.
    "button_grid_saved_config_shadow_generated.h",
}

DIRECT_CONTRACT_PATTERN = re.compile(r"\b(?:card_contract_[A-Za-z0-9_]+|CARD_CONTRACT_[A-Z0-9_]+)\b")
CONTRACT_INCLUDE_PATTERN = re.compile(r'#\s*include\s+[<"]button_grid_contract_generated\.h[>"]')
MODE_ARRAY_PATTERN = re.compile(
    r"\{[^}\n]*\"(?:play_pause|previous|next|volume|position|now_playing|cover_art|"
    r"open|close|stop|set_position|tilt|toggle|lock|unlock|away|home|night|vacation|disarm)\""
)
SERVICE_MAPPING_PATTERN = re.compile(
    r"\"(?:cover\.(?:open_cover|close_cover|stop_cover|set_cover_position|"
    r"open_cover_tilt|close_cover_tilt|stop_cover_tilt|set_cover_tilt_position)|"
    r"lock\.(?:lock|unlock)|"
    r"media_player\.(?:media_play_pause|media_previous_track|media_next_track)|"
    r"alarm_control_panel\.(?:alarm_arm_away|alarm_arm_home|alarm_arm_night|alarm_arm_vacation|alarm_disarm))\""
)

LAWN_MOWER_HEADER = "button_grid_lawn_mower.h"
GRID_HEADER = "button_grid_grid.h"
ACTION_HEADER = "button_grid_actions.h"
IMAGE_HEADER = "button_grid_image.h"
STATUS_ENTITY_HEADER = "button_grid_status_entity_driver.h"
DATE_TIME_HEADER = "button_grid_date_time_driver.h"
DATE_TIME_CARDS_HEADER = "button_grid_datetime_cards.h"
SENSOR_HEADER = "button_grid_sensor_driver.h"
WEATHER_HEADER = "button_grid_weather_driver.h"
BASIC_ACTION_HEADER = "button_grid_basic_action_driver.h"
NUMERIC_SELECTABLE_HEADER = "button_grid_numeric_selectable_driver.h"
CLEANING_HEADER = "button_grid_cleaning_driver.h"
ACCESS_COVER_HEADER = "button_grid_access_cover_driver.h"
COVER_MODAL_DRIVER_HEADER = "button_grid_cover_modal_driver.h"
MEDIA_DRIVER_HEADER = "button_grid_media_driver.h"
LEGACY_COMPATIBILITY_DRIVER_HEADER = "button_grid_legacy_compatibility_driver.h"
NAVIGATION_DRIVER_HEADER = "button_grid_navigation_driver.h"
IMAGE_DRIVER_HEADER = "button_grid_image_driver.h"
LIGHT_CONTROL_DRIVER_HEADER = "button_grid_light_control_driver.h"
FAN_CONTROL_DRIVER_HEADER = "button_grid_fan_control_driver.h"
CLIMATE_CONTROL_DRIVER_HEADER = "button_grid_climate_control_driver.h"
ALARM_DRIVER_HEADER = "button_grid_alarm_driver.h"
CARDS_HEADER = "button_grid_cards.h"


def service_mapping_line_allowed(line: str) -> bool:
    if "ESP_LOGW" in line:
        return True
    if "cover." in line and "_tilt" in line:
        return True
    return False


def firmware_headers(root: Path) -> list[Path]:
    return sorted((root / "components" / "espcontrol").glob("button_grid*.h"))


def function_body(text: str, name: str) -> str | None:
    match = re.search(rf"\b{name}\s*\([^)]*\)\s*\{{", text)
    if not match:
        return None
    start = match.end() - 1
    depth = 0
    for index in range(start, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:index]
    return None


def check_root(root: Path) -> list[str]:
    failures: list[str] = []
    for path in firmware_headers(root):
        filename = path.name
        rel = path.relative_to(root)
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            if filename not in CONTRACT_INCLUDE_ALLOWLIST and CONTRACT_INCLUDE_PATTERN.search(line):
                failures.append(f"{rel}:{line_no}: include generated card contract through button_grid_card_runtime.h")
            if filename not in CARD_RUNTIME_BOUNDARY_FILES and DIRECT_CONTRACT_PATTERN.search(line):
                failures.append(f"{rel}:{line_no}: access generated card contract through button_grid_card_runtime.h")
            if filename not in MODE_ARRAY_ALLOWLIST and MODE_ARRAY_PATTERN.search(line):
                failures.append(f"{rel}:{line_no}: keep shared card mode lists in the card runtime/contract boundary")
            if (
                filename not in CARD_RUNTIME_BOUNDARY_FILES
                and SERVICE_MAPPING_PATTERN.search(line)
                and not service_mapping_line_allowed(line)
            ):
                failures.append(f"{rel}:{line_no}: keep shared card service mappings in the card runtime/contract boundary")
    runtime_header = root / "components" / "espcontrol" / "button_grid_card_runtime.h"
    if runtime_header.exists():
        text = runtime_header.read_text(encoding="utf-8")
        if "struct Context" in text and "driver_uses_legacy_dispatch" in text:
            failures.append(
                "components/espcontrol/button_grid_card_runtime.h: retire the broad driver fallback classifier"
            )
        if (
            "struct Context" in text
            and (text.count("context.legacy_dispatch = true;") != 1 or 'type == "todo"' not in text)
        ):
            failures.append(
                "components/espcontrol/button_grid_card_runtime.h: keep exactly one explicit Todo compatibility path"
            )
        for raw_alias in ('type == "local"', 'type == "text_sensor"'):
            if raw_alias in text:
                failures.append(
                    "components/espcontrol/button_grid_card_runtime.h: resolve saved-config aliases before runtime dispatch"
                )
    mower_header = root / "components" / "espcontrol" / LAWN_MOWER_HEADER
    if mower_header.exists():
        text = mower_header.read_text(encoding="utf-8")
        required = (
            "lawn_mower.start_mowing",
            "lawn_mower.pause",
            "lawn_mower.dock",
            'ctx->state == "mowing"',
            'ctx->state == "unavailable" || ctx->state == "unknown"',
        )
        for needle in required:
            if needle not in text:
                failures.append(f"components/espcontrol/{LAWN_MOWER_HEADER}: missing mower runtime guard {needle}")
        forbidden = (
            "vacuum.",
            "lawn_mower.stop",
            "lawn_mower.locate",
            "lawn_mower.clean_spot",
            "lawn_mower.clean_area",
        )
        for needle in forbidden:
            if needle in text:
                failures.append(f"components/espcontrol/{LAWN_MOWER_HEADER}: unexpected mower service/reference {needle}")
    grid_header = root / "components" / "espcontrol" / GRID_HEADER
    if grid_header.exists():
        text = grid_header.read_text(encoding="utf-8")
        compact_grid = re.sub(r"\s+", " ", text)
        visual_setup = function_body(text, "setup_card_visual")
        if visual_setup is not None:
            clickable_reset = "lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);"
            reset_index = visual_setup.find(clickable_reset)
            driver_index = visual_setup.find("if (espcontrol::cards::")
            if reset_index < 0 or driver_index < 0 or reset_index > driver_index:
                failures.append(
                    f"components/espcontrol/{GRID_HEADER}: restore persistent button clickability before visual driver dispatch"
                )
            unsupported_clear = "clear_unsupported_card_slot_visuals(s);"
            unsupported_clear_index = visual_setup.find(unsupported_clear)
            unsupported_warning_index = visual_setup.find(
                'ESP_LOGW("card_runtime", "Unsupported card type has no visual driver:'
            )
            if (
                unsupported_clear_index < 0
                or unsupported_warning_index < 0
                or unsupported_clear_index > unsupported_warning_index
            ):
                failures.append(
                    f"components/espcontrol/{GRID_HEADER}: clear stale slot visuals before leaving unsupported cards inert"
                )
        unsupported_clear_body = function_body(text, "clear_unsupported_card_slot_visuals")
        if unsupported_clear_body is None or any(
            f"lv_label_set_text(s.{label}, \"\");" not in unsupported_clear_body
            for label in ("icon_lbl", "text_lbl", "sensor_lbl", "unit_lbl")
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: clear every persistent label for unsupported cards"
            )
        if (
            "card_runtime_context(p)" not in text
            or "card_runtime_information_only(context)" not in text
            or "espcontrol::cards::Surface::SUBPAGE" not in text
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: route main and subpage setup through the shared card context"
            )
        if (
            "status_entity_driver_setup_visual( s, p, context, palette)" not in compact_grid
            or "status_entity_driver_bind_data( s, p, context, palette)" not in compact_grid
            or "date_time_driver_setup_visual( s, p, context, palette)" not in compact_grid
            or "date_time_driver_bind_data(s, p, context)" not in compact_grid
            or "sensor_driver_setup_visual( s, p, context, palette)" not in compact_grid
            or "sensor_driver_bind_data( s, p, context, palette)" not in compact_grid
            or "weather_driver_setup_visual( s, p, context, palette, display)" not in compact_grid
            or "weather_driver_bind_data(s, p, context)" not in compact_grid
            or "basic_action_driver_setup_visual(s, p, context)" not in compact_grid
            or "basic_action_driver_bind_main( s, p, context, cfg, palette, display, main_page_obj, COLS, toggle_state)" not in compact_grid
            or "basic_action_driver_bind_subpage( sub_slot, sb_cfg, context, action_environment)" not in compact_grid
            or "numeric_selectable_driver_setup_visual( s, p, context, palette, display)" not in compact_grid
            or "numeric_selectable_driver_bind_main( s, p, context, palette, display)" not in compact_grid
            or "numeric_selectable_driver_bind_subpage( sub_slot, sb_cfg, context, numeric_environment)" not in compact_grid
            or "cleaning_driver_setup_visual(s, p, context)" not in compact_grid
            or "cleaning_driver_bind_main( s, p, context)" not in compact_grid
            or "cleaning_driver_bind_subpage( sub_slot, sb_cfg, context, cleaning_environment)" not in compact_grid
            or "access_cover_driver_setup_visual( s, p, context, palette)" not in compact_grid
            or "access_cover_driver_bind_main( s, p, context)" not in compact_grid
            or "access_cover_driver_bind_subpage( sub_slot, sb_cfg, context, access_cover_environment)" not in compact_grid
            or "cover_modal_driver_setup_visual(s, p, context)" not in compact_grid
            or "cover_modal_driver_bind_main( s, p, context, cover_modal_environment)" not in compact_grid
            or "cover_modal_driver_bind_subpage( sub_slot, sb_cfg, context, cover_modal_environment)" not in compact_grid
            or "media_driver_setup_visual( s, p, context, palette, display, row_span, col_span)" not in compact_grid
            or "media_driver_bind_main( s, p, context, media_environment)" not in compact_grid
            or "media_driver_bind_subpage( sub_slot, sb_cfg, context, media_environment)" not in compact_grid
            or "legacy_compatibility_driver_setup_visual( s, p, context, palette, display, row_span, col_span)" not in compact_grid
            or "legacy_compatibility_driver_bind( s, p, context, palette, display, row_span, col_span)" not in compact_grid
            or "legacy_compatibility_driver_bind( sub_slot, sb_cfg, context, palette, display, rs, cs)" not in compact_grid
            or "navigation_driver_setup_visual( s, p, context, cfg, display)" not in compact_grid
            or "navigation_driver_bind_main( s, p, context, navigation_state)" not in compact_grid
            or "navigation_driver_own_subpage( slots[si], p, parent_context, si + 1, display_order, sub_scr)" not in compact_grid
            or "image_driver_setup_visual(s, p, context)" not in compact_grid
            or "image_driver_bind_main( s, p, context, cfg)" not in compact_grid
            or "image_driver_bind_subpage( sub_slot, sb_cfg, context, cfg)" not in compact_grid
            or "light_control_driver_setup_visual(s, p, context)" not in compact_grid
            or "light_control_driver_bind_main( s, p, context, light_control_environment)" not in compact_grid
            or "light_control_driver_bind_subpage( sub_slot, sb_cfg, context, light_control_environment)" not in compact_grid
            or "fan_control_driver_setup_visual(s, p, context)" not in compact_grid
            or "fan_control_driver_bind_main( s, p, context, fan_control_environment)" not in compact_grid
            or "fan_control_driver_bind_subpage( sub_slot, sb_cfg, context, fan_control_environment)" not in compact_grid
            or "climate_control_driver_setup_visual( s, p, context, display)" not in compact_grid
            or "climate_control_driver_bind_main( s, p, context, climate_control_environment)" not in compact_grid
            or "climate_control_driver_bind_subpage( sub_slot, sb_cfg, context, climate_control_environment)" not in compact_grid
            or "alarm_driver_setup_visual(s, p, context)" not in compact_grid
            or "alarm_driver_bind_main( s, p, context, alarm_environment)" not in compact_grid
            or "alarm_driver_bind_subpage( sub_slot, sb_cfg, context, alarm_environment)" not in compact_grid
            or "bind_basic_sensor_card(s, p, context, palette)" not in compact_grid
            or "bind_basic_sensor_card(sub_slot, sb_cfg, context, palette)" not in compact_grid
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: route main and subpage migrated cards through shared drivers"
            )
        for direct_branch in (
            'p.type == "door_window"', 'p.type == "presence"',
            'p.type == "clock"', 'p.type == "timezone"',
            'p.type == "calendar"', 'p.type == "sensor"',
            'p.type == "local_sensor"', 'p.type == "text_sensor"',
            'p.type == "weather"', 'p.type == "weather_forecast"',
            'sb_cfg.type == "calendar"', 'sb_cfg.type == "sensor"',
            'sb_cfg.type == "local_sensor"', 'sb_cfg.type == "text_sensor"',
            'sb_cfg.type == "weather"', 'sb_cfg.type == "weather_forecast"',
            'p.type == "screen_lock"', 'p.type == "push"',
            'p.type == "alarm_action"', 'p.type == "internal"',
            'p.type == "local"', 'p.type == "webhook"',
            'sb_cfg.type == "screen_lock"', 'sb_cfg.type == "push"',
            'sb_cfg.type == "alarm_action"', 'sb_cfg.type == "internal"',
            'sb_cfg.type == "local"', 'sb_cfg.type == "webhook"',
            'p.type == "slider"', 'p.type == "light_brightness"',
            'p.type == "light_temperature"', 'p.type == "fan_speed"',
            'p.type == "fan_oscillate"', 'p.type == "fan_direction"',
            'p.type == "fan_preset"', 'p.type == "option_select"',
            'sb_cfg.type == "slider"', 'sb_cfg.type == "light_brightness"',
            'sb_cfg.type == "light_temperature"', 'sb_cfg.type == "fan_speed"',
            'sb_cfg.type == "fan_oscillate"', 'sb_cfg.type == "fan_direction"',
            'sb_cfg.type == "fan_preset"', 'sb_cfg.type == "option_select"',
            'family == espcontrol::cards::Family::VACUUM',
            'family == espcontrol::cards::Family::MOWER',
            'p.type == "garage"', 'p.type == "gate"', 'p.type == "lock"',
            'sb_cfg.type == "garage"', 'sb_cfg.type == "gate"',
            'sb_cfg.type == "lock"',
            'if (p.type == "cover") {',
            'if (sb_cfg.type == "cover") {',
            'p.type == "cover" && cover_command_mode',
            'p.type == "cover" && cover_toggle_mode',
            'sb_cfg.type == "cover" && cover_command_mode',
            'sb_cfg.type == "cover" && cover_toggle_mode',
            'p.type == "cover" && cover_modal_mode',
            'sb_cfg.type == "cover" && cover_modal_mode',
            'p.type == "subpage"', 'p.type != "subpage"',
            'p.type == "image"', 'sb_cfg.type == "image"',
            'family == espcontrol::cards::Family::LIGHT_CONTROL',
            'p.type == "light_control"', 'sb_cfg.type == "light_control"',
            'p.type == "fan_control"', 'sb_cfg.type == "fan_control"',
            'family == espcontrol::cards::Family::CLIMATE',
            'family == espcontrol::cards::Family::ALARM',
            'family == espcontrol::cards::Family::MEDIA',
            'p.type == "media"', 'sb_cfg.type == "media"',
        ):
            if direct_branch in text:
                failures.append(
                    f"components/espcontrol/{GRID_HEADER}: keep migrated type overrides inside shared drivers"
                )
        for retired_fallback in (
            "Legacy setup fallback",
            "family == espcontrol::cards::Family::TODO",
            "create_todo_card_context(",
            "subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,",
            "bool switch_has_sensor = !sb_cfg.sensor.empty();",
        ):
            if retired_fallback in text:
                failures.append(
                    f"components/espcontrol/{GRID_HEADER}: broad legacy card fallback must remain retired ({retired_fallback})"
                )
        if (
            "cleaning_environment.add_mower_parent_indicator" not in text
            or "lawn_mower_state_active_ref" not in text
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: route mower subpage parent indicators through mower active-state handling"
            )
        if (
            "light_control_environment.add_parent_indicator" not in text
            or "light_control_driver_bind_subpage(" not in text
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: include full light controls in generic subpage parent indicators"
            )
        if (
            "fan_control_environment.add_parent_indicator" not in text
            or "fan_control_driver_bind_subpage(" not in text
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: include full fan controls in generic subpage parent indicators"
            )
        image_reset_pos = text.find("image_driver_reset_pool(cfg);")
        subpage_clear_pos = text.find("navigation_clear_subpages();")
        if image_reset_pos < 0 or subpage_clear_pos < 0 or image_reset_pos > subpage_clear_pos:
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: reset image-card contexts before deleting subpage screens"
            )
    action_header = root / "components" / "espcontrol" / ACTION_HEADER
    if action_header.exists():
        text = action_header.read_text(encoding="utf-8")
        click_body = function_body(text, "handle_button_click")
        if click_body is not None and (
            "card_runtime_context(p)" not in click_body
            or "card_runtime_passive(context)" not in click_body
            or "basic_action_driver_handle_main_click(" not in click_body
            or "numeric_selectable_driver_handle_main_click(" not in click_body
            or "cleaning_driver_handle_main_click(" not in click_body
            or "access_cover_driver_handle_main_click(" not in click_body
            or "cover_modal_driver_handle_main_click(" not in click_body
            or "navigation_driver_handle_main_click(" not in click_body
            or "image_driver_handle_main_click(" not in click_body
            or "light_control_driver_handle_main_click(" not in click_body
            or "fan_control_driver_handle_main_click(" not in click_body
            or "climate_control_driver_handle_main_click(" not in click_body
            or "alarm_driver_handle_main_click(" not in click_body
            or "media_driver_handle_main_click(" not in click_body
            or "legacy_compatibility_driver_handle_main_click(" not in click_body
        ):
            failures.append(
                f"components/espcontrol/{ACTION_HEADER}: route passive checks through the shared card context"
            )
        if click_body is not None:
            for direct_branch in (
                'p.type == "garage"',
                'p.type == "gate"',
                'p.type == "lock"',
                'p.type == "cover" && cover_command_mode',
                'p.type == "cover" && cover_toggle_mode',
                'else if (p.type == "cover")',
                'p.type == "cover" && cover_modal_mode',
                'p.type == "subpage"',
                'p.type == "image"',
                'p.type == "light_control"',
                'p.type == "fan_control"',
                'climate_card_type(p.type)',
                'p.type == "alarm"',
                'p.type == "media"',
            ):
                if direct_branch in click_body:
                    failures.append(
                        f"components/espcontrol/{ACTION_HEADER}: keep migrated clicks inside shared drivers"
                    )
            for retired_fallback in (
                "Legacy action fallback",
                'p.type == "todo"',
                "send_toggle_action(p.entity)",
            ):
                if retired_fallback in click_body:
                    failures.append(
                        f"components/espcontrol/{ACTION_HEADER}: broad legacy action fallback must remain retired ({retired_fallback})"
                    )
    image_header = root / "components" / "espcontrol" / IMAGE_HEADER
    if image_header.exists():
        text = image_header.read_text(encoding="utf-8")
        reset_body = function_body(text, "reset_image_card_pool")
        if reset_body is None or "for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++)" not in reset_body:
            failures.append(
                f"components/espcontrol/{IMAGE_HEADER}: reset every image-card context, including disabled slots"
            )
    status_entity_header = root / "components" / "espcontrol" / STATUS_ENTITY_HEADER
    if status_entity_header.exists():
        text = status_entity_header.read_text(encoding="utf-8")
        required = (
            "status_entity_driver_setup_visual",
            "status_entity_driver_bind_data",
            "status_entity_driver_attach_interaction",
            "status_entity_driver_refresh_layout",
            "status_entity_driver_cleanup",
            "case Type::DOOR_WINDOW: return is_entity_on_ref(state);",
            "case Type::PRESENCE: return presence_detected_ref(state);",
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{STATUS_ENTITY_HEADER}: missing shared status-entity lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{STATUS_ENTITY_HEADER}: missing shared status-entity driver"
        )
    date_time_header = root / "components" / "espcontrol" / DATE_TIME_HEADER
    if date_time_header.exists():
        text = date_time_header.read_text(encoding="utf-8")
        required = (
            "date_time_driver_setup_visual",
            "date_time_driver_bind_data",
            "date_time_driver_attach_interaction",
            "date_time_driver_refresh_layout",
            "date_time_driver_cleanup",
            "context.runtime.type == Type::CALENDAR",
            "context.runtime.type == Type::CLOCK",
            "context.runtime.type == Type::TIMEZONE",
            "register_calendar_card",
            "subscribe_calendar_date_source",
            "register_timezone_card",
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{DATE_TIME_HEADER}: missing shared date-time lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{DATE_TIME_HEADER}: missing shared date-time driver"
        )
    date_time_cards_header = root / "components" / "espcontrol" / DATE_TIME_CARDS_HEADER
    if date_time_cards_header.exists():
        text = date_time_cards_header.read_text(encoding="utf-8")
        for legacy_setup in ("setup_calendar_card", "setup_clock_card", "setup_timezone_card"):
            if legacy_setup in text:
                failures.append(
                    f"components/espcontrol/{DATE_TIME_CARDS_HEADER}: keep {legacy_setup} inside the shared date-time driver"
                )
    sensor_header = root / "components" / "espcontrol" / SENSOR_HEADER
    if sensor_header.exists():
        text = sensor_header.read_text(encoding="utf-8")
        required = (
            "sensor_driver_setup_visual",
            "sensor_driver_bind_data",
            "sensor_driver_attach_interaction",
            "sensor_driver_refresh_layout",
            "sensor_driver_cleanup",
            "sensor_driver_register_local_value",
            "subscribe_sensor_text_card_value",
            "subscribe_sensor_icon_state",
            "subscribe_time_sensor_value",
            "subscribe_sensor_value",
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{SENSOR_HEADER}: missing shared sensor lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{SENSOR_HEADER}: missing shared sensor driver"
        )
    weather_header = root / "components" / "espcontrol" / WEATHER_HEADER
    if weather_header.exists():
        text = weather_header.read_text(encoding="utf-8")
        required = (
            "weather_driver_setup_visual",
            "weather_driver_bind_data",
            "weather_driver_attach_interaction",
            "weather_driver_refresh_layout",
            "weather_driver_cleanup",
            "register_weather_forecast_card",
            "subscribe_weather_state",
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{WEATHER_HEADER}: missing shared weather lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{WEATHER_HEADER}: missing shared weather driver"
        )
    basic_action_header = root / "components" / "espcontrol" / BASIC_ACTION_HEADER
    if basic_action_header.exists():
        text = basic_action_header.read_text(encoding="utf-8")
        required = (
            "basic_action_driver_setup_visual",
            "basic_action_driver_bind_main",
            "basic_action_driver_bind_subpage",
            "basic_action_driver_attach_interaction",
            "basic_action_driver_refresh_layout",
            "basic_action_driver_cleanup",
            "basic_action_driver_handle_main_click",
            "basic_action_driver_bind_toggle",
            "basic_action_driver_bind_action_state",
            "basic_action_driver_bind_alarm_action",
            "basic_action_driver_bind_fan_switch",
            "send_webhook_action",
            "send_internal_relay_action",
            "screen_lock_toggle",
            "esphome.push_button_pressed",
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{BASIC_ACTION_HEADER}: missing shared basic-action lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{BASIC_ACTION_HEADER}: missing shared basic-action driver"
        )
    numeric_selectable_header = root / "components" / "espcontrol" / NUMERIC_SELECTABLE_HEADER
    if numeric_selectable_header.exists():
        text = numeric_selectable_header.read_text(encoding="utf-8")
        required = (
            "numeric_selectable_driver_setup_visual",
            "numeric_selectable_driver_bind_main",
            "numeric_selectable_driver_bind_subpage",
            "numeric_selectable_driver_attach_interaction",
            "numeric_selectable_driver_refresh_layout",
            "numeric_selectable_driver_cleanup",
            "numeric_selectable_driver_handle_main_click",
            "numeric_selectable_driver_bind_slider",
            "numeric_selectable_driver_bind_light_temperature",
            "numeric_selectable_driver_bind_fan_action",
            "numeric_selectable_driver_bind_option_select",
            "option_select_open_modal",
            "fan_card_handle_click",
            "send_slider_action",
            '"light_brightness"',
            '"light_temperature"',
            '"fan_speed"',
            '"fan_oscillate"',
            '"fan_direction"',
            '"fan_preset"',
            '"option_select"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{NUMERIC_SELECTABLE_HEADER}: missing shared numeric/selectable lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{NUMERIC_SELECTABLE_HEADER}: missing shared numeric/selectable driver"
        )
    cleaning_header = root / "components" / "espcontrol" / CLEANING_HEADER
    if cleaning_header.exists():
        text = cleaning_header.read_text(encoding="utf-8")
        required = (
            "cleaning_driver_setup_visual",
            "cleaning_driver_bind_main",
            "cleaning_driver_bind_subpage",
            "cleaning_driver_attach_interaction",
            "cleaning_driver_refresh_layout",
            "cleaning_driver_cleanup",
            "cleaning_driver_handle_main_click",
            "cleaning_driver_refresh_translated_text",
            "cleaning_driver_refresh_subpage_translated_text",
            "create_vacuum_card_context",
            "subscribe_vacuum_card_state",
            "send_vacuum_card_action",
            "create_lawn_mower_card_context",
            "subscribe_lawn_mower_card_state",
            "send_lawn_mower_card_action",
            '"vacuum"',
            '"lawn_mower"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{CLEANING_HEADER}: missing shared cleaning lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{CLEANING_HEADER}: missing shared cleaning driver"
        )
    access_cover_header = root / "components" / "espcontrol" / ACCESS_COVER_HEADER
    if access_cover_header.exists():
        text = access_cover_header.read_text(encoding="utf-8")
        required = (
            "access_cover_driver_setup_visual",
            "access_cover_driver_bind_main",
            "access_cover_driver_bind_subpage",
            "access_cover_driver_attach_interaction",
            "access_cover_driver_refresh_layout",
            "access_cover_driver_cleanup",
            "access_cover_driver_handle_main_click",
            "bind_garage_status_card",
            "bind_gate_status_card",
            "bind_lock_status_card",
            "subscribe_cover_command_features",
            "subscribe_cover_toggle_state",
            "subscribe_slider_state",
            "send_cover_command_action",
            "send_lock_command_action",
            "send_slider_action",
            '"garage"',
            '"gate"',
            '"lock"',
            '"cover"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{ACCESS_COVER_HEADER}: missing shared access/cover lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{ACCESS_COVER_HEADER}: missing shared access/cover driver"
        )
    cover_modal_driver_header = (
        root / "components" / "espcontrol" / COVER_MODAL_DRIVER_HEADER
    )
    if cover_modal_driver_header.exists():
        text = cover_modal_driver_header.read_text(encoding="utf-8")
        required = (
            "cover_modal_driver_setup_visual",
            "cover_modal_driver_bind_main",
            "cover_modal_driver_bind_subpage",
            "cover_modal_driver_attach_interaction",
            "cover_modal_driver_refresh_layout",
            "cover_modal_driver_cleanup",
            "cover_modal_driver_handle_main_click",
            "create_cover_control_context",
            "subscribe_cover_control_state",
            "cover_control_open_modal",
            "grid_track_cover_control_runtime",
            "grid_delete_cover_control_with_owner",
            '"cover"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{COVER_MODAL_DRIVER_HEADER}: missing shared cover-modal lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{COVER_MODAL_DRIVER_HEADER}: missing shared cover-modal driver"
        )
    media_driver_header = root / "components" / "espcontrol" / MEDIA_DRIVER_HEADER
    if media_driver_header.exists():
        text = media_driver_header.read_text(encoding="utf-8")
        required = (
            "media_driver_setup_visual",
            "media_driver_bind_main",
            "media_driver_bind_subpage",
            "media_driver_attach_interaction",
            "media_driver_refresh_layout",
            "media_driver_cleanup",
            "media_driver_handle_main_click",
            "create_media_control_context",
            "create_media_volume_context",
            "create_media_playlist_context",
            "subscribe_media_control_state",
            "subscribe_media_volume_state",
            "subscribe_media_now_playing_state",
            "subscribe_media_cover_art",
            "subscribe_media_playlist_state",
            "subscribe_media_slider_state",
            "grid_track_media_control_runtime",
            "grid_delete_media_control_with_owner",
            '"media"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{MEDIA_DRIVER_HEADER}: missing shared media lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{MEDIA_DRIVER_HEADER}: missing shared media driver"
        )
    compatibility_driver_header = (
        root / "components" / "espcontrol" / LEGACY_COMPATIBILITY_DRIVER_HEADER
    )
    if compatibility_driver_header.exists():
        text = compatibility_driver_header.read_text(encoding="utf-8")
        required = (
            "legacy_compatibility_driver_matches",
            "legacy_compatibility_driver_setup_visual",
            "legacy_compatibility_driver_bind",
            "legacy_compatibility_driver_handle_main_click",
            "context.family == Family::TODO",
            "create_todo_card_context",
            "subscribe_todo_state",
            "subscribe_todo_friendly_name",
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{LEGACY_COMPATIBILITY_DRIVER_HEADER}: missing narrow compatibility guard {needle}"
                )
        if 'config.type' in text or 'type == "todo"' in text:
            failures.append(
                f"components/espcontrol/{LEGACY_COMPATIBILITY_DRIVER_HEADER}: match compatibility through the shared runtime context"
            )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{LEGACY_COMPATIBILITY_DRIVER_HEADER}: missing narrow legacy compatibility driver"
        )
    navigation_driver_header = root / "components" / "espcontrol" / NAVIGATION_DRIVER_HEADER
    if navigation_driver_header.exists():
        text = navigation_driver_header.read_text(encoding="utf-8")
        required = (
            "navigation_driver_setup_visual",
            "navigation_driver_bind_main",
            "navigation_driver_attach_interaction",
            "navigation_driver_refresh_layout",
            "navigation_driver_cleanup",
            "navigation_driver_handle_main_click",
            "navigation_driver_reset_child_indicators",
            "navigation_driver_add_child_indicator",
            "navigation_driver_own_subpage",
            "subscribe_subpage_parent_indicator",
            "subscribe_climate_subpage_parent_indicator",
            "navigation_register_subpage",
            '"subpage"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{NAVIGATION_DRIVER_HEADER}: missing shared navigation lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{NAVIGATION_DRIVER_HEADER}: missing shared navigation driver"
        )
    image_driver_header = root / "components" / "espcontrol" / IMAGE_DRIVER_HEADER
    if image_driver_header.exists():
        text = image_driver_header.read_text(encoding="utf-8")
        required = (
            "image_driver_setup_visual",
            "image_driver_bind_main",
            "image_driver_bind_subpage",
            "image_driver_attach_interaction",
            "image_driver_refresh_layout",
            "image_driver_cleanup",
            "image_driver_reset_pool",
            "image_driver_handle_main_click",
            "image_card_bind_runtime",
            "reset_image_card_pool",
            '"image"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{IMAGE_DRIVER_HEADER}: missing shared image lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{IMAGE_DRIVER_HEADER}: missing shared image driver"
        )
    light_control_driver_header = (
        root / "components" / "espcontrol" / LIGHT_CONTROL_DRIVER_HEADER
    )
    if light_control_driver_header.exists():
        text = light_control_driver_header.read_text(encoding="utf-8")
        required = (
            "light_control_driver_setup_visual",
            "light_control_driver_bind_main",
            "light_control_driver_bind_subpage",
            "light_control_driver_attach_interaction",
            "light_control_driver_refresh_layout",
            "light_control_driver_cleanup",
            "light_control_driver_handle_main_click",
            "create_light_control_context",
            "subscribe_light_control_state",
            "light_control_open_modal",
            '"light_control"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{LIGHT_CONTROL_DRIVER_HEADER}: missing shared light-control lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{LIGHT_CONTROL_DRIVER_HEADER}: missing shared light-control driver"
        )
    fan_control_driver_header = (
        root / "components" / "espcontrol" / FAN_CONTROL_DRIVER_HEADER
    )
    if fan_control_driver_header.exists():
        text = fan_control_driver_header.read_text(encoding="utf-8")
        required = (
            "fan_control_driver_setup_visual",
            "fan_control_driver_bind_main",
            "fan_control_driver_bind_subpage",
            "fan_control_driver_attach_interaction",
            "fan_control_driver_refresh_layout",
            "fan_control_driver_cleanup",
            "fan_control_driver_handle_main_click",
            "create_fan_card_context",
            "subscribe_fan_card_state",
            "fan_control_open_modal",
            "grid_track_fan_card_runtime",
            "grid_delete_fan_card_with_owner",
            '"fan_control"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{FAN_CONTROL_DRIVER_HEADER}: missing shared fan-control lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{FAN_CONTROL_DRIVER_HEADER}: missing shared fan-control driver"
        )
    climate_control_driver_header = (
        root / "components" / "espcontrol" / CLIMATE_CONTROL_DRIVER_HEADER
    )
    if climate_control_driver_header.exists():
        text = climate_control_driver_header.read_text(encoding="utf-8")
        required = (
            "climate_control_driver_setup_visual",
            "climate_control_driver_bind_main",
            "climate_control_driver_bind_subpage",
            "climate_control_driver_attach_interaction",
            "climate_control_driver_refresh_layout",
            "climate_control_driver_cleanup",
            "climate_control_driver_handle_main_click",
            "create_climate_control_context",
            "subscribe_climate_control_state",
            "climate_control_open_modal",
            "grid_track_climate_control_runtime",
            "grid_delete_climate_control_with_owner",
            '"climate_control"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{CLIMATE_CONTROL_DRIVER_HEADER}: missing shared climate-control lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{CLIMATE_CONTROL_DRIVER_HEADER}: missing shared climate-control driver"
        )
    alarm_driver_header = (
        root / "components" / "espcontrol" / ALARM_DRIVER_HEADER
    )
    if alarm_driver_header.exists():
        text = alarm_driver_header.read_text(encoding="utf-8")
        required = (
            "alarm_driver_setup_visual",
            "alarm_driver_bind_main",
            "alarm_driver_bind_subpage",
            "alarm_driver_attach_interaction",
            "alarm_driver_refresh_layout",
            "alarm_driver_cleanup",
            "alarm_driver_handle_main_click",
            "alarm_driver_effective_config",
            "create_alarm_card_context",
            "subscribe_alarm_state",
            "alarm_card_open_page",
            "grid_track_alarm_card_runtime",
            "grid_delete_alarm_card_with_owner",
            '"alarm"',
        )
        for needle in required:
            if needle not in text:
                failures.append(
                    f"components/espcontrol/{ALARM_DRIVER_HEADER}: missing shared alarm lifecycle guard {needle}"
                )
    elif grid_header.exists():
        failures.append(
            f"components/espcontrol/{ALARM_DRIVER_HEADER}: missing shared alarm driver"
        )
    cards_header = root / "components" / "espcontrol" / CARDS_HEADER
    if cards_header.exists():
        text = cards_header.read_text(encoding="utf-8")
        for legacy_setup in (
            "setup_sensor_card",
            "setup_text_sensor_card",
            "setup_local_sensor_card",
            "setup_weather_card",
            "setup_weather_forecast_card",
        ):
            if legacy_setup in text:
                failures.append(
                    f"components/espcontrol/{CARDS_HEADER}: keep {legacy_setup} inside its shared information driver"
                )
    return failures


def run_self_test() -> None:
    cases: tuple[tuple[dict[str, str], tuple[str, ...]], ...] = (
        (
            {"button_grid_actions.h": "return card_contract_media_mode_valid(mode);\n"},
            ("access generated card contract through button_grid_card_runtime.h",),
        ),
        (
            {"button_grid_config.h": "return CARD_CONTRACT_MEDIA_DEFAULT_MODE;\n"},
            ("access generated card contract through button_grid_card_runtime.h",),
        ),
        (
            {"button_grid_config.h": '#include "button_grid_contract_generated.h"\n'},
            ("include generated card contract through button_grid_card_runtime.h",),
        ),
        (
            {"button_grid_cards.h": "static const char *modes[] = {\"open\", \"close\"};\n"},
            ("keep shared card mode lists in the card runtime/contract boundary",),
        ),
        (
            {"button_grid_actions.h": "if (mode == \"lock\") return \"lock.lock\";\n"},
            ("keep shared card service mappings in the card runtime/contract boundary",),
        ),
        (
            {"button_grid_alarm.h": "return \"alarm_control_panel.alarm_arm_away\";\n"},
            ("keep shared card service mappings in the card runtime/contract boundary",),
        ),
        (
            {"button_grid_card_runtime.h": "return card_contract_media_mode_valid(mode);\n"},
            (),
        ),
        (
            {"button_grid_card_runtime.h": '#include "button_grid_contract_generated.h"\n'},
            (),
        ),
        (
            {"button_grid_contract_generated.h": "inline const char *const CARD_CONTRACT_MEDIA_MODES[] = {\"play_pause\"};\n"},
            (),
        ),
        (
            {"button_grid_alarm.h": "static const char *modes[3] = {\"home\", \"away\", \"disarm\"};\n"},
            (),
        ),
        (
            {"button_grid_actions.h": "ESP_LOGW(\"cover\", \"cover.stop_cover failed\");\n"},
            (),
        ),
        (
            {"button_grid_actions.h": "cover_tilt ? \"cover.set_cover_tilt_position\" : \"cover.set_cover_position\";\n"},
            (),
        ),
        (
            {"button_grid_actions.h": "if (sensor == \"open\") return \"cover.open_cover_tilt\";\n"},
            (),
        ),
        (
            {"button_grid_lawn_mower.h": "return \"lawn_mower.start_mowing\";\n"},
            ("missing mower runtime guard lawn_mower.pause",),
        ),
        (
            {"button_grid_lawn_mower.h": "return \"lawn_mower.clean_spot\";\n"},
            ("unexpected mower service/reference lawn_mower.clean_spot",),
        ),
        (
            {"button_grid_grid.h": 'if (parent_subpage_kind == "climate") {}\n'},
            ("route mower subpage parent indicators through mower active-state handling",),
        ),
        (
            {
                "button_grid_grid.h": (
                    "inline void setup_card_visual() {\n"
                    "  if (espcontrol::cards::image_driver_setup_visual()) return;\n"
                    "}\n"
                )
            },
            ("restore persistent button clickability before visual driver dispatch",),
        ),
        (
            {
                "button_grid_grid.h": (
                    "inline void clear_unsupported_card_slot_visuals() {}\n"
                    "inline void setup_card_visual() {\n"
                    "  lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);\n"
                    "  if (espcontrol::cards::image_driver_setup_visual()) return;\n"
                    "  ESP_LOGW(\"card_runtime\", \"Unsupported card type has no visual driver: type=%s\", type);\n"
                    "}\n"
                )
            },
            ("clear stale slot visuals before leaving unsupported cards inert",),
        ),
        (
            {
                "button_grid_grid.h": (
                    'if (parent_subpage_kind == "lawn_mower") { lawn_mower_state_active_ref(state); }\n'
                    'if (sb_cfg.type == "light_control") {\n'
                    '  subscribe_light_control_state(ctx);\n'
                    '}\n'
                )
            },
            ("include full light controls in generic subpage parent indicators",),
        ),
        (
            {
                "button_grid_grid.h": (
                    'if (parent_subpage_kind == "lawn_mower") { lawn_mower_state_active_ref(state); }\n'
                    'if (sb_cfg.type == "light_control") {\n'
                    '  subscribe_light_control_state(ctx);\n'
                    '  add_parent_indicator(sb_cfg.entity);\n'
                    '}\n'
                    'navigation_clear_subpages();\n'
                    'reset_image_card_pool(cfg);\n'
                )
            },
            ("reset image-card contexts before deleting subpage screens",),
        ),
        (
            {"button_grid_image.h": "for (int i = 0; i < count; i++) {}\n"},
            ("reset every image-card context, including disabled slots",),
        ),
        (
            {
                "button_grid_status_entity_driver.h": (
                    "inline bool status_entity_driver_setup_visual() {}\n"
                    "inline bool status_entity_driver_bind_data() {}\n"
                )
            },
            ("missing shared status-entity lifecycle guard",),
        ),
        (
            {
                "button_grid_date_time_driver.h": (
                    "inline bool date_time_driver_setup_visual() {}\n"
                    "inline bool date_time_driver_bind_data() {}\n"
                )
            },
            ("missing shared date-time lifecycle guard",),
        ),
        (
            {
                "button_grid_sensor_driver.h": (
                    "inline bool sensor_driver_setup_visual() {}\n"
                    "inline bool sensor_driver_bind_data() {}\n"
                )
            },
            ("missing shared sensor lifecycle guard",),
        ),
        (
            {
                "button_grid_weather_driver.h": (
                    "inline bool weather_driver_setup_visual() {}\n"
                    "inline bool weather_driver_bind_data() {}\n"
                )
            },
            ("missing shared weather lifecycle guard",),
        ),
        (
            {
                "button_grid_basic_action_driver.h": (
                    "inline bool basic_action_driver_setup_visual() {}\n"
                    "inline bool basic_action_driver_bind_main() {}\n"
                )
            },
            ("missing shared basic-action lifecycle guard",),
        ),
        (
            {
                "button_grid_numeric_selectable_driver.h": (
                    "inline bool numeric_selectable_driver_setup_visual() {}\n"
                    "inline bool numeric_selectable_driver_bind_main() {}\n"
                )
            },
            ("missing shared numeric/selectable lifecycle guard",),
        ),
        (
            {
                "button_grid_cleaning_driver.h": (
                    "inline bool cleaning_driver_setup_visual() {}\n"
                    "inline bool cleaning_driver_bind_main() {}\n"
                )
            },
            ("missing shared cleaning lifecycle guard",),
        ),
        (
            {
                "button_grid_access_cover_driver.h": (
                    "inline bool access_cover_driver_setup_visual() {}\n"
                    "inline bool access_cover_driver_bind_main() {}\n"
                )
            },
            ("missing shared access/cover lifecycle guard",),
        ),
        (
            {
                "button_grid_navigation_driver.h": (
                    "inline bool navigation_driver_setup_visual() {}\n"
                    "inline bool navigation_driver_bind_main() {}\n"
                )
            },
            ("missing shared navigation lifecycle guard",),
        ),
        (
            {
                "button_grid_image_driver.h": (
                    "inline bool image_driver_setup_visual() {}\n"
                    "inline bool image_driver_bind_main() {}\n"
                )
            },
            ("missing shared image lifecycle guard",),
        ),
        (
            {
                "button_grid_light_control_driver.h": (
                    "inline bool light_control_driver_setup_visual() {}\n"
                    "inline bool light_control_driver_bind_main() {}\n"
                )
            },
            ("missing shared light-control lifecycle guard",),
        ),
        (
            {
                "button_grid_fan_control_driver.h": (
                    "inline bool fan_control_driver_setup_visual() {}\n"
                    "inline bool fan_control_driver_bind_main() {}\n"
                )
            },
            ("missing shared fan-control lifecycle guard",),
        ),
        (
            {
                "button_grid_climate_control_driver.h": (
                    "inline bool climate_control_driver_setup_visual() {}\n"
                    "inline bool climate_control_driver_bind_main() {}\n"
                )
            },
            ("missing shared climate-control lifecycle guard",),
        ),
        (
            {
                "button_grid_alarm_driver.h": (
                    "inline bool alarm_driver_setup_visual() {}\n"
                    "inline bool alarm_driver_bind_main() {}\n"
                )
            },
            ("missing shared alarm lifecycle guard",),
        ),
        (
            {
                "button_grid_image.h": (
                    "inline void image_card_start_next_queued_download() {\n"
                    "  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {}\n"
                    "}\n"
                    "inline void reset_image_card_pool(const GridConfig &cfg) {\n"
                    "  int count = cfg.image_card_image_count;\n"
                    "  for (int i = 0; i < count; i++) {}\n"
                    "}\n"
                )
            },
            ("reset every image-card context, including disabled slots",),
        ),
        (
            {
                "button_grid_image.h": (
                    "inline void reset_image_card_pool(const GridConfig &cfg) {\n"
                    "  for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++) {}\n"
                    "}\n"
                )
            },
            (),
        ),
    )
    for files, expected in cases:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            firmware_dir = root / "components" / "espcontrol"
            firmware_dir.mkdir(parents=True)
            for name, content in files.items():
                (firmware_dir / name).write_text(content, encoding="utf-8")
            failures = check_root(root)
            for text in expected:
                assert any(text in failure for failure in failures), (files, failures, text)
            if not expected:
                assert not failures, (files, failures)
    print("Firmware card runtime self-tests passed.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        run_self_test()
        return 0

    failures = check_root(ROOT)
    if failures:
        print("Firmware card runtime guard failed:")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print("Firmware card runtime checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
