#!/usr/bin/env python3
"""Guard firmware display sizing decisions behind display/modal helpers."""

from __future__ import annotations

import argparse
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = ROOT / "components" / "espcontrol"

DISPLAY_BOUNDARY_FILES = {
    "button_grid_display.h",
    "button_grid_modal.h",
}

DISPLAY_TOKEN_FILES = DISPLAY_BOUNDARY_FILES | {
    "button_grid_sliders.h",
}

RULES: tuple[tuple[re.Pattern[str], str, set[str]], ...] = (
    (
        re.compile(r"\blv_disp_get_(?:hor|ver)_res\s*\("),
        "read display dimensions through button_grid_display.h/button_grid_modal.h helpers",
        DISPLAY_BOUNDARY_FILES,
    ),
    (
        re.compile(
            r"(?:\blayout\.(?:sw|sh)\s*(?:==|!=|<=|>=|<|>)\s*-?\d|"
            r"-?\d+\s*(?:==|!=|<=|>=|<|>)\s*layout\.(?:sw|sh)\b)"
        ),
        "route modal screen-size tuning through named modal display helpers",
        DISPLAY_BOUNDARY_FILES,
    ),
    (
        re.compile(r"\bdisplay_modal_is_[A-Za-z0-9_]*_size\s*\("),
        "select a declarative modal profile instead of inferring one from display dimensions",
        set(),
    ),
    (
        re.compile(r"\b[A-Za-z0-9_]*(?:jc1060|jc4880|jc8012|p4_86|4848)[A-Za-z0-9_]*\b", re.IGNORECASE),
        "name modal layout decisions by semantic profile rather than a device model",
        set(),
    ),
    (
        re.compile(r"\b(?:CONTROL|DISPLAY)_MODAL_[A-Z0-9_]+_REF_PX\b"),
        "keep modal reference tokens inside the display/modal token boundary",
        DISPLAY_TOKEN_FILES,
    ),
)

# YAML substitutions and the runtime C++ palette are authored independently.
# Guard their dark RGB parity until both can consume one shared source.
THEME_RGB = {
    "BACKGROUND": 0x303030,
    "SURFACE_PRIMARY": 0x303030,
    "SURFACE_SECONDARY": 0x404040,
    "TEXT_PRIMARY": 0xFFFFFF,
    "CLOCKBAR_TEXT": 0xFFFFFF,
    "TEXT_MUTED": 0xB0B0B0,
    "TEXT_INVERTED": 0x000000,
    "TEXT_DISABLED": 0x707070,
    "BORDER": 0x303030,
    "CONTROL_NEUTRAL": 0x303030,
    "TRACK_BACKGROUND": 0x313131,
    "OVERLAY": 0x000000,
    "SETUP_ACTION": 0x333333,
}
LIGHT_THEME_RGB = {
    "BACKGROUND": 0xE3E3E3,
    "SURFACE_PRIMARY": 0xB0B0B0,
    "SURFACE_SECONDARY": 0xFFFFFF,
    "TEXT_PRIMARY": 0x333333,
    "CLOCKBAR_TEXT": 0x333333,
    "TEXT_MUTED": 0x606060,
    "TEXT_INVERTED": 0xFFFFFF,
    "TEXT_DISABLED": 0x9A9A9A,
    "BORDER": 0xD0D0D0,
    "CONTROL_NEUTRAL": 0xE0E0E0,
    "TRACK_BACKGROUND": 0xD0D0D0,
    "OVERLAY": 0x000000,
    "SETUP_ACTION": 0xE0E0E0,
}
LEGACY_NEUTRAL_NAMES = re.compile(
    r"\b(?:DARK_(?:TEXT_PRIMARY|TEXT_INVERTED|TEXT_MUTED|TEXT_SOFT|"
    r"TEXT_DISABLED|BORDER|CONTROL_NEUTRAL|OVERLAY|TRACK_BACKGROUND)|"
    r"SECONDARY_GREY|TERTIARY_GREY|DEFAULT_(?:SECONDARY|TERTIARY|OFF)_COLOR(?:_RAW)?)\b"
)


def check_theme_colors(root: Path) -> list[str]:
    yaml_path = root / "common/theme/colors.yaml"
    palette_path = root / "components/espcontrol/theme_palette.h"
    accent_path = root / "components/espcontrol/button_grid_style.h"
    if not yaml_path.exists() and not palette_path.exists():
        return []  # Isolated display-token self-test fixtures.
    failures: list[str] = []
    if not yaml_path.exists() or not palette_path.exists() or not accent_path.exists():
        return ["semantic theme colors need YAML, the runtime palette, and the accent boundary"]
    yaml_text = yaml_path.read_text(encoding="utf-8")
    palette_text = palette_path.read_text(encoding="utf-8")
    accent_text = accent_path.read_text(encoding="utf-8")
    preview_path = root / "src/webserver/state/preview_theme.ts"
    if not preview_path.exists():
        failures.append("preview_theme.ts: define the device preview palettes")
    else:
        preview_text = preview_path.read_text(encoding="utf-8")
        for mode, colors in (("Dark", THEME_RGB), ("Light", LIGHT_THEME_RGB)):
            block = re.search(rf"\b{mode}:\s*\{{([^}}]+)\}}", preview_text)
            for role in ("BACKGROUND", "SURFACE_PRIMARY", "SURFACE_SECONDARY", "TEXT_PRIMARY",
                         "TEXT_MUTED", "TEXT_DISABLED", "BORDER", "TRACK_BACKGROUND", "CONTROL_NEUTRAL"):
                words = role.lower().split("_")
                field = words[0] + "".join(word.title() for word in words[1:])
                value = re.search(rf'\b{field}:\s*"([0-9A-Fa-f]{{6}})"', block.group(1)) if block else None
                if value is None or int(value.group(1), 16) != colors[role]:
                    failures.append(f"preview_theme.ts: {mode} {field} must match firmware 0x{colors[role]:06X}")
    for role, rgb in THEME_RGB.items():
        yaml_name = f"theme_{role.lower()}_color"
        yaml_value = re.search(rf"^  {yaml_name}: [\"']?0x([0-9A-Fa-f]{{6}})[\"']?$", yaml_text, re.M)
        if yaml_value is None or int(yaml_value.group(1), 16) != rgb:
            failures.append(f"common/theme/colors.yaml: {yaml_name} must remain 0x{rgb:06X}")
        field = role.lower()
        cpp_value = re.search(rf"\btheme\.{field}\s*=\s*0x([0-9A-Fa-f]{{6}});", palette_text)
        if cpp_value is None or int(cpp_value.group(1), 16) != rgb:
            failures.append(f"theme_palette.h: {field} must remain raw RGB 0x{rgb:06X}")
    if "inline constexpr ThemePalette DARK_THEME = make_dark_theme();" not in palette_text:
        failures.append("theme_palette.h: define the concrete Dark palette")
    light_match = re.search(r"inline constexpr ThemePalette LIGHT_THEME\s*=\s*\{([^}]+)\};", palette_text, re.S)
    light_values = [int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{6})", light_match.group(1))] if light_match else []
    if light_values != list(LIGHT_THEME_RGB.values()):
        failures.append("theme_palette.h: Light roles must match the reviewed raw RGB palette")
    if "inline const ThemePalette &current_theme()" not in palette_text:
        failures.append("theme_palette.h: provide runtime current-theme access")
    if "constexpr uint32_t DEFAULT_ACCENT_COLOR_RAW = 0xFF8C00;" not in accent_text:
        failures.append("button_grid_style.h: preserve the separate default user accent")
    neutral_global = re.compile(r"\bTHEME_(?:BACKGROUND|SURFACE_[A-Z_]+|TEXT_[A-Z_]+|BORDER|CONTROL_NEUTRAL|TRACK_BACKGROUND|OVERLAY)\b")
    for path in (root / "components/espcontrol").glob("*.h"):
        text = path.read_text(encoding="utf-8")
        if LEGACY_NEUTRAL_NAMES.search(text) or neutral_global.search(text):
            failures.append(f"{path.relative_to(root)}: consume the runtime palette, not global neutral colors")
    return failures


def firmware_headers(root: Path) -> list[Path]:
    firmware_dir = root / "components" / "espcontrol"
    return sorted(firmware_dir.glob("button_grid*.h"))


def check_root(root: Path) -> list[str]:
    failures: list[str] = []
    failures.extend(check_theme_colors(root))
    for path in firmware_headers(root):
        filename = path.name
        lines = path.read_text(encoding="utf-8").splitlines()
        constant_lines: dict[str, int] = {}
        for line_no, line in enumerate(lines, start=1):
            constant = re.search(r"\bconstexpr\s+[^;=]+\b([A-Z][A-Z0-9_]+)\s*=", line)
            if constant:
                name = constant.group(1)
                if name in constant_lines:
                    rel = path.relative_to(root)
                    failures.append(
                        f"{rel}:{line_no}: keep firmware constant names unique "
                        f"({name} was first declared on line {constant_lines[name]})"
                    )
                else:
                    constant_lines[name] = line_no
            for pattern, message, allowed_files in RULES:
                if filename in allowed_files:
                    continue
                if pattern.search(line):
                    rel = path.relative_to(root)
                    failures.append(f"{rel}:{line_no}: {message}")

    tab_files = {
        "button_grid_modal.h": 0,
        "button_grid_sliders.h": 2,
        "button_grid_fan.h": 1,
        "button_grid_climate.h": 1,
        "button_grid_media.h": 1,
    }
    tab_paths = {name: FIRMWARE_DIR.relative_to(ROOT) / name for name in tab_files}
    resolved_tab_paths = {name: root / relative for name, relative in tab_paths.items()}
    if all(path.exists() for path in resolved_tab_paths.values()):
        modal_text = resolved_tab_paths["button_grid_modal.h"].read_text(encoding="utf-8")
        if "apply_width_compensation(tab_row, width_compensation_percent);" not in modal_text:
            failures.append(
                "components/espcontrol/button_grid_modal.h: compensate the shared tab controller container"
            )
        tab_button_body = re.search(
            r"inline void control_modal_layout_tab_button\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            modal_text,
            re.S,
        )
        if tab_button_body is None or "apply_width_compensation(tab_btn" in tab_button_body.group("body"):
            failures.append(
                "components/espcontrol/button_grid_modal.h: apply tab compensation once at the shared container"
            )
        compensated_call = re.compile(
            r"control_modal_apply_tab_row\(\s*ui\.tab_row,\s*layout,\s*tabs_layout,\s*"
            r"ctx->width_compensation_percent\s*\);"
        )
        for name, expected_count in tab_files.items():
            if expected_count == 0:
                continue
            text = resolved_tab_paths[name].read_text(encoding="utf-8")
            if len(compensated_call.findall(text)) != expected_count:
                failures.append(
                    f"components/espcontrol/{name}: pass display compensation to every modal tab controller"
                )
        tab_creators = {
            "button_grid_sliders.h": (
                "light_control_create_tab_button",
                "cover_control_create_tab_button",
            ),
            "button_grid_climate.h": ("climate_control_create_tab_button",),
            "button_grid_media.h": ("media_control_create_tab_button",),
            "button_grid_fan.h": ("fan_control_create_tab_button",),
        }
        for name, creators in tab_creators.items():
            text = resolved_tab_paths[name].read_text(encoding="utf-8")
            for creator in creators:
                body = re.search(
                    rf"inline lv_obj_t \*{creator}\(.*?\n\}}",
                    text,
                    re.S,
                )
                if body is None or "width_compensation_percent" in body.group(0):
                    failures.append(
                        f"components/espcontrol/{name}: compensate tab icons once through the shared row"
                    )

    alarm_path = root / FIRMWARE_DIR.relative_to(ROOT) / "button_grid_alarm.h"
    if alarm_path.exists():
        alarm_text = alarm_path.read_text(encoding="utf-8")
        mode_button = re.search(
            r"inline lv_obj_t \*alarm_control_create_mode_button\(.*?\n\}",
            alarm_text,
            re.S,
        )
        if (
            "apply_width_compensation(ui.rail, ctx->width_compensation_percent);"
            not in alarm_text
            or mode_button is None
            or "apply_width_compensation" in mode_button.group(0)
        ):
            failures.append(
                "components/espcontrol/button_grid_alarm.h: compensate alarm action icons once through the shared rail"
            )
    return failures


def run_self_test() -> None:
    cases: tuple[tuple[dict[str, str], tuple[str, ...]], ...] = (
        (
            {"button_grid_climate.h": "if (layout.sw == 480 && layout.sh == 480) return true;\n"},
            ("route modal screen-size tuning through named modal display helpers",),
        ),
        (
            {"button_grid_climate.h": "if (layout.sw <= 480 || layout.sh >= 480) return true;\n"},
            ("route modal screen-size tuning through named modal display helpers",),
        ),
        (
            {"button_grid_climate.h": "if (480 >= layout.sw || 480 <= layout.sh) return true;\n"},
            ("route modal screen-size tuning through named modal display helpers",),
        ),
        (
            {"button_grid_climate.h": "if (layout.sh > layout.sw) return true;\n"},
            (),
        ),
        (
            {"button_grid_alarm.h": "auto w = lv_disp_get_hor_res(disp);\n"},
            ("read display dimensions through button_grid_display.h/button_grid_modal.h helpers",),
        ),
        (
            {"button_grid_alarm.h": "if (display_modal_is_compact_size(layout)) return;\n"},
            ("select a declarative modal profile instead of inferring one from display dimensions",),
        ),
        (
            {"button_grid_climate.h": "auto px = CONTROL_MODAL_BUTTON_REF_PX;\n"},
            ("keep modal reference tokens inside the display/modal token boundary",),
        ),
        (
            {"button_grid_modal.h": "return display_modal_is_jc4880p443_size(layout.sw, layout.sh);\n"},
            (
                "select a declarative modal profile instead of inferring one from display dimensions",
                "name modal layout decisions by semantic profile rather than a device model",
            ),
        ),
        (
            {"button_grid_climate.h": "if (control_modal_uses_compact_square_tuning(layout)) return true;\n"},
            (),
        ),
        (
            {"button_grid_climate.h": "if (control_modal_uses_jc1060_tuning(layout)) return true;\n"},
            ("name modal layout decisions by semantic profile rather than a device model",),
        ),
        (
            {
                "button_grid_climate.h": (
                    "constexpr int CLIMATE_MODAL_WIDE_OPTION_GAP = 12;\n"
                    "constexpr int CLIMATE_MODAL_WIDE_OPTION_GAP = 16;\n"
                )
            },
            ("keep firmware constant names unique",),
        ),
        (
            {
                "button_grid_alarm.h": (
                    "inline lv_obj_t *alarm_control_create_mode_button() {\n"
                    "  apply_width_compensation(icon, 95);\n"
                    "}\n"
                    "apply_width_compensation(ui.rail, ctx->width_compensation_percent);\n"
                )
            },
            ("compensate alarm action icons once through the shared rail",),
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
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "common/theme").mkdir(parents=True)
        (root / "components/espcontrol").mkdir(parents=True)
        yaml_path = root / "common/theme/colors.yaml"
        cpp_path = root / "components/espcontrol/theme_palette.h"
        accent_path = root / "components/espcontrol/button_grid_style.h"
        preview_path = root / "src/webserver/state/preview_theme.ts"
        preview_path.parent.mkdir(parents=True)
        preview_path.write_text((ROOT / "src/webserver/state/preview_theme.ts").read_text(encoding="utf-8"), encoding="utf-8")
        yaml_path.write_text((ROOT / "common/theme/colors.yaml").read_text(encoding="utf-8"), encoding="utf-8")
        cpp_path.write_text((ROOT / "components/espcontrol/theme_palette.h").read_text(encoding="utf-8"), encoding="utf-8")
        accent_path.write_text((ROOT / "components/espcontrol/button_grid_style.h").read_text(encoding="utf-8"), encoding="utf-8")
        assert not check_theme_colors(root)
        yaml_path.write_text(yaml_path.read_text(encoding="utf-8").replace("0xB0B0B0", "0xB0B0B1"), encoding="utf-8")
        assert any("theme_text_muted_color" in failure for failure in check_theme_colors(root))
        yaml_path.write_text((ROOT / "common/theme/colors.yaml").read_text(encoding="utf-8"), encoding="utf-8")
        cpp_path.write_text(cpp_path.read_text(encoding="utf-8").replace("theme.text_muted = 0xB0B0B0", "theme.text_muted = 0xB0B0B1"), encoding="utf-8")
        assert any("text_muted" in failure for failure in check_theme_colors(root))
        cpp_path.write_text((ROOT / "components/espcontrol/theme_palette.h").read_text(encoding="utf-8"), encoding="utf-8")
        preview_path.write_text(preview_path.read_text(encoding="utf-8").replace('background: "F4F4F4"', 'background: "F4F4F5"'), encoding="utf-8")
        assert any("Light background" in failure for failure in check_theme_colors(root))
    print("Firmware display token self-tests passed.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        run_self_test()
        return 0

    failures = check_root(ROOT)
    if failures:
        print("Firmware display token guard failed:")
        for failure in failures:
            print(f"  {failure}")
        return 1
    print("Firmware display token checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
