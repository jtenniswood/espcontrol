#!/usr/bin/env python3
"""Guard firmware modal code against repeated row allocation and ad hoc shells."""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = ROOT / "components" / "espcontrol"
FORBIDDEN_ALLOCATIONS = (
    "ClimateOptionClick",
    "FanPresetClick",
    "OptionSelectOptionClick",
)
LAYER_TOP_ALLOWLIST = {
    "button_grid_modal.h",
}
MANUAL_OVERLAY_DELETE_ALLOWLIST = {
    "button_grid_modal.h",
}


def yaml_script_body(text: str, script_id: str) -> str | None:
    marker = re.search(rf"(?m)^  - id: {re.escape(script_id)}\n", text)
    if marker is None:
        return None
    start = marker.end()
    next_marker = re.search(r"(?m)^  - id: ", text[start:])
    end = start + next_marker.start() if next_marker else len(text)
    return text[start:end]


def firmware_modal_errors(firmware_dir: Path, root: Path) -> list[str]:
    allocation_pattern = re.compile(r"\bnew\s+(" + "|".join(FORBIDDEN_ALLOCATIONS) + r")\b")
    layer_top_pattern = re.compile(r"\blv_layer_top\s*\(\s*\)")
    manual_overlay_delete_pattern = re.compile(r"\blv_obj_del\s*\(\s*(?:ui\.)?(?:menu_)?overlay\s*\)")
    errors: list[str] = []

    for path in sorted(firmware_dir.glob("button_grid*.h")):
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            match = allocation_pattern.search(line)
            if match:
                rel = path.relative_to(root)
                errors.append(
                    f"{rel}:{line_no}: avoid per-row heap allocation for {match.group(1)}"
                )
            if path.name not in LAYER_TOP_ALLOWLIST and layer_top_pattern.search(line):
                rel = path.relative_to(root)
                errors.append(
                    f"{rel}:{line_no}: route modal top-layer access through button_grid_modal.h helpers"
                )
            if path.name not in MANUAL_OVERLAY_DELETE_ALLOWLIST and manual_overlay_delete_pattern.search(line):
                rel = path.relative_to(root)
                errors.append(
                    f"{rel}:{line_no}: delete modal overlays through button_grid_modal.h lifecycle helpers"
                )
    sliders_path = firmware_dir / "button_grid_sliders.h"
    if not sliders_path.exists():
        errors.append("components/espcontrol/button_grid_sliders.h: keep cover modal back button accessible")
    else:
        text = sliders_path.read_text(encoding="utf-8")
        cover_layout = re.search(
            r"inline\s+void\s+cover_control_layout_modal\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            text,
            re.S,
        )
        if cover_layout is None or "lv_obj_move_foreground(ui.back_btn);" not in cover_layout.group("body"):
            errors.append(
                "components/espcontrol/button_grid_sliders.h: keep the cover modal back button above tab and slider controls"
            )
    return errors


def firmware_modal_sleep_takeover_errors(root: Path) -> list[str]:
    firmware_dir = root / "components" / "espcontrol"
    backlight_header_path = firmware_dir / "backlight.h"
    modal_path = firmware_dir / "button_grid_modal.h"
    navigation_path = firmware_dir / "button_grid_navigation.h"
    grid_path = firmware_dir / "button_grid_grid.h"
    image_path = firmware_dir / "button_grid_image.h"
    alarm_path = firmware_dir / "button_grid_alarm.h"
    backlight_path = root / "common" / "addon" / "backlight.yaml"
    schedule_path = root / "common" / "addon" / "backlight_schedule.yaml"
    generator_path = root / "scripts" / "generate_device_slots.py"
    errors: list[str] = []

    if not backlight_header_path.exists():
        errors.append("components/espcontrol/backlight.h: provide early display-takeover hook")
    else:
        text = backlight_header_path.read_text(encoding="utf-8")
        if (
            "backlight_close_modals_for_display_takeover" not in text
            or "set_backlight_display_takeover_callback" not in text
        ):
            errors.append("components/espcontrol/backlight.h: expose an early display-takeover modal hook")

    if not modal_path.exists():
        errors.append("components/espcontrol/button_grid_modal.h: provide shared modal lifecycle helpers")
    else:
        text = modal_path.read_text(encoding="utf-8")
        if (
            "enum class ControlModalDismissPolicy" not in text
            or "control_modal_force_close_active" not in text
            or "control_modal_close_active_internal(false)" not in text
            or "control_modal_close_for_display_takeover" not in text
            or "PRESERVE_DURING_DISPLAY_TAKEOVER" not in text
        ):
            errors.append(
                "components/espcontrol/button_grid_modal.h: centralize modal dismissal policy for display takeover"
            )
        kind_enum = re.search(r"enum class ControlModalKind\s*\{(?P<body>.*?)\};", text, re.S)
        definition = re.search(
            r"inline\s+ControlModalDefinition\s+control_modal_definition\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            text,
            re.S,
        )
        if kind_enum is None or definition is None:
            errors.append("components/espcontrol/button_grid_modal.h: define every modal type through the shared registry")
        else:
            kinds = re.findall(r"\b([A-Z][A-Z0-9_]*)\b", kind_enum.group("body"))
            missing = [
                kind for kind in kinds
                if kind != "NONE" and f"ControlModalKind::{kind}" not in definition.group("body")
            ]
            if missing:
                errors.append(
                    "components/espcontrol/button_grid_modal.h: register modal definitions for "
                    + ", ".join(missing)
                )

    if not navigation_path.exists():
        errors.append("components/espcontrol/button_grid_navigation.h: close modals before display takeover")
    else:
        text = navigation_path.read_text(encoding="utf-8")
        hide_modals = re.search(
            r"inline\s+void\s+navigation_hide_modals\s*\(\s*\)\s*\{(?P<body>.*?)\n\}",
            text,
            re.S,
        )
        return_home = re.search(
            r"inline\s+bool\s+navigation_return_home\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            text,
            re.S,
        )
        if hide_modals is None or "control_modal_force_close_active();" not in hide_modals.group("body"):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: return-home navigation must close active shared modals"
            )
        elif re.search(r"\b[A-Za-z0-9_]+_hide_modal\s*\(\s*\)\s*;", hide_modals.group("body")):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: keep modal-type cleanup out of navigation"
            )
        if return_home is None or "navigation_hide_modals();" not in return_home.group("body"):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: return-home navigation must use the shared modal close path"
            )
        if (
            "navigation_close_modals_for_display_takeover" not in text
            or "control_modal_close_for_display_takeover(alarm_display_takeover_active());" not in text
        ):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: preserve alarm controls only during an active alarm takeover"
            )

    if not image_path.exists():
        errors.append("components/espcontrol/button_grid_image.h: wire image modals to display-takeover guards")
    else:
        text = image_path.read_text(encoding="utf-8")
        if "pause_home_idle" in text or "resume_home_idle" in text:
            errors.append(
                "components/espcontrol/button_grid_image.h: name image modal guards after display takeover, not home idle"
            )
        if (
            "begin_display_takeover" not in text
            or "end_display_takeover" not in text
            or "DisplayTakeoverKind::INTERACTIVE" not in text
        ):
            errors.append(
                "components/espcontrol/button_grid_image.h: use typed interactive takeover hooks"
            )

    if not alarm_path.exists():
        errors.append("components/espcontrol/button_grid_alarm.h: wire critical display takeovers")
    else:
        text = alarm_path.read_text(encoding="utf-8")
        if (
            "begin_display_takeover" not in text
            or "end_display_takeover" not in text
            or "DisplayTakeoverKind::CRITICAL" not in text
            or "critical_takeover_active" not in text
        ):
            errors.append("components/espcontrol/button_grid_alarm.h: use typed critical takeover hooks")

    if not grid_path.exists():
        errors.append("components/espcontrol/button_grid_grid.h: register the display-takeover modal hook")
    else:
        text = grid_path.read_text(encoding="utf-8")
        if "set_backlight_display_takeover_callback(navigation_close_modals_for_display_takeover)" not in text:
            errors.append("components/espcontrol/button_grid_grid.h: register the display-takeover modal hook")

    if not backlight_path.exists():
        errors.append("common/addon/backlight.yaml: keep display-off modal guards")
    else:
        text = backlight_path.read_text(encoding="utf-8")
        home_idle_body = yaml_script_body(text, "home_screen_idle_check")
        if "home_screen_idle_suspended" in text:
            errors.append(
                "common/addon/backlight.yaml: keep display-takeover suspension separate from home-return idle"
            )
        if "display_takeover_suspended" in text:
            errors.append("common/addon/backlight.yaml: remove the compatibility takeover boolean")
        if "screensaver_sensor_sleep_pending" not in text:
            errors.append("common/addon/backlight.yaml: preserve pending sensor-mode sleep while image modals are active")
        begin_body = yaml_script_body(text, "display_takeover_begin")
        end_body = yaml_script_body(text, "display_takeover_end")
        if (
            begin_body is None
            or "begin_takeover" not in begin_body
            or "script.execute: display_mode_reconcile" not in begin_body
            or "script.stop:" in begin_body
        ):
            errors.append("common/addon/backlight.yaml: begin typed takeovers through the controller")
        if (
            end_body is None
            or "end_takeover" not in end_body
            or "script.execute: display_mode_reconcile" not in end_body
            or "display_takeover_resume_restore" in text
        ):
            errors.append("common/addon/backlight.yaml: resolve current requests when a takeover ends")
        sleep_timer_body = yaml_script_body(text, "screensaver_sleep_timer")
        if sleep_timer_body is None:
            errors.append("common/addon/backlight.yaml: keep the screensaver sleep timer script")
        elif (
            "DisplayTakeoverKind::INTERACTIVE" not in sleep_timer_body
            or "display_mode_request_automatic" not in sleep_timer_body
        ):
            errors.append(
                "common/addon/backlight.yaml: record automatic requests beneath interactive takeovers"
            )
        if home_idle_body is None:
            errors.append("common/addon/backlight.yaml: keep the home-screen idle return script")
        else:
            if (
                "display_takeover_suspended" in home_idle_body
                or "home_screen_idle_suspended" in home_idle_body
            ):
                errors.append(
                    "common/addon/backlight.yaml: do not gate home-return idle on display-takeover suspension"
                )
            if "navigation_return_home(id(main_page)->obj);" not in home_idle_body:
                errors.append("common/addon/backlight.yaml: home-return idle must use navigation_return_home")
        controller_off_body = yaml_script_body(text, "display_mode_effect_off")
        controller_owns_automatic_off = (
            controller_off_body is not None
            and "DisplayRequestSource::IDLE_TIMER" in controller_off_body
            and "DisplayRequestSource::PRESENCE_SENSOR" in controller_off_body
            and "backlight_close_modals_for_display_takeover();" in controller_off_body
        )
        if (
            "Skipping automatic display-off while image modal is active" not in text
            and not controller_owns_automatic_off
        ):
            errors.append("common/addon/backlight.yaml: keep automatic idle display-off blocked by image modals")
        if "backlight_close_modals_for_display_takeover();" not in text:
            errors.append("common/addon/backlight.yaml: close modals before manual or scheduled display-off")

    if not generator_path.exists():
        errors.append("scripts/generate_device_slots.py: generate display-takeover modal guards")
    else:
        text = generator_path.read_text(encoding="utf-8")
        if (
            "cfg.pause_home_idle" in text
            or "home_screen_idle_suspended" in text
            or "id(home_screen_idle_check).stop();" in text
        ):
            errors.append(
                "scripts/generate_device_slots.py: image modal display guard must not stop the home-return timer"
            )
        if (
            "cfg.begin_display_takeover" not in text
            or "cfg.end_display_takeover" not in text
            or "id(display_takeover_begin).execute(static_cast<int>(kind));" not in text
            or "id(display_takeover_end).execute(static_cast<int>(kind));" not in text
        ):
            errors.append("scripts/generate_device_slots.py: generate typed display-takeover hooks")
        backlight_text = backlight_path.read_text(encoding="utf-8") if backlight_path.exists() else ""
        if (
            "id: display_takeover_begin" not in backlight_text
            or "id: display_takeover_end" not in backlight_text
            or "id: display_mode_controller" not in backlight_text
            or "cover_art_screensaver_active" in backlight_text
        ):
            errors.append("common/addon/backlight.yaml: centralize typed display-takeover lifecycle")

    if not schedule_path.exists():
        errors.append("common/addon/backlight_schedule.yaml: close modals before scheduled takeover")
    else:
        text = schedule_path.read_text(encoding="utf-8")
        backlight_text = backlight_path.read_text(encoding="utf-8") if backlight_path.exists() else ""
        controller_closes_scheduled_takeover = (
            "id: display_mode_effect_off" in backlight_text
            and "DisplayRequestSource::SCREEN_SCHEDULE" in backlight_text
            and "backlight_close_modals_for_display_takeover();" in backlight_text
        )
        controller_closes_clock_takeover = (
            "id: display_mode_apply_transition" in backlight_text
            and "schedule_owned" in backlight_text
        )
        if (
            text.count("backlight_close_modals_for_display_takeover();") < 2
            and not (controller_closes_scheduled_takeover and controller_closes_clock_takeover)
        ):
            errors.append(
                "common/addon/backlight_schedule.yaml: close modals before scheduled sleep and clock takeover"
            )

    return errors


def firmware_subpage_modal_wiring_errors(root: Path) -> list[str]:
    grid_path = root / "components" / "espcontrol" / "button_grid_grid.h"
    subpages_path = root / "components" / "espcontrol" / "button_grid_subpages.h"
    errors: list[str] = []

    if not grid_path.exists():
        errors.append("components/espcontrol/button_grid_grid.h: wire subpage modal cards")
        return errors

    text = grid_path.read_text(encoding="utf-8")
    media_home_block = re.search(
        r"MediaControlCtx \*ctx = grid_track_media_control_runtime"
        r"(?P<body>.*?)"
        r"\n\s*\}\s*else if\s*\(mode == \"volume\"\)",
        text,
        re.S,
    )
    if media_home_block is None:
        errors.append("components/espcontrol/button_grid_grid.h: keep media control cards wired on the home grid")
    else:
        body = media_home_block.group("body")
        if (
            "create_media_control_context" not in body
            or "subscribe_media_control_state(ctx);" not in body
        ):
            errors.append("components/espcontrol/button_grid_grid.h: keep media control cards wired on the home grid")
        if "media_control_open_modal(ctx);" in body or "LV_EVENT_CLICKED, ctx" in body:
            errors.append("components/espcontrol/button_grid_grid.h: open home media control cards through the shared button dispatcher")

    media_refresh_block = re.search(
        r'if\s*\(\s*mode\s*==\s*"control_modal"\s*\)\s*\{(?P<body>.*?)\n  \}\n  if\s*\(\s*mode\s*==\s*"volume"\s*\)',
        text,
        re.S,
    )
    if media_refresh_block is None:
        errors.append("components/espcontrol/button_grid_grid.h: keep media control cards refreshed on grid layout updates")
    else:
        body = media_refresh_block.group("body")
        if (
            "grid_media_control_runtime_for_owner(s.btn)" not in body
            or "lv_obj_set_user_data(s.btn, ctx)" not in body
            or "media_control_refresh_parent_card(ctx)" not in body
            or "lv_obj_set_user_data(s.btn, nullptr)" in body
        ):
            errors.append("components/espcontrol/button_grid_grid.h: preserve media control context during grid layout refresh")

    light_block = re.search(
        r'if\s*\(\s*sb_cfg\.type\s*==\s*"light_control"\s*\)\s*\{(?P<body>.*?)\n      \}',
        text,
        re.S,
    )
    if light_block is None:
        errors.append("components/espcontrol/button_grid_grid.h: keep light control cards available in subpages")
        return errors

    body = light_block.group("body")
    if (
        "create_light_control_context" not in body
        or "subscribe_light_control_state(ctx);" not in body
        or "light_control_open_modal(ctx);" not in body
        or "LV_EVENT_CLICKED" not in body
    ):
        errors.append("components/espcontrol/button_grid_grid.h: open light control modals from subpage cards")

    fan_block = re.search(
        r'if\s*\(\s*sb_cfg\.type\s*==\s*"fan_control"\s*\)\s*\{(?P<body>.*?)\n      \}',
        text,
        re.S,
    )
    if fan_block is None:
        errors.append("components/espcontrol/button_grid_grid.h: keep fan control modal cards available in subpages")
    else:
        body = fan_block.group("body")
        if (
            "create_fan_card_context" not in body
            or "subscribe_fan_card_state(ctx);" not in body
            or "fan_control_open_modal(ctx);" not in body
            or "LV_EVENT_CLICKED" not in body
        ):
            errors.append("components/espcontrol/button_grid_grid.h: open fan control modals from subpage cards")

    if not subpages_path.exists():
        errors.append("components/espcontrol/button_grid_subpages.h: preserve light control tab options in subpages")
        return errors

    subpages_text = subpages_path.read_text(encoding="utf-8")
    if (
        'b.type == "light_control"' not in subpages_text
        or "light_control_card_options_normalized(b.options)" not in subpages_text
        or 'b.type == "fan_control"' not in subpages_text
        or "fan_control_card_options_normalized(b.options)" not in subpages_text
    ):
        errors.append("components/espcontrol/button_grid_subpages.h: preserve light and fan control tab options in subpages")
    unsupported_block = re.search(
        r'if\s*\(\s*!b\.type\.empty\(\)(?P<body>.*?)\)\s*\{\s*\n\s*b\.options\.clear\(\);',
        subpages_text,
        re.S,
    )
    if unsupported_block is None or 'b.type != "light_control"' not in unsupported_block.group("body"):
        errors.append("components/espcontrol/button_grid_subpages.h: keep light control options out of the unsupported-card cleanup")

    return errors


def firmware_light_control_brightness_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_sliders.h"
    errors: list[str] = []

    if not path.exists():
        errors.append("components/espcontrol/button_grid_sliders.h: keep light-off brightness display at zero")
        return errors

    text = path.read_text(encoding="utf-8")
    if (
        "light_control_display_pct" not in text
        or "ctx && ctx->on ? ctx->current_pct : 0" not in text
    ):
        errors.append("components/espcontrol/button_grid_sliders.h: display zero brightness while light control is off")
    if text.count("light_control_set_modal_value(ctx, light_control_display_pct(ctx));") < 2:
        errors.append("components/espcontrol/button_grid_sliders.h: refresh brightness slider from light on/off and brightness updates")
    if "light_control_set_modal_value(ui.active, light_control_display_pct(ui.active));" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: update brightness slider immediately when the light power button is used")
    if (
        "bool turn_on = !ui.active->on;" not in text
        or "lv_obj_add_event_cb(ui.power_group" not in text
        or "lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);" not in text
    ):
        errors.append("components/espcontrol/button_grid_sliders.h: toggle light power from the whole modal control")

    return errors


def firmware_cover_control_tab_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_sliders.h"
    errors: list[str] = []

    if not path.exists():
        errors.append("components/espcontrol/button_grid_sliders.h: hide cover modal tabs when only one control is visible")
        return errors

    text = path.read_text(encoding="utf-8")
    if "bool show_tab_bar = visible_tabs.count > 1;" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: hide cover modal tabs when only one control is visible")
    if "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: keep cover modal tab row hidden through the shared tab layout helper")
    if "control_modal_calc_content_layout(\n    layout, tabs_layout, show_tab_bar, 160)" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: position cover modal content with the shared content recipe")
    if "lv_coord_t content_center_y = content.center_y;" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: center cover modal controls within their planned content space")

    return errors


def firmware_light_control_tab_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_sliders.h"
    errors: list[str] = []

    if not path.exists():
        errors.append("components/espcontrol/button_grid_sliders.h: hide light modal tabs when only one control is visible")
        return errors

    text = path.read_text(encoding="utf-8")
    if "inline void light_control_apply_tab_visibility()" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: keep light modal tab visibility helper")
    if text.count("bool show_tab_bar = visible_tabs.count > 1;") < 2:
        errors.append("components/espcontrol/button_grid_sliders.h: hide light and cover modal tabs when only one control is visible")
    if text.count("control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);") < 2:
        errors.append("components/espcontrol/button_grid_sliders.h: keep single-tab modal rows hidden through the shared tab layout helper")
    if text.count("control_modal_calc_content_layout(\n    layout, tabs_layout, show_tab_bar, 160)") < 2:
        errors.append("components/espcontrol/button_grid_sliders.h: let single-control modals use the shared content recipe")

    return errors


def firmware_climate_control_tab_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_climate.h"
    errors: list[str] = []

    if not path.exists():
        errors.append("components/espcontrol/button_grid_climate.h: keep climate modal tabs")
        return errors

    text = path.read_text(encoding="utf-8")
    if "enum class ClimateControlTab" not in text or "ClimateControlVisibleTabs" not in text:
        errors.append("components/espcontrol/button_grid_climate.h: model climate modal controls as top-level tabs")
    if 'cfg_option_value(ctx ? ctx->options : "", CLIMATE_CONTROL_TABS_OPTION)' not in text:
        errors.append("components/espcontrol/button_grid_climate.h: order climate modal tabs from saved climate_tabs config")
    if "climate_control_tab_supported(ctx, tab)" not in text:
        errors.append("components/espcontrol/button_grid_climate.h: filter climate tabs using Home Assistant capabilities")
    if "ui.tab = climate_control_first_visible_tab(ctx);" not in text:
        errors.append("components/espcontrol/button_grid_climate.h: fall back when the active climate tab disappears")
    if "bool show_tab_bar = tab_count > 1;" not in text:
        errors.append("components/espcontrol/button_grid_climate.h: hide climate modal tabs unless multiple controls are visible")
    if "all_controls" in text:
        errors.append("components/espcontrol/button_grid_climate.h: remove the legacy climate/all-controls split")
    if "climate_set_dial_controls_visible(show_temperature)" not in text:
        errors.append("components/espcontrol/button_grid_climate.h: keep temperature controls scoped to the temperature tab")
    if "climate_open_inline_option_list(ctx, climate_control_tab_kind(ui.tab))" not in text:
        errors.append("components/espcontrol/button_grid_climate.h: show non-temperature climate controls as tab pages")
    if (
        "case ClimateControlTab::SWING:\n      return !ctx->swing_modes.empty();" not in text
        or 'subscribe_list("swing_modes", &ClimateControlCtx::swing_modes);' not in text
    ):
        errors.append("components/espcontrol/button_grid_climate.h: show the swing tab only when Home Assistant exposes swing modes")
    if (
        'climate_send_action(ctx->entity_id, "climate.set_swing_mode", {{"swing_mode", value}});' not in text
        or 'ui.tab_row, find_icon("Arrow Up Down"), ctx->icon_font,' not in text
    ):
        errors.append("components/espcontrol/button_grid_climate.h: keep the swing mode action and requested tab icon")

    return errors


def firmware_modal_tab_layout_errors(root: Path) -> list[str]:
    firmware_dir = root / "components" / "espcontrol"
    modal_path = firmware_dir / "button_grid_modal.h"
    geometry_path = firmware_dir / "button_grid_modal_layout.h"
    errors: list[str] = []

    if not geometry_path.exists():
        errors.append("components/espcontrol/button_grid_modal_layout.h: provide a testable modal tab layout recipe")
    else:
        geometry_text = geometry_path.read_text(encoding="utf-8")
        if (
            "struct TabLayout" not in geometry_text
            or "constexpr TabLayout calculate_tabs" not in geometry_text
            or "struct ContentLayout" not in geometry_text
            or "constexpr ContentLayout calculate_content" not in geometry_text
        ):
            errors.append("components/espcontrol/button_grid_modal_layout.h: keep modal tab and content geometry in shared recipes")

    if not modal_path.exists():
        errors.append("components/espcontrol/button_grid_modal.h: provide shared modal tab layout helpers")
    else:
        text = modal_path.read_text(encoding="utf-8")
        required = (
            "struct ControlModalTabLayout",
            "inline ControlModalTabLayout control_modal_calc_tab_layout",
            "inline void control_modal_apply_tab_row",
            "inline lv_obj_t *control_modal_create_tab_row",
            "inline void control_modal_layout_tab_button",
            "inline lv_coord_t control_modal_shared_tab_content_gap",
            "control_modal_calc_content_layout",
            "espcontrol::modal::calculate_tabs",
        )
        for needle in required:
            if needle not in text:
                errors.append("components/espcontrol/button_grid_modal.h: keep shared modal tab layout helpers")
                break

    required_by_file = {
        "button_grid_climate.h": (
            "return control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);",
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);",
            "control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);",
            "return control_modal_shared_tab_content_gap(layout);",
            "ui.tab_row = control_modal_create_tab_row(ui.panel);",
        ),
        "button_grid_fan.h": (
            "ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);",
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);",
            "control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);",
            "control_modal_calc_content_layout(",
            "ui.tab_row = control_modal_create_tab_row(ui.panel);",
        ),
        "button_grid_media.h": (
            "control_modal_calc_tab_layout(layout, MEDIA_CONTROL_TAB_COUNT, true)",
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);",
            "control_modal_layout_tab_button(tabs[i].btn, layout, tabs_layout, i, active);",
            "control_modal_calc_content_layout(",
            "ui.tab_row = control_modal_create_tab_row(ui.panel);",
        ),
    }
    sliders_required = (
        "ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);",
        "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);",
        "control_modal_layout_tab_button(",
        "control_modal_calc_content_layout(",
        "ui.tab_row = control_modal_create_tab_row(ui.panel);",
    )
    for filename, required in required_by_file.items():
        path = firmware_dir / filename
        if not path.exists():
            errors.append(f"components/espcontrol/{filename}: use shared modal tab layout helpers")
            continue
        text = path.read_text(encoding="utf-8")
        for needle in required:
            if needle not in text:
                errors.append(f"components/espcontrol/{filename}: use shared modal tab layout helpers")
                break

    sliders_path = firmware_dir / "button_grid_sliders.h"
    if not sliders_path.exists():
        errors.append("components/espcontrol/button_grid_sliders.h: use shared modal tab layout helpers")
    else:
        text = sliders_path.read_text(encoding="utf-8")
        for needle in sliders_required:
            if needle not in text:
                errors.append("components/espcontrol/button_grid_sliders.h: use shared modal tab layout helpers")
                break
        if (
            text.count("ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);") < 2
            or text.count("control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);") < 2
            or text.count("control_modal_calc_content_layout(") < 2
            or text.count("ui.tab_row = control_modal_create_tab_row(ui.panel);") < 2
        ):
            errors.append("components/espcontrol/button_grid_sliders.h: use shared modal tab layout helpers for light and cover tabs")

    forbidden_tab_math = (
        "lv_coord_t selected_tab_size =",
        "lv_coord_t tab_frame_pad =",
        "lv_coord_t tabs_total_w =",
        "lv_coord_t first_tab_x =",
        "lv_coord_t centered_left =",
        "lv_coord_t tab_safe_left =",
        "lv_coord_t max_tab_frame_w =",
    )
    for filename in (
        "button_grid_climate.h",
        "button_grid_fan.h",
        "button_grid_media.h",
        "button_grid_sliders.h",
    ):
        path = firmware_dir / filename
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8")
        for needle in forbidden_tab_math:
            if needle in text:
                errors.append(f"components/espcontrol/{filename}: keep modal tab sizing in button_grid_modal_layout.h")
                break

    return errors


def firmware_media_modal_progress_layout_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_media.h"
    errors: list[str] = []

    if not path.exists():
        errors.append("components/espcontrol/button_grid_media.h: keep media modal progress layout stable")
        return errors

    text = path.read_text(encoding="utf-8")
    required = (
        "bool progress_layout_ready = false;",
        "bool progress_refresh_pending = false;",
        "if (!ui.progress_layout_ready) {\n    ui.progress_refresh_pending = true;\n    return;\n  }",
        "if (ui.progress_fill) lv_obj_add_flag(ui.progress_fill, LV_OBJ_FLAG_HIDDEN);",
        "if (ui.progress_handle) lv_obj_add_flag(ui.progress_handle, LV_OBJ_FLAG_HIDDEN);",
        "ui.progress_layout_ready = false;\n    lv_obj_set_size(ui.progress_slider, progress_slider_w, progress_slider_h);",
        "lv_obj_update_layout(ui.progress_slider);\n    ui.progress_layout_ready = true;",
        "media_control_refresh_progress(ctx);\n    if (ui.progress_fill) lv_obj_clear_flag(ui.progress_fill, LV_OBJ_FLAG_HIDDEN);",
        "lv_obj_clear_flag(ui.progress_slider, LV_OBJ_FLAG_HIDDEN);",
    )
    for needle in required:
        if needle not in text:
            errors.append(
                "components/espcontrol/button_grid_media.h: keep media modal progress drawing gated until final layout"
            )
            break

    return errors


def firmware_network_status_version_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "network_status.h"
    errors: list[str] = []
    if not path.exists():
        errors.append("components/espcontrol/network_status.h: keep network status version labeling")
        return errors

    text = path.read_text(encoding="utf-8")
    if "network_status_is_specific_firmware_version" not in text:
        errors.append("components/espcontrol/network_status.h: classify release versions before labeling firmware")

    label_match = re.search(
        r"inline\s+std::string\s+network_status_firmware_label\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.S,
    )
    if label_match is None:
        errors.append("components/espcontrol/network_status.h: keep network_status_firmware_label helper")
        return errors

    body = label_match.group("body")
    if "network_status_is_specific_firmware_version(trimmed)" not in body:
        errors.append("components/espcontrol/network_status.h: show only release versions as installed versions")
    if 'espcontrol_i18n(std::string("Dev build"))' not in body:
        errors.append("components/espcontrol/network_status.h: label local ESPHome builds as Dev build")
    if 'espcontrol_i18n(std::string("Version unknown"))' not in body:
        errors.append("components/espcontrol/network_status.h: keep empty firmware versions readable")

    return errors


def firmware_climate_step_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_climate.h"
    if not path.exists():
        return []

    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    errors: list[str] = []

    helper = re.search(
        r"inline\s+int\s+climate_effective_step_tenths\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.S,
    )
    if helper is None:
        errors.append(f"{rel}: keep climate controls at a 0.5C minimum step")
    else:
        body = helper.group("body")
        if (
            "CLIMATE_DEFAULT_STEP_TENTHS" not in body
            or "CLIMATE_WHOLE_NUMBER_STEP_TENTHS" not in body
            or "ctx->configured_step_tenths" not in body
            or "return ctx->configured_step_tenths" not in body
            or "ctx->step_tenths > minimum" in body
        ):
            errors.append(f"{rel}: use the configured climate temperature step")

    required = (
        "constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 5;",
        "constexpr int CLIMATE_WHOLE_NUMBER_STEP_TENTHS = 10;",
        "int configured_step_tenths = CLIMATE_WHOLE_NUMBER_STEP_TENTHS;",
        'cfg_option_value(p.options, "temperature_step")',
        "int step = climate_effective_step_tenths(ctx);",
        "int base = ctx->precision <= 0 ? 0 : ctx->min_tenths;",
        "climate_round_to_step(ctx, climate_constrain_selected_target(ctx, value))",
        "climate_preview_selected_target(ui.active,",
        "climate_target_from_modal_arc_value(ui.active, lv_arc_get_value(arc))",
        "climate_apply_selected_target(ui.active, value, true, false);",
        "climate_selected_target(ui.active) - climate_effective_step_tenths(ui.active)",
        "climate_selected_target(ui.active) + climate_effective_step_tenths(ui.active)",
    )
    for needle in required:
        if needle not in text:
            errors.append(f"{rel}: route climate modal temperature changes through step rounding")
            break

    forbidden = (
        "constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 1;",
        "climate_selected_target(ui.active) - ui.active->step_tenths",
        "climate_selected_target(ui.active) + ui.active->step_tenths",
    )
    for needle in forbidden:
        if needle in text:
            errors.append(f"{rel}: do not allow 0.1C climate modal temperature steps")
            break

    cooling_drag_required = (
        "inline int climate_target_from_modal_arc_value(ClimateControlCtx *ctx, int value)",
        "if (climate_uses_cooling_arc(ctx)) return ctx->min_tenths + ctx->max_tenths - value;",
        "climate_target_from_modal_arc_value(ui.active, lv_arc_get_value(arc))",
    )
    for needle in cooling_drag_required:
        if needle not in text:
            errors.append(f"{rel}: keep cooling-mode climate arc drags aligned with the touch direction")
            break

    return errors


def firmware_climate_option_selection_errors(root: Path) -> list[str]:
    path = root / "components" / "espcontrol" / "button_grid_climate.h"
    if not path.exists():
        return []

    rel = path.relative_to(root)
    text = path.read_text(encoding="utf-8")
    match = re.search(
        r"inline\s+bool\s+climate_option_selected\s*\([^)]*\)\s*\{(?P<body>.*?)\n\}",
        text,
        re.S,
    )
    if match is None:
        return [f"{rel}: keep climate option selection state matching"]

    body = match.group("body")
    if (
        "climate_option_current_value(ctx, kind)" not in body
        or "climate_lower(climate_trim(value))" not in body
        or "climate_lower(climate_trim(current))" not in body
    ):
        return [f"{rel}: match climate option state without case sensitivity"]
    return []


def run_scan() -> int:
    errors = firmware_modal_errors(FIRMWARE_DIR, ROOT)
    errors.extend(firmware_modal_sleep_takeover_errors(ROOT))
    errors.extend(firmware_subpage_modal_wiring_errors(ROOT))
    errors.extend(firmware_climate_step_errors(ROOT))
    errors.extend(firmware_climate_option_selection_errors(ROOT))
    errors.extend(firmware_light_control_brightness_errors(ROOT))
    errors.extend(firmware_light_control_tab_errors(ROOT))
    errors.extend(firmware_cover_control_tab_errors(ROOT))
    errors.extend(firmware_climate_control_tab_errors(ROOT))
    errors.extend(firmware_modal_tab_layout_errors(ROOT))
    errors.extend(firmware_media_modal_progress_layout_errors(ROOT))
    errors.extend(firmware_network_status_version_errors(ROOT))

    if errors:
        print("Firmware modal allocation check failed:")
        for error in errors:
            print(f"  {error}")
        return 1

    print("Firmware modal allocation checks passed.")
    return 0


def expect_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        firmware_dir = root / "components" / "espcontrol"
        firmware_dir.mkdir(parents=True)
        for filename, text in files.items():
            (firmware_dir / filename).write_text(text, encoding="utf-8")
        sliders_path = firmware_dir / "button_grid_sliders.h"
        if not sliders_path.exists():
            sliders_path.write_text(
                "inline void cover_control_layout_modal(CoverControlCtx *ctx) {\n"
                "  lv_obj_move_foreground(ui.back_btn);\n"
                "}\n",
                encoding="utf-8",
            )

        errors = firmware_modal_errors(firmware_dir, root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_sleep_takeover_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        for filename, text in files.items():
            path = root / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")

        errors = firmware_modal_sleep_takeover_errors(root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_subpage_modal_wiring_errors(name: str, grid_text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "components" / "espcontrol" / "button_grid_grid.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            grid_text +
            '  if (mode == "control_modal") {\n'
            "    MediaControlCtx *ctx = grid_media_control_runtime_for_owner(s.btn);\n"
            "    setup_media_control_button(\n"
            "      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, p);\n"
            "    if (s.btn) lv_obj_set_user_data(s.btn, ctx);\n"
            "    if (ctx) media_control_refresh_parent_card(ctx);\n"
            "    return;\n"
            "  }\n"
            "  if (mode == \"volume\") return;\n"
            '        } else if (mode == "control_modal") {\n'
            "          MediaControlCtx *ctx = grid_track_media_control_runtime(s.btn, create_media_control_context(\n"
            "            s, p, DEFAULT_SLIDER_COLOR, DEFAULT_OFF_COLOR, DEFAULT_TERTIARY_COLOR,\n"
            "            nullptr, nullptr, nullptr, nullptr, 100));\n"
            "          subscribe_media_control_state(ctx);\n"
            '        } else if (mode == "volume") {\n'
            '      if (sb_cfg.type == "fan_control") {\n'
            "        if (!sb_cfg.entity.empty()) {\n"
            "          FanCardCtx *ctx = create_fan_card_context(\n"
            "            sub_slot, sb_cfg, DEFAULT_SLIDER_COLOR, DEFAULT_OFF_COLOR, DEFAULT_TERTIARY_COLOR, nullptr, nullptr, 100);\n"
            "          subscribe_fan_card_state(ctx);\n"
            "          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {\n"
            "            FanCardCtx *ctx = (FanCardCtx *)lv_event_get_user_data(e);\n"
            "            if (ctx) fan_control_open_modal(ctx);\n"
            "          }, LV_EVENT_CLICKED, ctx);\n"
            "        }\n"
            "        continue;\n"
            "      }\n",
            encoding="utf-8",
        )
        (path.parent / "button_grid_subpages.h").write_text(
            'if (b.type == "light_control") {\n'
            "  b.options = light_control_card_options_normalized(b.options);\n"
            "}\n"
            'if (b.type == "fan_control") {\n'
            "  b.options = fan_control_card_options_normalized(b.options);\n"
            "}\n"
            'if (!b.type.empty() && b.type != "light_control" && !fan_card_type(b.type)) {\n'
            "  b.options.clear();\n"
            "}\n",
            encoding="utf-8",
        )

        errors = firmware_subpage_modal_wiring_errors(root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_climate_step_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "components" / "espcontrol" / "button_grid_climate.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_climate_step_errors(root)

        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_climate_option_selection_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "components" / "espcontrol" / "button_grid_climate.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_climate_option_selection_errors(root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_network_status_version_errors(name: str, header_text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "components" / "espcontrol" / "network_status.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(header_text, encoding="utf-8")

        errors = firmware_network_status_version_errors(root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def valid_modal_tab_layout_files() -> dict[str, str]:
    return {
        "components/espcontrol/button_grid_modal_layout.h": (
            "struct TabLayout {};\n"
            "constexpr TabLayout calculate_tabs() {}\n"
            "struct ContentLayout {};\n"
            "constexpr ContentLayout calculate_content() {}\n"
        ),
        "components/espcontrol/button_grid_modal.h": (
            "struct ControlModalTabLayout {};\n"
            "inline lv_coord_t control_modal_shared_tab_content_gap(const ControlModalLayout &layout) { return 0; }\n"
            "inline ControlModalTabLayout control_modal_calc_tab_layout(const ControlModalLayout &layout, int tab_count, bool show_tab_bar) { return espcontrol::modal::calculate_tabs(); }\n"
            "inline ContentLayout control_modal_calc_content_layout() {}\n"
            "inline void control_modal_apply_tab_row(lv_obj_t *tab_row, const ControlModalLayout &layout, const ControlModalTabLayout &tabs_layout) {}\n"
            "inline lv_obj_t *control_modal_create_tab_row(lv_obj_t *panel) {}\n"
            "inline void control_modal_layout_tab_button(lv_obj_t *tab_btn, const ControlModalLayout &layout, const ControlModalTabLayout &tabs_layout, int index, bool active) {}\n"
        ),
        "components/espcontrol/button_grid_climate.h": (
            "return control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);\n"
            "return control_modal_shared_tab_content_gap(layout);\n"
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);\n"
            "control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);\n"
            "ui.tab_row = control_modal_create_tab_row(ui.panel);\n"
        ),
        "components/espcontrol/button_grid_fan.h": (
            "ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);\n"
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);\n"
            "control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);\n"
            "control_modal_calc_content_layout(\n"
            "ui.tab_row = control_modal_create_tab_row(ui.panel);\n"
        ),
        "components/espcontrol/button_grid_media.h": (
            "control_modal_calc_tab_layout(layout, MEDIA_CONTROL_TAB_COUNT, true)\n"
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);\n"
            "control_modal_layout_tab_button(tabs[i].btn, layout, tabs_layout, i, active);\n"
            "control_modal_calc_content_layout(\n"
            "ui.tab_row = control_modal_create_tab_row(ui.panel);\n"
        ),
        "components/espcontrol/button_grid_sliders.h": (
            "ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);\n"
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);\n"
            "control_modal_layout_tab_button(\n"
            "control_modal_calc_content_layout(\n"
            "ui.tab_row = control_modal_create_tab_row(ui.panel);\n"
            "ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);\n"
            "control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);\n"
            "control_modal_layout_tab_button(\n"
            "control_modal_calc_content_layout(\n"
            "ui.tab_row = control_modal_create_tab_row(ui.panel);\n"
        ),
    }


def expect_modal_tab_layout_errors(name: str, files: dict[str, str], expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        for filename, text in files.items():
            path = root / filename
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")

        errors = firmware_modal_tab_layout_errors(root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def expect_media_modal_progress_layout_errors(name: str, text: str, expected: tuple[str, ...]) -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        path = root / "components" / "espcontrol" / "button_grid_media.h"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

        errors = firmware_media_modal_progress_layout_errors(root)
        for item in expected:
            assert any(item in error for error in errors), f"{name}: missing {item!r} in {errors!r}"
        if not expected:
            assert not errors, f"{name}: expected no errors, got {errors!r}"


def valid_media_modal_progress_layout_text() -> str:
    return (
        "struct MediaControlModalUi {\n"
        "  bool progress_layout_ready = false;\n"
        "  bool progress_refresh_pending = false;\n"
        "};\n"
        "inline void media_control_refresh_progress(MediaControlCtx *ctx) {\n"
        "  if (!ui.progress_layout_ready) {\n"
        "    ui.progress_refresh_pending = true;\n"
        "    return;\n"
        "  }\n"
        "}\n"
        "inline void media_control_create_progress_tab_content(MediaControlCtx *ctx) {\n"
        "  if (ui.progress_fill) lv_obj_add_flag(ui.progress_fill, LV_OBJ_FLAG_HIDDEN);\n"
        "  if (ui.progress_handle) lv_obj_add_flag(ui.progress_handle, LV_OBJ_FLAG_HIDDEN);\n"
        "}\n"
        "inline void media_control_layout_modal(MediaControlCtx *ctx) {\n"
        "  ui.progress_layout_ready = false;\n"
        "    lv_obj_set_size(ui.progress_slider, progress_slider_w, progress_slider_h);\n"
        "  lv_obj_update_layout(ui.progress_slider);\n"
        "    ui.progress_layout_ready = true;\n"
        "  media_control_refresh_progress(ctx);\n"
        "    if (ui.progress_fill) lv_obj_clear_flag(ui.progress_fill, LV_OBJ_FLAG_HIDDEN);\n"
        "  lv_obj_clear_flag(ui.progress_slider, LV_OBJ_FLAG_HIDDEN);\n"
        "}\n"
    )


def valid_sleep_takeover_files() -> dict[str, str]:
    return {
        "components/espcontrol/backlight.h": (
            "inline void set_backlight_display_takeover_callback(BacklightDisplayTakeoverCallback callback) {}\n"
            "inline void backlight_close_modals_for_display_takeover() {}\n"
        ),
        "components/espcontrol/button_grid_modal.h": (
            "enum class ControlModalKind { NONE };\n"
            "enum class ControlModalDismissPolicy { PRESERVE_DURING_DISPLAY_TAKEOVER };\n"
            "struct ControlModalDefinition {};\n"
            "inline ControlModalDefinition control_modal_definition(ControlModalKind kind) {\n"
            "  return {};\n"
            "}\n"
            "inline void control_modal_close_active_internal(bool honor_close_guard) {}\n"
            "inline void control_modal_force_close_active() { control_modal_close_active_internal(false); }\n"
            "inline void control_modal_close_for_display_takeover(bool preserve_policy_active) {}\n"
        ),
        "components/espcontrol/button_grid_navigation.h": (
            "inline void navigation_hide_modals() {\n"
            "  control_modal_force_close_active();\n"
            "}\n"
            "inline bool navigation_return_home(lv_obj_t *main_page_obj) {\n"
            "  navigation_hide_modals();\n"
            "  return true;\n"
            "}\n"
            "inline void navigation_close_modals_for_display_takeover() {\n"
            "  control_modal_close_for_display_takeover(alarm_display_takeover_active());\n"
            "}\n"
        ),
        "components/espcontrol/button_grid_grid.h": (
            "set_backlight_display_takeover_callback(navigation_close_modals_for_display_takeover);\n"
        ),
        "components/espcontrol/button_grid_image.h": (
            "std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover;\n"
            "std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;\n"
            "ctx->begin_display_takeover(espcontrol::DisplayTakeoverKind::INTERACTIVE);\n"
            "ctx->end_display_takeover(espcontrol::DisplayTakeoverKind::INTERACTIVE);\n"
        ),
        "components/espcontrol/button_grid_alarm.h": (
            "std::function<void(espcontrol::DisplayTakeoverKind)> begin_display_takeover;\n"
            "std::function<void(espcontrol::DisplayTakeoverKind)> end_display_takeover;\n"
            "bool critical_takeover_active = false;\n"
            "ctx->begin_display_takeover(espcontrol::DisplayTakeoverKind::CRITICAL);\n"
            "ctx->end_display_takeover(espcontrol::DisplayTakeoverKind::CRITICAL);\n"
            "ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER\n"
        ),
        "common/addon/backlight.yaml": (
            "globals:\n"
            "  - id: screensaver_sensor_sleep_pending\n"
            "  - id: display_mode_controller\n"
            "script:\n"
            "  - id: display_takeover_begin\n"
            "    then:\n"
            "      - lambda: 'id(display_mode_controller).begin_takeover(static_cast<espcontrol::DisplayTakeoverKind>(takeover_kind));'\n"
            "      - script.execute: display_mode_reconcile\n"
            "  - id: display_takeover_end\n"
            "    then:\n"
            "      - lambda: 'id(display_mode_controller).end_takeover(static_cast<espcontrol::DisplayTakeoverKind>(takeover_kind));'\n"
            "      - script.execute: display_mode_reconcile\n"
            "  - id: screensaver_sleep_timer\n"
            "    then:\n"
            "      - lambda: 'id(display_mode_controller).takeover_active(espcontrol::DisplayTakeoverKind::INTERACTIVE);'\n"
            "      - script.execute: display_mode_request_automatic\n"
            "  - id: home_screen_idle_check\n"
            "    then:\n"
            "      - lambda: |-\n"
            "          navigation_return_home(id(main_page)->obj);\n"
            "Skipping automatic display-off while image modal is active\n"
            "backlight_close_modals_for_display_takeover();\n"
        ),
        "common/addon/backlight_schedule.yaml": (
            "backlight_close_modals_for_display_takeover();\n"
            "backlight_close_modals_for_display_takeover();\n"
        ),
        "scripts/generate_device_slots.py": (
            "cfg.begin_display_takeover = [](espcontrol::DisplayTakeoverKind kind) {\n"
            "  id(display_takeover_begin).execute(static_cast<int>(kind));\n"
            "};\n"
            "cfg.end_display_takeover = [](espcontrol::DisplayTakeoverKind kind) {\n"
            "  id(display_takeover_end).execute(static_cast<int>(kind));\n"
            "};\n"
        ),
    }


def run_self_test() -> int:
    expect_errors(
        "forbidden click allocation",
        {"button_grid_climate.h": "auto *click = new ClimateOptionClick();\n"},
        ("avoid per-row heap allocation for ClimateOptionClick",),
    )
    expect_errors(
        "ad hoc top layer",
        {"button_grid_climate.h": "lv_obj_t *overlay = lv_obj_create(lv_layer_top());\n"},
        ("route modal top-layer access through button_grid_modal.h helpers",),
    )
    expect_errors(
        "ad hoc top layer reference",
        {"button_grid_alarm.h": "lv_obj_move_foreground(lv_layer_top());\n"},
        ("route modal top-layer access through button_grid_modal.h helpers",),
    )
    expect_errors(
        "shared helpers",
        {"button_grid_modal.h": "lv_obj_t *overlay = lv_obj_create(lv_layer_top());\n"},
        (),
    )
    expect_errors(
        "manual overlay delete",
        {"button_grid_media.h": "if (ui.overlay) lv_obj_del(ui.overlay);\n"},
        ("delete modal overlays through button_grid_modal.h lifecycle helpers",),
    )
    expect_errors(
        "shared delete helper",
        {"button_grid_media.h": "control_modal_delete_overlay(ControlModalKind::MEDIA_VOLUME, ui.overlay);\n"},
        (),
    )
    expect_sleep_takeover_errors(
        "missing display takeover close",
        {
            "components/espcontrol/backlight.h": "inline void backlight_close_modals_for_display_takeover() {}\n",
            "components/espcontrol/button_grid_modal.h": "inline void control_modal_close_active() {}\n",
            "components/espcontrol/button_grid_navigation.h": "inline void navigation_hide_modals() {}\n",
            "components/espcontrol/button_grid_grid.h": "inline void grid_phase1() {}\n",
            "common/addon/backlight.yaml": "Skipping automatic display-off while image modal is active\n",
            "common/addon/backlight_schedule.yaml": "script:\n",
        },
        (
            "expose an early display-takeover modal hook",
            "centralize modal dismissal policy for display takeover",
            "preserve alarm controls only during an active alarm takeover",
            "register the display-takeover modal hook",
            "close modals before manual or scheduled display-off",
            "close modals before scheduled sleep and clock takeover",
        ),
    )
    expect_sleep_takeover_errors(
        "display takeover close",
        valid_sleep_takeover_files(),
        (),
    )
    unconditional_alarm_preservation = valid_sleep_takeover_files()
    navigation_path = "components/espcontrol/button_grid_navigation.h"
    unconditional_alarm_preservation[navigation_path] = unconditional_alarm_preservation[
        navigation_path
    ].replace(
        "control_modal_close_for_display_takeover(alarm_display_takeover_active());",
        "control_modal_close_for_display_takeover();",
    )
    expect_sleep_takeover_errors(
        "manual alarm modal preservation",
        unconditional_alarm_preservation,
        ("preserve alarm controls only during an active alarm takeover",),
    )
    expect_subpage_modal_wiring_errors(
        "media refresh preserves modal context",
        (
            '  if (mode == "control_modal") {\n'
            "    if (s.btn) lv_obj_set_user_data(s.btn, nullptr);\n"
            "    setup_media_control_button(\n"
            "      s.btn, s.icon_lbl, s.sensor_container, s.sensor_lbl, s.unit_lbl, s.text_lbl, p);\n"
            "    MediaControlCtx *ctx = (MediaControlCtx *)lv_obj_get_user_data(s.btn);\n"
            "    if (ctx) media_control_refresh_parent_card(ctx);\n"
            "    return;\n"
            "  }\n"
            "  if (mode == \"volume\") return;\n"
        ),
        ("preserve media control context during grid layout refresh",),
    )
    expect_subpage_modal_wiring_errors(
        "subpage light modal missing click handler",
        (
            '      if (sb_cfg.type == "light_control") {\n'
            "        if (!sb_cfg.entity.empty()) {\n"
            "          LightControlCtx *ctx = create_light_control_context(\n"
            "            sub_slot, sb_cfg, DEFAULT_SLIDER_COLOR, nullptr, nullptr, nullptr, 100);\n"
            "          subscribe_light_control_state(ctx);\n"
            "        }\n"
            "        continue;\n"
            "      }\n"
        ),
        ("open light control modals from subpage cards",),
    )
    expect_subpage_modal_wiring_errors(
        "subpage light modal click handler",
        (
            '      if (sb_cfg.type == "light_control") {\n'
            "        if (!sb_cfg.entity.empty()) {\n"
            "          LightControlCtx *ctx = create_light_control_context(\n"
            "            sub_slot, sb_cfg, DEFAULT_SLIDER_COLOR, nullptr, nullptr, nullptr, 100);\n"
            "          subscribe_light_control_state(ctx);\n"
            "          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {\n"
            "            LightControlCtx *ctx = (LightControlCtx *)lv_event_get_user_data(e);\n"
            "            if (ctx) light_control_open_modal(ctx);\n"
            "          }, LV_EVENT_CLICKED, ctx);\n"
            "        }\n"
            "        continue;\n"
            "      }\n"
        ),
        (),
    )
    expect_climate_step_errors(
        "climate modal allows 0.1C steps",
        (
            "constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 1;\n"
            "constexpr int CLIMATE_WHOLE_NUMBER_STEP_TENTHS = 10;\n"
            "inline int climate_effective_step_tenths(ClimateControlCtx *ctx) {\n"
            "  if (!ctx) return CLIMATE_DEFAULT_STEP_TENTHS;\n"
            "  return ctx->step_tenths;\n"
            "}\n"
            "inline int climate_round_to_step(ClimateControlCtx *ctx, int value) {\n"
            "  int step = ctx->step_tenths;\n"
            "  int base = ctx->min_tenths;\n"
            "  return value;\n"
            "}\n"
            "inline void climate_control_open_modal(ClimateControlCtx *ctx) {\n"
            "  climate_selected_target(ui.active) - ui.active->step_tenths;\n"
            "  climate_selected_target(ui.active) + ui.active->step_tenths;\n"
            "}\n"
        ),
        (
            "use the configured climate temperature step",
            "route climate modal temperature changes through step rounding",
            "do not allow 0.1C climate modal temperature steps",
        ),
    )
    expect_climate_step_errors(
        "climate modal uses configured step increment",
        (
            "constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 5;\n"
            "constexpr int CLIMATE_WHOLE_NUMBER_STEP_TENTHS = 10;\n"
            "int configured_step_tenths = CLIMATE_WHOLE_NUMBER_STEP_TENTHS;\n"
            "inline int climate_effective_step_tenths(ClimateControlCtx *ctx) {\n"
            "  if (!ctx) return CLIMATE_DEFAULT_STEP_TENTHS;\n"
            "  if (ctx->configured_step_tenths == CLIMATE_DEFAULT_STEP_TENTHS ||\n"
            "      ctx->configured_step_tenths == CLIMATE_WHOLE_NUMBER_STEP_TENTHS)\n"
            "    return ctx->configured_step_tenths;\n"
            "  return CLIMATE_WHOLE_NUMBER_STEP_TENTHS;\n"
            "}\n"
            "inline int climate_round_to_step(ClimateControlCtx *ctx, int value) {\n"
            "  int step = climate_effective_step_tenths(ctx);\n"
            "  int base = ctx->precision <= 0 ? 0 : ctx->min_tenths;\n"
            "  return value + step;\n"
            "}\n"
            "inline void climate_apply_selected_target(ClimateControlCtx *ctx, int value, bool send_now, bool debounce) {\n"
            "  value = climate_round_to_step(ctx, climate_constrain_selected_target(ctx, value));\n"
            "}\n"
            "inline ClimateControlCtx *create_climate_control_context(const ParsedCfg &p) {\n"
            "  ctx->configured_step_tenths = normalize_climate_temperature_step(\n"
            "    cfg_option_value(p.options, \"temperature_step\")) == \"0.5\"\n"
            "      ? CLIMATE_DEFAULT_STEP_TENTHS\n"
            "      : CLIMATE_WHOLE_NUMBER_STEP_TENTHS;\n"
            "}\n"
            "inline int climate_target_from_modal_arc_value(ClimateControlCtx *ctx, int value) {\n"
            "  if (climate_uses_cooling_arc(ctx)) return ctx->min_tenths + ctx->max_tenths - value;\n"
            "  return value;\n"
            "}\n"
            "inline void climate_control_open_modal(ClimateControlCtx *ctx) {\n"
            "  climate_preview_selected_target(ui.active,\n"
            "    climate_target_from_modal_arc_value(ui.active, lv_arc_get_value(arc)));\n"
            "  climate_apply_selected_target(ui.active, value, true, false);\n"
            "  climate_selected_target(ui.active) - climate_effective_step_tenths(ui.active);\n"
            "  climate_selected_target(ui.active) + climate_effective_step_tenths(ui.active);\n"
            "}\n"
        ),
        (),
    )
    expect_climate_option_selection_errors(
        "climate option selection is case-sensitive",
        (
            "inline bool climate_option_selected(ClimateControlCtx *ctx,\n"
            "                                    const std::string &kind,\n"
            "                                    const std::string &value) {\n"
            "  if (!ctx) return false;\n"
            "  return value == climate_option_current_value(ctx, kind);\n"
            "}\n"
        ),
        ("match climate option state without case sensitivity",),
    )
    expect_climate_option_selection_errors(
        "climate option selection ignores attribute case",
        (
            "inline bool climate_option_selected(ClimateControlCtx *ctx,\n"
            "                                    const std::string &kind,\n"
            "                                    const std::string &value) {\n"
            "  if (!ctx) return false;\n"
            "  std::string current = climate_option_current_value(ctx, kind);\n"
            "  return climate_lower(climate_trim(value)) == climate_lower(climate_trim(current));\n"
            "}\n"
        ),
        (),
    )
    expect_network_status_version_errors(
        "raw local firmware version leaks",
        (
            "inline std::string network_status_firmware_label(const std::string &version) {\n"
            "  std::string trimmed = version;\n"
            "  if (trimmed.empty()) return espcontrol_i18n(std::string(\"Version unknown\"));\n"
            "  if (trimmed == \"dev\" || trimmed == \"0.0.0\") return espcontrol_i18n(std::string(\"Dev build\"));\n"
            "  return trimmed;\n"
            "}\n"
        ),
        (
            "classify release versions before labeling firmware",
            "show only release versions as installed versions",
        ),
    )
    expect_network_status_version_errors(
        "release-only firmware version label",
        (
            "inline bool network_status_is_specific_firmware_version(const std::string &version) { return true; }\n"
            "inline std::string network_status_firmware_label(const std::string &version) {\n"
            "  std::string trimmed = version;\n"
            "  if (trimmed.empty()) return espcontrol_i18n(std::string(\"Version unknown\"));\n"
            "  if (network_status_is_specific_firmware_version(trimmed)) return trimmed;\n"
            "  return espcontrol_i18n(std::string(\"Dev build\"));\n"
            "}\n"
        ),
        (),
    )
    expect_modal_tab_layout_errors(
        "shared modal tab layout",
        valid_modal_tab_layout_files(),
        (),
    )
    old_tab_layout = valid_modal_tab_layout_files()
    old_tab_layout["components/espcontrol/button_grid_fan.h"] = (
        old_tab_layout["components/espcontrol/button_grid_fan.h"]
        + "lv_coord_t selected_tab_size = tab_size + tab_size / 8;\n"
    )
    expect_modal_tab_layout_errors(
        "modal tab layout drifts back to local sizing",
        old_tab_layout,
        ("keep modal tab sizing in button_grid_modal_layout.h",),
    )
    missing_shared_tab_helper = valid_modal_tab_layout_files()
    missing_shared_tab_helper["components/espcontrol/button_grid_media.h"] = (
        "lv_coord_t selected_tab_size = tab_size + tab_size / 8;\n"
    )
    expect_modal_tab_layout_errors(
        "media modal stops using shared tab helper",
        missing_shared_tab_helper,
        ("use shared modal tab layout helpers",),
    )
    expect_media_modal_progress_layout_errors(
        "media progress layout gated",
        valid_media_modal_progress_layout_text(),
        (),
    )
    expect_media_modal_progress_layout_errors(
        "media progress draws before layout",
        valid_media_modal_progress_layout_text().replace(
            "  if (!ui.progress_layout_ready) {\n"
            "    ui.progress_refresh_pending = true;\n"
            "    return;\n"
            "  }\n",
            "",
        ),
        ("progress drawing gated",),
    )
    home_idle_gated = valid_sleep_takeover_files()
    home_idle_gated["common/addon/backlight.yaml"] = home_idle_gated[
        "common/addon/backlight.yaml"
    ].replace(
        "          navigation_return_home(id(main_page)->obj);\n",
        "          if (id(display_takeover_suspended)) return;\n"
        "          navigation_return_home(id(main_page)->obj);\n",
    )
    expect_sleep_takeover_errors(
        "home return gated by display takeover",
        home_idle_gated,
        ("do not gate home-return idle",),
    )
    image_guard_stops_home_idle = valid_sleep_takeover_files()
    image_guard_stops_home_idle["scripts/generate_device_slots.py"] = (
        "cfg.begin_display_takeover = [](espcontrol::DisplayTakeoverKind kind) {\n"
        "  id(display_takeover_begin).execute(static_cast<int>(kind));\n"
        "  id(home_screen_idle_check).stop();\n"
        "};\n"
        "cfg.end_display_takeover = [](espcontrol::DisplayTakeoverKind kind) {\n"
        "  id(display_takeover_end).execute(static_cast<int>(kind));\n"
        "};\n"
    )
    expect_sleep_takeover_errors(
        "image modal display guard stops home return",
        image_guard_stops_home_idle,
        ("must not stop the home-return timer",),
    )
    missing_takeover_reconcile = valid_sleep_takeover_files()
    missing_takeover_reconcile["common/addon/backlight.yaml"] = (
        missing_takeover_reconcile["common/addon/backlight.yaml"].replace(
            "      - script.execute: display_mode_reconcile\n"
            "  - id: screensaver_sleep_timer\n",
            "  - id: screensaver_sleep_timer\n",
        )
    )
    expect_sleep_takeover_errors(
        "takeover end misses request resolution",
        missing_takeover_reconcile,
        ("resolve current requests when a takeover ends",),
    )
    print("Firmware modal allocation self-tests passed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run guardrail self-tests")
    args = parser.parse_args()
    return run_self_test() if args.self_test else run_scan()


if __name__ == "__main__":
    raise SystemExit(main())
