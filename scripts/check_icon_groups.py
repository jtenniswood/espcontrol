#!/usr/bin/env python3
"""Verify icon gallery groups match the Product Model icon source.

Usage:
    python scripts/check_icon_groups.py       # exit 1 if any icon grouping is stale
"""
import json
import re
import sys
from pathlib import Path

from product_model_v2 import source_path

ROOT = Path(__file__).resolve().parent.parent
ICONS_JSON = source_path("icons")
GALLERY_VUE = ROOT / "docs" / ".vitepress" / "theme" / "components" / "IconGallery.vue"


def main():
    with open(ICONS_JSON) as f:
        icon_names = {"Auto"} | {icon["name"] for icon in json.load(f)["icons"]}

    vue_text = GALLERY_VUE.read_text()

    match = re.search(
        r"const ICON_GROUPS\s*=\s*\{(.*?)\n\}",
        vue_text,
        re.DOTALL,
    )
    if not match:
        print("ERROR: Could not find ICON_GROUPS in IconGallery.vue")
        return 1

    assignments = re.findall(r"'([^']+)':\s*'([^']+)'", match.group(1))
    grouped_names = {name for name, _group in assignments}

    order_match = re.search(r"const GROUP_ORDER\s*=\s*\[(.*?)\]", vue_text, re.DOTALL)
    if not order_match:
        print("ERROR: Could not find GROUP_ORDER in IconGallery.vue")
        return 1

    visible_groups = set(re.findall(r"'([^']+)'", order_match.group(1)))
    hidden = [(name, group) for name, group in assignments if group not in visible_groups]
    if hidden:
        print("ERROR: Icons assigned to groups missing from GROUP_ORDER:")
        for name, group in hidden:
            print(f"  {name}: {group}")
        return 1

    ungrouped = sorted(icon_names - grouped_names)
    if ungrouped:
        print(f"ERROR: {len(ungrouped)} icon(s) missing from ICON_GROUPS in IconGallery.vue:")
        for name in ungrouped:
            print(f"  {name}")
        print("\nAdd a group assignment for each icon in docs/.vitepress/theme/components/IconGallery.vue")
        return 1

    stale = sorted(grouped_names - icon_names)
    if stale:
        print(f"ERROR: {len(stale)} name(s) in ICON_GROUPS are not available icon options:")
        for name in stale:
            print(f"  {name}")
        print("\nRemove stale group assignments from docs/.vitepress/theme/components/IconGallery.vue")
        return 1

    print(f"All {len(icon_names)} icons have current group assignments.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
