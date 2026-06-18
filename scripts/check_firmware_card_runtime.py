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

# Existing alarm UI code keeps a few local button-order arrays. The runtime
# guard still blocks direct generated-contract access in that file.
MODE_ARRAY_ALLOWLIST = CARD_RUNTIME_BOUNDARY_FILES | {
    "button_grid_alarm.h",
}

DIRECT_CONTRACT_PATTERN = re.compile(r"\b(?:card_contract_[A-Za-z0-9_]+|CARD_CONTRACT_[A-Z0-9_]+)\b")
MODE_ARRAY_PATTERN = re.compile(
    r"\{[^}\n]*\"(?:play_pause|previous|next|volume|position|now_playing|"
    r"open|close|stop|set_position|tilt|toggle|lock|unlock|away|home|night|vacation|disarm)\""
)
SERVICE_MAPPING_PATTERN = re.compile(
    r"\"(?:cover\.(?:open_cover|close_cover|stop_cover|set_cover_position)|"
    r"lock\.(?:lock|unlock)|"
    r"media_player\.(?:media_play_pause|media_previous_track|media_next_track)|"
    r"alarm_control_panel\.(?:alarm_arm_away|alarm_arm_home|alarm_arm_night|alarm_arm_vacation|alarm_disarm))\""
)


def service_mapping_line_allowed(line: str) -> bool:
    if "ESP_LOGW" in line:
        return True
    if "cover.set_cover_tilt_position" in line:
        return True
    return False


def firmware_headers(root: Path) -> list[Path]:
    return sorted((root / "components" / "espcontrol").glob("button_grid*.h"))


def check_root(root: Path) -> list[str]:
    failures: list[str] = []
    for path in firmware_headers(root):
        filename = path.name
        rel = path.relative_to(root)
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
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
