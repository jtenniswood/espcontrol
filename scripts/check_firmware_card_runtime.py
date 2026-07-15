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
        if (
            "card_runtime_context(p)" not in text
            or "card_runtime_information_only(context)" not in text
            or "espcontrol::cards::Surface::SUBPAGE" not in text
            or "Legacy setup fallback" not in text
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: route main and subpage setup through the shared card context"
            )
        if 'parent_subpage_kind == "lawn_mower"' not in text or "lawn_mower_state_active_ref" not in text:
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: route mower subpage parent indicators through mower active-state handling"
            )
        if (
            'if (sb_cfg.type == "light_control")' not in text
            or "subscribe_light_control_state(ctx);\n          add_parent_indicator(sb_cfg.entity);" not in text
        ):
            failures.append(
                f"components/espcontrol/{GRID_HEADER}: include full light controls in generic subpage parent indicators"
            )
        image_reset_pos = text.find("reset_image_card_pool(cfg);")
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
            or "Legacy action fallback" not in click_body
        ):
            failures.append(
                f"components/espcontrol/{ACTION_HEADER}: route passive checks through the shared card context"
            )
    image_header = root / "components" / "espcontrol" / IMAGE_HEADER
    if image_header.exists():
        text = image_header.read_text(encoding="utf-8")
        reset_body = function_body(text, "reset_image_card_pool")
        if reset_body is None or "for (int i = 0; i < IMAGE_CARD_MAX_CONTEXTS; i++)" not in reset_body:
            failures.append(
                f"components/espcontrol/{IMAGE_HEADER}: reset every image-card context, including disabled slots"
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
