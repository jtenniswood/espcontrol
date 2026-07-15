#!/usr/bin/env python3
"""Generate the host C++ saved-configuration parser fixture test."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONFIG_DIR = ROOT / "common" / "config"
FIELDS = ("entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options")
FIXTURE_SUFFIX = "_card_normalization_fixtures.json"


def cpp_string(value: str) -> str:
    return json.dumps(value)


def fixture_groups() -> list[tuple[str, list[dict]]]:
    shared = json.loads((CONFIG_DIR / "card_normalization_fixtures.json").read_text(encoding="utf-8"))
    groups = [(label, fixtures) for label, fixtures in sorted(shared.items())]
    for path in sorted(CONFIG_DIR.glob(f"*{FIXTURE_SUFFIX}")):
        label = path.name[:-len(FIXTURE_SUFFIX)].replace("_", " ")
        groups.append((label, json.loads(path.read_text(encoding="utf-8"))))
    return groups


def identifier(value: str) -> str:
    return "".join(char if char.isalnum() else "_" for char in value.lower())


def generate() -> str:
    lines = [
        "// Generated from common/config/*_card_normalization_fixtures.json.",
        "#include <cassert>",
        "#include <cstdint>",
        "#include <string>",
        "#include <vector>",
        '#include "esphome/core/string_ref.h"',
        "",
        "struct lv_obj_t {};",
        "inline void lv_label_set_text(lv_obj_t *, const char *) {}",
        "inline const char *espcontrol_i18n(const char *text) { return text ? text : \"\"; }",
        "inline std::string espcontrol_i18n(const std::string &text) { return text; }",
        '#include "button_grid_config_parser.h"',
        "#define ESPCONTROL_SUBPAGE_PARSER_ONLY",
        '#include "button_grid_subpages.h"',
        "",
        "int main() {",
    ]
    for group, fixtures in fixture_groups():
        lines.append(f"  // {group}")
        for fixture in fixtures:
            base = f"fixture_{identifier(group)}_{identifier(fixture['name'])}"
            cases = [("input", fixture["input"])]
            if "canonical" in fixture:
                cases.append(("canonical", fixture["canonical"]))
            for case_name, encoded in cases:
                variable = base if case_name == "input" else f"{base}_{case_name}"
                lines.append(f"  const auto {variable} = parse_cfg({cpp_string(encoded)});")
                for field in FIELDS:
                    lines.append(
                        f"  assert({variable}.{field} == {cpp_string(fixture['expected'][field])});"
                    )
    lines.extend(
        (
            "  // Time sensor duration formatting uses at most two non-zero components.",
            "  char duration[48];",
            '  assert(format_duration_value(duration, sizeof(duration), 0.0, "seconds") && std::string(duration) == "0s");',
            '  assert(format_duration_value(duration, sizeof(duration), 59.0, "s") && std::string(duration) == "59s");',
            '  assert(format_duration_value(duration, sizeof(duration), 60.0, "seconds") && std::string(duration) == "1m");',
            '  assert(format_duration_value(duration, sizeof(duration), 90.0, "seconds") && std::string(duration) == "1m 30s");',
            '  assert(format_duration_value(duration, sizeof(duration), 3599.0, "seconds") && std::string(duration) == "59m 59s");',
            '  assert(format_duration_value(duration, sizeof(duration), 60.0, "minutes") && std::string(duration) == "1h");',
            '  assert(format_duration_value(duration, sizeof(duration), 86340.0, "seconds") && std::string(duration) == "23h 59m");',
            '  assert(format_duration_value(duration, sizeof(duration), 24.0, "hours") && std::string(duration) == "1d");',
            '  assert(format_duration_value(duration, sizeof(duration), 28.0, "hours") && std::string(duration) == "1d 4h");',
            '  assert(format_duration_value(duration, sizeof(duration), 0.6, "hours") && std::string(duration) == "36m");',
            '  assert(format_duration_value(duration, sizeof(duration), 1.999, "seconds") && std::string(duration) == "1s");',
            '  assert(format_duration_value(duration, sizeof(duration), 1500.0, "ms") && std::string(duration) == "1s");',
            '  assert(format_duration_value(duration, sizeof(duration), 1500000.0, "µs") && std::string(duration) == "1s");',
            '  assert(format_duration_value(duration, sizeof(duration), 12345.0, "days") && std::string(duration) == "12345d");',
            '  assert(!format_duration_value(duration, sizeof(duration), -1.0, "seconds"));',
            '  assert(!format_duration_value(duration, sizeof(duration), std::numeric_limits<double>::quiet_NaN(), "seconds"));',
            '  assert(!format_duration_value(duration, sizeof(duration), std::numeric_limits<double>::infinity(), "seconds"));',
            '  assert(!format_duration_value(duration, sizeof(duration), 1.0, "weeks"));',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "0.6", true, "", false));',
            '  assert(format_duration_sensor_state(duration, sizeof(duration), "0.6", true, "h", true) && std::string(duration) == "36m");',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "", false, "h", true));',
            '  assert(format_duration_sensor_state(duration, sizeof(duration), "90", true, "s", true) && std::string(duration) == "1m 30s");',
            '  assert(format_duration_sensor_state(duration, sizeof(duration), "90", true, "min", true) && std::string(duration) == "1h 30m");',
            '  assert(format_duration_sensor_state(duration, sizeof(duration), "90", true, "unsupported", true, "seconds") && std::string(duration) == "1m 30s");',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "unavailable", true, "s", true));',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "unknown", true, "s", true));',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "12 seconds", true, "s", true));',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "-1", true, "s", true));',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "nan", true, "s", true));',
            '  assert(!format_duration_sensor_state(duration, sizeof(duration), "inf", true, "s", true));',
            "  // Media behaviour consumes a typed versioned view, not raw option strings.",
            '  ParsedCfg typed_media{"media_player.kitchen", "Kitchen", "Auto", "Auto", "playlist", "", "media", "",',
            '                        "volume_max=150,playlist_content_id=morning,playlist_player_source=Kitchen%2C Main,large_numbers"};',
            "  const auto media_config = espcontrol::media::decode_config_v1(typed_media);",
            "  assert(media_config.version == 1);",
            "  assert(media_config.mode == espcontrol::media::Mode::PLAYLIST);",
            "  assert(media_config.max_volume_percent == 100);",
            '  assert(media_config.playlist_content_id == "morning");',
            '  assert(media_config.playlist_content_type == "playlist");',
            '  assert(media_config.playlist_player_source == "Kitchen, Main");',
            "  assert(media_config.large_numbers);",
            '  ParsedCfg legacy_media{"", "", "", "", "controls", "", "media", "state", "volume_max=0"};',
            "  const auto legacy_media_config = espcontrol::media::decode_config_v1(legacy_media);",
            "  assert(legacy_media_config.mode == espcontrol::media::Mode::PLAY_PAUSE);",
            "  assert(legacy_media_config.state_display == espcontrol::media::StateDisplay::STATE);",
            "  assert(legacy_media_config.max_volume_percent == 1);",
        )
    )
    issue_248 = (
        "~B,,4,2,3,,,,8,9,,,1,6,5|X,,Office,Window Closed,Window Open,binary_sensor.office_window_sensor_opening,,window,active_color"
        "|X,,Linnea 1,Window Closed,Window Open,binary_sensor.linnea_br_window_sensor_opening,,window,active_color"
        "|X,,Linnea 2,Window Closed,Window Open,binary_sensor.linnea_br_window_2_sensor_opening,,window,active_color"
        "|X,,Maxime,Window Closed,Window Open,binary_sensor.maxime_br_window_sensor_opening,,window,active_color"
        "|X,,Study 2,Window Closed,Window Open,binary_sensor.study_window_2_sensor_opening,,window,active_color"
        "|X,,Study 1,Window Closed,Window Open,binary_sensor.study_window_2_sensor_opening,,window,active_color"
        "||X,,Master 1,Window Closed,Window Open,binary_sensor.master_bedroom_window_1_sensor,,window,active_color"
        "|X,,Master 2,Window Closed,Window Open,binary_sensor.master_bedroom_window_2_sensor,,window,active_color"
        "|X,,Kitchen,Window Closed,Window Open,binary_sensor.kitchen_window_sensor_opening,,window,active_color"
    )
    lines.extend(
        (
            "  // Issue 248: fixed-size storage chunks can split inside compact card data.",
            f"  const std::string issue_248_config = {cpp_string(issue_248)};",
            "  std::string issue_248_joined;",
            "  for (size_t offset = 0; offset < issue_248_config.size(); offset += 255) {",
            "    issue_248_joined += issue_248_config.substr(offset, 255);",
            "  }",
            "  assert(issue_248_joined == issue_248_config);",
            "  const auto issue_248_buttons = parse_subpage_config(issue_248_joined);",
            "  const auto issue_248_expected = parse_subpage_config(issue_248_config);",
            "  assert(issue_248_buttons.size() == issue_248_expected.size());",
            "  assert(issue_248_buttons.size() == 10);",
            "  for (size_t i = 0; i < issue_248_buttons.size(); ++i) {",
            "    assert(issue_248_buttons[i].entity == issue_248_expected[i].entity);",
            "    assert(issue_248_buttons[i].label == issue_248_expected[i].label);",
            "    assert(issue_248_buttons[i].type == issue_248_expected[i].type);",
            "  }",
            '  assert(issue_248_buttons.front().label == "Office");',
            '  assert(issue_248_buttons.front().sensor == "binary_sensor.office_window_sensor_opening");',
            '  assert(issue_248_buttons.back().label == "Kitchen");',
            '  assert(issue_248_buttons.back().sensor == "binary_sensor.kitchen_window_sensor_opening");',
        )
    )
    lines.extend(("  return 0;", "}", ""))
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(generate(), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
