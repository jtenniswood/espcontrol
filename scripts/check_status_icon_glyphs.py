#!/usr/bin/env python3
"""Verify built-in status glyphs and the editable clock bar icon catalogue.

ESPHome subsets LVGL fonts to their configured glyph lists. Missing symbols
silently render blank on-device. Check the built-in indicator subset, and
ensure each panel's clock bar font includes the full sensor icon catalogue.

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
    full_glyphs = codepoints_in((ROOT / "common/assets/icon_glyphs.yaml").read_text())
    # Clock bar sensor icons share the searchable card catalogue. Their font must
    # include that whole catalogue as well as the existing built-in indicators.
    if glyph_codepoints - full_glyphs:
        print("ERROR: shared icon font is missing clock bar status glyphs:", sorted(glyph_codepoints - full_glyphs))
        return 1
    for path in sorted((ROOT / "devices").glob("*/device/fonts.yaml")):
        text = path.read_text()
        font_id = "font_icon_clock_bar" if "id: font_icon_clock_bar\n" in text else "font_icon_status"
        block = re.search(rf"id: {font_id}\n\s*size: \d+\n\s*bpp: \d+\n\s*glyphs: !include (.+)", text)
        if not block or not block.group(1).strip().endswith("common/assets/icon_glyphs.yaml"):
            print(f"ERROR: {path.relative_to(ROOT)}: {font_id} must include the full sensor icon catalogue")
            return 1

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
