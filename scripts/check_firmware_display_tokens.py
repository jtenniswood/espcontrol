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


def firmware_headers(root: Path) -> list[Path]:
    firmware_dir = root / "components" / "espcontrol"
    return sorted(firmware_dir.glob("button_grid*.h"))


def check_root(root: Path) -> list[str]:
    failures: list[str] = []
    for path in firmware_headers(root):
        filename = path.name
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            for pattern, message, allowed_files in RULES:
                if filename in allowed_files:
                    continue
                if pattern.search(line):
                    rel = path.relative_to(root)
                    failures.append(f"{rel}:{line_no}: {message}")
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
