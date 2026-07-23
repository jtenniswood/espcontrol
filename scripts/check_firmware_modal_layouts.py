#!/usr/bin/env python3
"""Compile and exercise the pure firmware modal layout planner."""

from __future__ import annotations

import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from device_profiles import ROOT, load_device_profiles


FIXTURES = ROOT / "common" / "config" / "modal_layout_geometry_fixtures.json"
HEADER = ROOT / "components" / "espcontrol" / "button_grid_modal_layout.h"

FAMILY_ENUMS = {
    "compact-square": "COMPACT_SQUARE",
    "large-square": "LARGE_SQUARE",
    "compact-portrait": "COMPACT_PORTRAIT",
    "wide-landscape": "WIDE_LANDSCAPE",
    "large-landscape": "LARGE_LANDSCAPE",
}
DENSITY_ENUMS = {
    "compact": "COMPACT",
    "comfortable": "COMFORTABLE",
    "spacious": "SPACIOUS",
}
MEMORY_ENUMS = {
    "standard": "STANDARD",
    "constrained": "CONSTRAINED",
}


def parse_resolution(value: str) -> tuple[int, int]:
    match = re.fullmatch(r"([1-9]\d*)\s*x\s*([1-9]\d*)", value.strip())
    if not match:
        raise AssertionError(f"invalid public resolution: {value!r}")
    return int(match.group(1)), int(match.group(2))


def assert_profile_contract(fixtures: dict, profiles: dict[str, dict]) -> None:
    fixture_by_slug = {entry["slug"]: entry for entry in fixtures["layouts"]}
    missing = sorted(set(profiles) - set(fixture_by_slug))
    allowed_aliases = {"guition-esp32-p4-jc8012p4a1-v2"}
    unexpected = sorted(set(missing) - allowed_aliases)
    assert not unexpected, f"modal geometry fixtures missing devices: {', '.join(unexpected)}"

    for slug, profile in profiles.items():
        fixture_slug = "guition-esp32-p4-jc8012p4a1" if slug in allowed_aliases else slug
        fixture = fixture_by_slug[fixture_slug]
        modal = profile["firmware"]["display"]["modal"]
        expected_profile = fixture["profile"]
        assert modal["layoutFamily"] == expected_profile["family"], f"{slug}: modal family drifted"
        assert modal["density"] == expected_profile["density"], f"{slug}: modal density drifted"
        assert modal["memoryTier"] == expected_profile["memoryTier"], f"{slug}: modal memory tier drifted"
        assert modal["baseTouchTarget"] == expected_profile["baseTouchTarget"], (
            f"{slug}: modal base touch target drifted"
        )
        assert parse_resolution(profile["public"]["resolution"]) == (
            fixture["viewport"]["width"], fixture["viewport"]["height"]
        ), f"{slug}: fixture viewport does not match public resolution"


def cpp_assertions(entry: dict, index: int) -> list[str]:
    profile = entry["profile"]
    viewport = entry["viewport"]
    insets = entry["panelInsets"]
    expected = entry["expected"]
    panel = expected["panel"]
    back = expected["back"]
    arc = expected["arc"]
    tabs = expected["tabs"]
    content = expected["content"]
    prefix = f"layout_{index}"
    return [
        "  {",
        "    DeviceProfile profile;",
        f"    profile.layout_family = LayoutFamily::{FAMILY_ENUMS[profile['family']]};",
        f"    profile.density = Density::{DENSITY_ENUMS[profile['density']]};",
        f"    profile.memory_tier = MemoryTier::{MEMORY_ENUMS[profile['memoryTier']]};",
        f"    profile.base_touch_target = {profile['baseTouchTarget']};",
        "    FrameRequest request;",
        f"    request.screen_width = {viewport['width']};",
        f"    request.screen_height = {viewport['height']};",
        f"    request.panel_left = {insets['left']};",
        f"    request.panel_top = {insets['top']};",
        f"    request.panel_right = {insets['right']};",
        f"    request.panel_bottom = {insets['bottom']};",
        "    const FrameLayout layout = calculate_frame(profile, request);",
        f"    assert(layout.panel_x == {panel['x']});",
        f"    assert(layout.panel_y == {panel['y']});",
        f"    assert(layout.panel_width == {panel['width']});",
        f"    assert(layout.panel_height == {panel['height']});",
        f"    assert(layout.short_side == {expected['shortSide']});",
        f"    assert(layout.inset == {expected['inset']});",
        f"    assert(layout.back_size == {back['size']});",
        f"    assert(layout.back_inset_x == {back['x']});",
        f"    assert(layout.back_inset_y == {back['y']});",
        f"    assert(layout.button_size == {expected['buttonSize']});",
        f"    assert(layout.arc_size == {arc['size']});",
        f"    assert(layout.arc_stroke == {arc['stroke']});",
        f"    assert(layout.arc_center_x == {arc['centerX']});",
        f"    assert(layout.controls_center_y == {expected['controlsCenterY']});",
        "    assert(layout.back_size >= profile.base_touch_target);",
        "    assert(layout.panel_width > layout.inset * 2);",
        "    assert(layout.panel_height > layout.inset * 2);",
        "    TabRequest tab_request;",
        "    tab_request.tab_count = 5;",
        "    tab_request.show_tab_bar = true;",
        "    const TabLayout tabs = calculate_tabs(profile, layout, tab_request);",
        f"    assert(tabs.tab_size == {tabs['size']});",
        f"    assert(tabs.selected_tab_size == {tabs['selectedSize']});",
        f"    assert(tabs.tab_gap == {tabs['gap']});",
        f"    assert(tabs.frame_width == {tabs['frameWidth']});",
        f"    assert(tabs.frame_height == {tabs['frameHeight']});",
        f"    assert(tabs.safe_left == {tabs['safeLeft']});",
        f"    assert(tabs.row_left == {tabs['rowLeft']});",
        f"    assert(tabs.content_gap == {tabs['contentGap']});",
        f"    assert(tab_icon_zoom(profile) == {tabs['iconZoom']});",
        "    assert(tabs.tab_size > 0);",
        "    assert(tabs.row_left >= 0);",
        "    assert(tabs.row_left + tabs.frame_width <= layout.panel_width);",
        "    ContentRequest content_request;",
        "    content_request.show_tab_bar = true;",
        "    content_request.tab_frame_height = tabs.frame_height;",
        "    content_request.tab_content_gap = tabs.content_gap;",
        "    content_request.minimum_height = 160;",
        "    const ContentLayout content = calculate_content(layout, content_request);",
        f"    assert(content.top == {content['top']});",
        f"    assert(content.bottom == {content['bottom']});",
        f"    assert(content.width == {content['width']});",
        f"    assert(content.height == {content['height']});",
        f"    assert(content.center_y == {content['centerY']});",
        "    assert(content.top >= layout.inset);",
        "    assert(content.bottom <= layout.panel_height);",
        "    // All Controls can expose four media tabs when grouping is supported.",
        "    TabRequest speaker_tabs_request;",
        "    speaker_tabs_request.tab_count = 4;",
        "    speaker_tabs_request.show_tab_bar = true;",
        "    const TabLayout speaker_tabs = calculate_tabs(profile, layout, speaker_tabs_request);",
        "    assert(speaker_tabs.show_tab_bar);",
        "    assert(speaker_tabs.tab_count == 4);",
        "    assert(speaker_tabs.row_left >= 0);",
        "    assert(speaker_tabs.row_left + speaker_tabs.frame_width <= layout.panel_width);",
        "    ContentRequest speaker_content_request;",
        "    speaker_content_request.show_tab_bar = true;",
        "    speaker_content_request.tab_frame_height = speaker_tabs.frame_height;",
        "    speaker_content_request.tab_content_gap = speaker_tabs.content_gap;",
        "    speaker_content_request.minimum_height = 180;",
        "    speaker_content_request.fallback_height = layout.panel_height / 2;",
        "    const ContentLayout speaker_content = calculate_content(layout, speaker_content_request);",
        "    assert(speaker_content.width > 0);",
        "    assert(speaker_content.height >= 180);",
        "    assert(speaker_content.top >= layout.inset);",
        "    // Standalone Speaker Group uses the same content without a tab row.",
        "    TabRequest standalone_tabs_request;",
        "    standalone_tabs_request.tab_count = 0;",
        "    standalone_tabs_request.show_tab_bar = false;",
        "    const TabLayout standalone_tabs = calculate_tabs(profile, layout, standalone_tabs_request);",
        "    assert(!standalone_tabs.show_tab_bar);",
        "    assert(standalone_tabs.frame_height == 0);",
        "    ContentRequest standalone_content_request;",
        "    standalone_content_request.show_tab_bar = false;",
        "    standalone_content_request.top_without_tabs = layout.inset * 2;",
        "    standalone_content_request.minimum_height = 180;",
        "    standalone_content_request.fallback_height = layout.panel_height / 2;",
        "    const ContentLayout standalone_content = calculate_content(layout, standalone_content_request);",
        "    assert(standalone_content.top == layout.inset * 2);",
        "    assert(standalone_content.width == speaker_content.width);",
        "    assert(standalone_content.height >= 180);",
        f"    (void) \"{prefix}\";",
        "  }",
    ]


def compile_layout_contract(fixtures: dict) -> None:
    compiler = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if compiler is None:
        raise RuntimeError("a C++ compiler is required for firmware modal layout checks")

    source = [
        "#include <cassert>",
        f'#include "{HEADER}"',
        "using namespace espcontrol::modal;",
        "int main() {",
    ]
    for index, entry in enumerate(fixtures["layouts"]):
        source.extend(cpp_assertions(entry, index))

    source.extend([
        "  DeviceProfile compact_portrait;",
        "  compact_portrait.layout_family = LayoutFamily::COMPACT_PORTRAIT;",
        "  FrameRequest rotated;",
        "  rotated.screen_width = 800;",
        "  rotated.screen_height = 480;",
        "  const FrameLayout rotated_layout = calculate_frame(compact_portrait, rotated);",
        "  assert(rotated_layout.short_side == 480);",
        "  assert(rotated_layout.back_inset_x == 30);",
        "  assert(rotated_layout.back_inset_y == 30);",
        "  return 0;",
        "}",
    ])

    with tempfile.TemporaryDirectory(prefix="espcontrol-modal-layout-") as tmp:
        tmp_path = Path(tmp)
        source_path = tmp_path / "modal_layout_test.cpp"
        binary_path = tmp_path / "modal_layout_test"
        source_path.write_text("\n".join(source) + "\n", encoding="utf-8")
        subprocess.run(
            [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", str(source_path), "-o", str(binary_path)],
            check=True,
        )
        subprocess.run([str(binary_path)], check=True)


def main() -> int:
    fixtures = json.loads(FIXTURES.read_text(encoding="utf-8"))
    assert fixtures.get("version") == 1, "unsupported modal layout fixture version"
    profiles = load_device_profiles()
    assert_profile_contract(fixtures, profiles)
    compile_layout_contract(fixtures)
    print("Firmware modal layout geometry checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
