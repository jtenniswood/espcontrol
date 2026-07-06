#!/usr/bin/env python3
"""Verify icon gallery groups match icons.json.

Usage:
    python scripts/check_icon_groups.py       # exit 1 if any icon grouping is stale
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ICONS_JSON = ROOT / "common" / "assets" / "icons.json"
GALLERY_VUE = ROOT / "docs" / ".vitepress" / "theme" / "components" / "IconGallery.vue"


def main():
    with open(ICONS_JSON) as f:
        icon_names = {icon["name"] for icon in json.load(f)["icons"]}

    vue_text = GALLERY_VUE.read_text()

    match = re.search(
        r"const ICON_GROUPS\s*=\s*\{(.*?)\n\}",
        vue_text,
        re.DOTALL,
    )
    if not match:
        print("ERROR: Could not find ICON_GROUPS in IconGallery.vue")
        return 1

    grouped_names = set(re.findall(r"'([^']+)':\s*'", match.group(1)))

    ungrouped = sorted(icon_names - grouped_names)
    if ungrouped:
        print(f"ERROR: {len(ungrouped)} icon(s) missing from ICON_GROUPS in IconGallery.vue:")
        for name in ungrouped:
            print(f"  {name}")
        print("\nAdd a group assignment for each icon in docs/.vitepress/theme/components/IconGallery.vue")
        return 1

    stale = sorted(grouped_names - icon_names)
    if stale:
        print(f"ERROR: {len(stale)} name(s) in ICON_GROUPS no longer exist in icons.json:")
        for name in stale:
            print(f"  {name}")
        print("\nRemove stale group assignments from docs/.vitepress/theme/components/IconGallery.vue")
        return 1

    print(f"All {len(icon_names)} icons have current group assignments.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
