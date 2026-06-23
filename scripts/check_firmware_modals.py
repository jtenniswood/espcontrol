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
    layer_top_pattern = re.compile(r"\blv_obj_create\s*\(\s*lv_layer_top\s*\(\s*\)\s*\)")
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
                    f"{rel}:{line_no}: open modal overlays through button_grid_modal.h helpers"
                )
            if path.name not in MANUAL_OVERLAY_DELETE_ALLOWLIST and manual_overlay_delete_pattern.search(line):
                rel = path.relative_to(root)
                errors.append(
                    f"{rel}:{line_no}: delete modal overlays through button_grid_modal.h lifecycle helpers"
                )
    return errors


def firmware_modal_sleep_takeover_errors(root: Path) -> list[str]:
    firmware_dir = root / "components" / "espcontrol"
    backlight_header_path = firmware_dir / "backlight.h"
    modal_path = firmware_dir / "button_grid_modal.h"
    navigation_path = firmware_dir / "button_grid_navigation.h"
    grid_path = firmware_dir / "button_grid_grid.h"
    image_path = firmware_dir / "button_grid_image.h"
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
        if "control_modal_force_close_active" not in text or "control_modal_close_active_internal(false)" not in text:
            errors.append(
                "components/espcontrol/button_grid_modal.h: provide a forced modal close path for display takeover"
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
        if hide_modals is None or "control_modal_close_active();" not in hide_modals.group("body"):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: return-home navigation must close active shared modals"
            )
        if return_home is None or "navigation_hide_modals();" not in return_home.group("body"):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: return-home navigation must use the shared modal close path"
            )
        if (
            "navigation_close_modals_for_display_takeover" not in text
            or "control_modal_force_close_active();" not in text
        ):
            errors.append(
                "components/espcontrol/button_grid_navigation.h: close modals through a display-takeover helper"
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
            "suspend_display_takeover" not in text
            or "resume_display_takeover" not in text
            or "ctx->suspend_display_takeover" not in text
            or "ctx->resume_display_takeover" not in text
        ):
            errors.append(
                "components/espcontrol/button_grid_image.h: keep image modal display-takeover suspend/resume hooks"
            )

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
        if "display_takeover_suspended" not in text:
            errors.append("common/addon/backlight.yaml: track display-takeover suspension explicitly")
        if "screensaver_sensor_sleep_pending" not in text:
            errors.append("common/addon/backlight.yaml: preserve pending sensor-mode sleep while image modals are active")
        sleep_timer_body = yaml_script_body(text, "screensaver_sleep_timer")
        if sleep_timer_body is None:
            errors.append("common/addon/backlight.yaml: keep the screensaver sleep timer script")
        elif (
            "display_takeover_suspended" not in sleep_timer_body
            or "Skipping automatic sleep while image modal is active" not in sleep_timer_body
        ):
            errors.append(
                "common/addon/backlight.yaml: block automatic screensaver sleep while image modals are active"
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
        if "Skipping automatic display-off while image modal is active" not in text:
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
            "id(screensaver_sensor_sleep_pending)" not in text
            or "id(screensaver_sleep_sensor).execute();" not in text
            or "!id(presence_detected)" not in text
        ):
            errors.append(
                "scripts/generate_device_slots.py: re-check pending sensor-mode sleep when image modals close"
            )
        if (
            "cfg.suspend_display_takeover" not in text
            or "cfg.resume_display_takeover" not in text
            or "id(display_takeover_suspended) = true;" not in text
            or "id(display_takeover_suspended) = false;" not in text
        ):
            errors.append("scripts/generate_device_slots.py: generate explicit display-takeover guard hooks")

    if not schedule_path.exists():
        errors.append("common/addon/backlight_schedule.yaml: close modals before scheduled takeover")
    else:
        text = schedule_path.read_text(encoding="utf-8")
        if text.count("backlight_close_modals_for_display_takeover();") < 2:
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

    if not subpages_path.exists():
        errors.append("components/espcontrol/button_grid_subpages.h: preserve light control tab options in subpages")
        return errors

    subpages_text = subpages_path.read_text(encoding="utf-8")
    if (
        'b.type == "light_control"' not in subpages_text
        or "light_control_card_options_normalized(b.options)" not in subpages_text
    ):
        errors.append("components/espcontrol/button_grid_subpages.h: preserve light control tab options in subpages")
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
    if "if (show_tab_bar) lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: keep cover modal tab row hidden for single-control modals")
    if "lv_coord_t content_top = show_tab_bar" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: position cover modal content from explicit top and bottom bounds")
    if "lv_coord_t content_center_y = content_top + content_h / 2 - layout.panel_h / 2;" not in text:
        errors.append("components/espcontrol/button_grid_sliders.h: center cover modal controls within their available space")

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


def run_scan() -> int:
    errors = firmware_modal_errors(FIRMWARE_DIR, ROOT)
    errors.extend(firmware_modal_sleep_takeover_errors(ROOT))
    errors.extend(firmware_subpage_modal_wiring_errors(ROOT))
    errors.extend(firmware_light_control_brightness_errors(ROOT))
    errors.extend(firmware_cover_control_tab_errors(ROOT))
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
        path.write_text(grid_text, encoding="utf-8")
        (path.parent / "button_grid_subpages.h").write_text(
            'if (b.type == "light_control") {\n'
            "  b.options = light_control_card_options_normalized(b.options);\n"
            "}\n"
            'if (!b.type.empty() && b.type != "light_control") {\n'
            "  b.options.clear();\n"
            "}\n",
            encoding="utf-8",
        )

        errors = firmware_subpage_modal_wiring_errors(root)
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


def valid_sleep_takeover_files() -> dict[str, str]:
    return {
        "components/espcontrol/backlight.h": (
            "inline void set_backlight_display_takeover_callback(BacklightDisplayTakeoverCallback callback) {}\n"
            "inline void backlight_close_modals_for_display_takeover() {}\n"
        ),
        "components/espcontrol/button_grid_modal.h": (
            "inline void control_modal_close_active_internal(bool honor_close_guard) {}\n"
            "inline void control_modal_force_close_active() { control_modal_close_active_internal(false); }\n"
        ),
        "components/espcontrol/button_grid_navigation.h": (
            "inline void navigation_hide_modals() {\n"
            "  control_modal_close_active();\n"
            "}\n"
            "inline bool navigation_return_home(lv_obj_t *main_page_obj) {\n"
            "  navigation_hide_modals();\n"
            "  return true;\n"
            "}\n"
            "inline void navigation_close_modals_for_display_takeover() {\n"
            "  control_modal_force_close_active();\n"
            "}\n"
        ),
        "components/espcontrol/button_grid_grid.h": (
            "set_backlight_display_takeover_callback(navigation_close_modals_for_display_takeover);\n"
        ),
        "components/espcontrol/button_grid_image.h": (
            "std::function<void()> suspend_display_takeover;\n"
            "std::function<void()> resume_display_takeover;\n"
            "if (ctx->suspend_display_takeover) ctx->suspend_display_takeover();\n"
            "if (ctx && ctx->resume_display_takeover) ctx->resume_display_takeover();\n"
        ),
        "common/addon/backlight.yaml": (
            "globals:\n"
            "  - id: display_takeover_suspended\n"
            "  - id: screensaver_sensor_sleep_pending\n"
            "script:\n"
            "  - id: screensaver_sleep_timer\n"
            "    then:\n"
            "      - if:\n"
            "          condition:\n"
            "            lambda: 'return !id(display_takeover_suspended);'\n"
            "          else:\n"
            "            - lambda: 'Skipping automatic sleep while image modal is active'\n"
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
            "cfg.suspend_display_takeover = []() {\n"
            "  id(display_takeover_suspended) = true;\n"
            "  id(screensaver_idle_check).stop();\n"
            "};\n"
            "cfg.resume_display_takeover = []() {\n"
            "  id(display_takeover_suspended) = false;\n"
            "  if (id(screensaver_sensor_sleep_pending) && !id(presence_detected)) {\n"
            "    id(screensaver_sleep_sensor).execute();\n"
            "  }\n"
            "  id(home_screen_idle_check).execute();\n"
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
        ("open modal overlays through button_grid_modal.h helpers",),
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
            "provide a forced modal close path for display takeover",
            "close modals through a display-takeover helper",
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
    home_idle_gated = valid_sleep_takeover_files()
    home_idle_gated["common/addon/backlight.yaml"] = (
        "globals:\n"
        "  - id: display_takeover_suspended\n"
        "  - id: screensaver_sensor_sleep_pending\n"
        "script:\n"
        "  - id: screensaver_sleep_timer\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return !id(display_takeover_suspended);'\n"
        "          else:\n"
        "            - lambda: 'Skipping automatic sleep while image modal is active'\n"
        "  - id: home_screen_idle_check\n"
        "    then:\n"
        "      - if:\n"
        "          condition:\n"
        "            lambda: 'return !id(display_takeover_suspended);'\n"
        "          then:\n"
        "            - lambda: 'navigation_return_home(id(main_page)->obj);'\n"
        "Skipping automatic display-off while image modal is active\n"
        "backlight_close_modals_for_display_takeover();\n"
    )
    expect_sleep_takeover_errors(
        "home return gated by display takeover",
        home_idle_gated,
        ("do not gate home-return idle",),
    )
    image_guard_stops_home_idle = valid_sleep_takeover_files()
    image_guard_stops_home_idle["scripts/generate_device_slots.py"] = (
        "cfg.suspend_display_takeover = []() {\n"
        "  id(display_takeover_suspended) = true;\n"
        "  id(home_screen_idle_check).stop();\n"
        "};\n"
        "cfg.resume_display_takeover = []() {\n"
        "  id(display_takeover_suspended) = false;\n"
        "  if (id(screensaver_sensor_sleep_pending) && !id(presence_detected)) {\n"
        "    id(screensaver_sleep_sensor).execute();\n"
        "  }\n"
        "  id(home_screen_idle_check).execute();\n"
        "};\n"
    )
    expect_sleep_takeover_errors(
        "image modal display guard stops home return",
        image_guard_stops_home_idle,
        ("must not stop the home-return timer",),
    )
    missing_sensor_resume = valid_sleep_takeover_files()
    missing_sensor_resume["scripts/generate_device_slots.py"] = (
        "cfg.suspend_display_takeover = []() {\n"
        "  id(display_takeover_suspended) = true;\n"
        "  id(screensaver_idle_check).stop();\n"
        "};\n"
        "cfg.resume_display_takeover = []() {\n"
        "  id(display_takeover_suspended) = false;\n"
        "  id(home_screen_idle_check).execute();\n"
        "};\n"
    )
    expect_sleep_takeover_errors(
        "image modal close misses pending sensor sleep",
        missing_sensor_resume,
        ("re-check pending sensor-mode sleep",),
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
