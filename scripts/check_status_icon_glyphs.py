#!/usr/bin/env python3
"""Verify every icon codepoint used in components/espcontrol/*_status.h is
present in the compiled font_icon_status glyph subset.

ESPHome subsets the compiled LVGL font to only the codepoints listed in
common/assets/network_status_glyphs.yaml. A codepoint used in C++ but missing
from that list silently renders as a blank "tofu box" on-device instead of
failing to compile - this check catches that before it ships.

Usage:
    python scripts/check_status_icon_glyphs.py       # exit 1 if any codepoint is missing
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
COMPONENTS_DIR = ROOT / "components" / "espcontrol"
GLYPHS_YAML = ROOT / "common" / "assets" / "network_status_glyphs.yaml"

CODEPOINT_RE = re.compile(r"\\U([0-9A-Fa-f]{8})")


def codepoints_in(text: str) -> set[str]:
    return {match.group(1).upper() for match in CODEPOINT_RE.finditer(text)}


def main():
    glyph_codepoints = codepoints_in(GLYPHS_YAML.read_text())

    missing: dict[str, set[str]] = {}
    for header in sorted(COMPONENTS_DIR.glob("*_status.h")):
        used = codepoints_in(header.read_text())
        gap = used - glyph_codepoints
        if gap:
            missing[header.name] = gap

    if missing:
        print("ERROR: codepoint(s) used in C++ but missing from "
              f"{GLYPHS_YAML.relative_to(ROOT)}:")
        for name, codepoints in missing.items():
            for codepoint in sorted(codepoints):
                print(f'  {name}: "\\U{codepoint}"')
        print("\nAdd the missing codepoint(s) to that file, or the icon will "
              "render as a blank placeholder on-device.")
        return 1

    print(f"All status icon codepoints are present in {GLYPHS_YAML.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
