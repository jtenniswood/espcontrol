"""Compile production profile limits and grid foundations for every device."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from device_profiles import load_device_profiles
from device_ui_limits import profile_flag, render_header


# Pin the existing product capacities independently of the generator. Increasing
# these is a separate feature, not a side effect of the architecture migration.
EXPECTED = {
    "guition-esp32-p4-jc1060p470": (15, 6, 8),
    "guition-esp32-p4-jc1060p470-v2": (15, 6, 8),
    "guition-esp32-p4-jc4880p443": (6, 6, 8),
    "guition-esp32-p4-jc4880p443-v3": (6, 6, 8),
    "guition-esp32-p4-jc8012p4a1": (20, 6, 8),
    "guition-esp32-p4-jc8012p4a1-v2": (20, 6, 8),
    "guition-esp32-p4-jc8012p4a1-v3": (20, 6, 8),
    "esp32-p4-86": (9, 6, 8),
    "guition-esp32-s3-4848s040": (9, 2, 4),
}

HARNESS = r'''
#include <cassert>
#include <cstring>
#include "button_grid_limits.h"

int main() {
  constexpr auto limits = espcontrol::DEVICE_UI_LIMITS;
  static_assert(limits.slot_count == EXPECTED_SLOTS);
  static_assert(MAX_GRID_SLOTS == EXPECTED_SLOTS);
  static_assert(MAX_SUBPAGE_ITEMS >= limits.max_child_cards());
  static_assert(limits.grid_columns * limits.grid_rows == EXPECTED_SLOTS);
  static_assert(limits.max_first_level_subpages == EXPECTED_SLOTS);
  static_assert(limits.cards_per_subpage == EXPECTED_SLOTS - 1);
  static_assert(limits.max_cards() == EXPECTED_SLOTS * EXPECTED_SLOTS);
  static_assert(limits.max_navigation_depth == 1);
  static_assert(limits.image_slot_count == EXPECTED_IMAGES);
  static_assert(limits.legacy_subpage_chunks == EXPECTED_CHUNKS);
  static_assert(limits.legacy_text_bytes == 255);
  static_assert(limits.legacy_subpage_byte_ceiling() == EXPECTED_CHUNKS * 255);
  assert(std::strcmp(limits.device_profile, EXPECTED_SLUG) == 0);
  assert(limits.supports_card_type(""));
  assert(limits.supports_card_type("sensor", true));
  assert(limits.supports_card_type("subpage"));
  assert(!limits.supports_card_type("subpage", true));
  assert(!limits.supports_card_type("unknown"));
  assert(!limits.supports_card_type(nullptr));
  assert(limits.supports_card_type("image"));
}
'''


def main():
    profiles = load_device_profiles()
    assert set(profiles) == set(EXPECTED)
    header = ROOT / "components/espcontrol/device_ui_limits_generated.h"
    assert header.read_text() == render_header(), "generated limits must be current"
    compiler = os.environ.get("CXX", "c++")
    with tempfile.TemporaryDirectory(prefix="device-ui-limits-") as temporary:
        directory = Path(temporary)
        harness = directory / "limits.cpp"
        harness.write_text(HARNESS)
        common = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                  "-I", str(ROOT / "components/espcontrol"),
                  "-I", str(ROOT / "tests/firmware/stubs")]
        for slug, (slots, images, chunks) in EXPECTED.items():
            package = (ROOT / "devices" / slug / "packages.yaml").read_text()
            flag = profile_flag(slug)
            assert package.count(f'-D{flag}"') == 1, slug
            device = (ROOT / "devices" / slug / "device/device.yaml").read_text()
            assert "ESPCONTROL_MAX_GRID_SLOTS=" not in device, slug
            output = directory / "test"
            definitions = [f"-D{flag}", f"-DEXPECTED_SLOTS={slots}",
                           f"-DEXPECTED_IMAGES={images}", f"-DEXPECTED_CHUNKS={chunks}",
                           f'-DEXPECTED_SLUG="{slug}"']
            for source in (harness, ROOT / "tests/firmware/button_grid_foundations_test.cpp"):
                subprocess.run(common + definitions + [str(source), "-o", str(output)], check=True)
                subprocess.run([str(output)], check=True)

        # Conflicting device flags and a stale manual override must fail the
        # build rather than produce a device with inconsistent capacities.
        flags = [profile_flag(slug) for slug in EXPECTED]
        for definitions in ([f"-D{flags[0]}", f"-D{flags[1]}"],
                            [f"-D{flags[0]}", "-DESPCONTROL_MAX_GRID_SLOTS=25"]):
            result = subprocess.run(common + definitions + ["-fsyntax-only", "-x", "c++", "-"],
                                    input='#include "button_grid_limits.h"\n', text=True,
                                    capture_output=True)
            assert result.returncode != 0, "conflicting profile must fail"
    print("Device UI limits and grid foundations passed for all nine profiles.")


if __name__ == "__main__":
    main()
