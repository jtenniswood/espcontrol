#!/usr/bin/env python3
"""Generate a reviewable SVG reference for device modal layout profiles."""

from __future__ import annotations

import argparse
import html
import json
from pathlib import Path

from device_profiles import ROOT


FIXTURES = ROOT / "common" / "config" / "modal_layout_geometry_fixtures.json"
OUTPUT = ROOT / "dev-docs" / "modal-layout-profiles.svg"

CARD_WIDTH = 292
CARD_HEIGHT = 292
VIEWPORT_WIDTH = 250
VIEWPORT_HEIGHT = 190
MARGIN = 24


def svg_text(x: float, y: float, value: str, css_class: str, anchor: str = "start") -> str:
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" class="{css_class}" '
        f'text-anchor="{anchor}">{html.escape(value)}</text>'
    )


def render_device(entry: dict, index: int) -> list[str]:
    viewport = entry["viewport"]
    expected = entry["expected"]
    panel = expected["panel"]
    back = expected["back"]
    tabs = expected["tabs"]
    content = expected["content"]
    profile = entry["profile"]

    card_x = MARGIN + index * CARD_WIDTH
    card_y = 70
    scale = min(VIEWPORT_WIDTH / viewport["width"], VIEWPORT_HEIGHT / viewport["height"])
    screen_w = viewport["width"] * scale
    screen_h = viewport["height"] * scale
    screen_x = card_x + (VIEWPORT_WIDTH - screen_w) / 2
    screen_y = card_y + (VIEWPORT_HEIGHT - screen_h) / 2

    panel_x = screen_x + panel["x"] * scale
    panel_y = screen_y + panel["y"] * scale
    panel_w = panel["width"] * scale
    panel_h = panel["height"] * scale
    tab_x = panel_x + tabs["rowLeft"] * scale
    tab_y = panel_y + (expected["inset"] + 2) * scale
    tab_w = tabs["frameWidth"] * scale
    tab_h = tabs["frameHeight"] * scale
    total_tabs_w = (tabs["size"] * 5 + tabs["gap"] * 4) * scale
    first_tab_x = tab_x + (tab_w - total_tabs_w) / 2

    lines = [
        f'<g id="{html.escape(entry["slug"])}">',
        f'<rect x="{screen_x:.1f}" y="{screen_y:.1f}" width="{screen_w:.1f}" '
        f'height="{screen_h:.1f}" rx="8" class="screen"/>',
        f'<rect x="{panel_x:.1f}" y="{panel_y:.1f}" width="{panel_w:.1f}" '
        f'height="{panel_h:.1f}" rx="6" class="panel"/>',
        f'<rect x="{tab_x:.1f}" y="{tab_y:.1f}" width="{tab_w:.1f}" '
        f'height="{tab_h:.1f}" rx="{tab_h / 2:.1f}" class="tab-frame"/>',
    ]
    for tab_index in range(5):
        tab_size = tabs["selectedSize"] if tab_index == 0 else tabs["size"]
        base_x = first_tab_x + tab_index * (tabs["size"] + tabs["gap"]) * scale
        tab_center_x = base_x + tabs["size"] * scale / 2
        lines.append(
            f'<circle cx="{tab_center_x:.1f}" cy="{tab_y + tab_h / 2:.1f}" '
            f'r="{tab_size * scale / 2:.1f}" class="tab{(" selected" if tab_index == 0 else "")}"/>'
        )
    lines.extend(
        [
            f'<rect x="{panel_x + expected["inset"] * scale:.1f}" '
            f'y="{panel_y + content["top"] * scale:.1f}" '
            f'width="{content["width"] * scale:.1f}" height="{content["height"] * scale:.1f}" '
            f'rx="4" class="content"/>',
            f'<circle cx="{panel_x + (back["x"] + back["size"] / 2) * scale:.1f}" '
            f'cy="{panel_y + (back["y"] + back["size"] / 2) * scale:.1f}" '
            f'r="{back["size"] * scale / 2:.1f}" class="back"/>',
            svg_text(card_x + VIEWPORT_WIDTH / 2, card_y + VIEWPORT_HEIGHT + 28,
                     entry["slug"].replace("guition-esp32-", ""), "device", "middle"),
            svg_text(card_x + VIEWPORT_WIDTH / 2, card_y + VIEWPORT_HEIGHT + 48,
                     f'{profile["family"]} · {profile["density"]}', "profile", "middle"),
            svg_text(card_x + VIEWPORT_WIDTH / 2, card_y + VIEWPORT_HEIGHT + 66,
                     f'{viewport["width"]}×{viewport["height"]} · {profile["memoryTier"]} memory',
                     "meta", "middle"),
            "</g>",
        ]
    )
    return lines


def render(fixtures: dict) -> str:
    layouts = fixtures["layouts"]
    width = MARGIN * 2 + CARD_WIDTH * len(layouts)
    height = 370
    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" role="img" aria-labelledby="title desc">',
        '<title id="title">EspControl modal layout profiles</title>',
        '<desc id="desc">Generated reference showing the shared modal shell, tabs, content region, and back button on each display family.</desc>',
        "<style>",
        "text { font-family: Inter, -apple-system, BlinkMacSystemFont, sans-serif; }",
        ".background { fill: #11151d; } .screen { fill: #090c12; stroke: #344054; stroke-width: 2; }",
        ".panel { fill: #222936; stroke: #475467; stroke-width: 1.5; }",
        ".tab-frame { fill: #344054; } .tab { fill: #667085; } .tab.selected { fill: #5e8bff; }",
        ".content { fill: #161b24; stroke: #98a2b3; stroke-width: 1.5; stroke-dasharray: 5 4; }",
        ".back { fill: #f2f4f7; stroke: #98a2b3; stroke-width: 1; }",
        ".title { fill: #f9fafb; font-size: 24px; font-weight: 700; }",
        ".subtitle { fill: #98a2b3; font-size: 13px; } .device { fill: #f2f4f7; font-size: 13px; font-weight: 600; }",
        ".profile { fill: #b2ccff; font-size: 12px; } .meta { fill: #98a2b3; font-size: 11px; }",
        "</style>",
        f'<rect width="{width}" height="{height}" class="background"/>',
        svg_text(MARGIN, 31, "Device-aware modal layout reference", "title"),
        svg_text(MARGIN, 52, "Generated from the geometry contract; dashed areas are modal content regions.", "subtitle"),
    ]
    for index, entry in enumerate(layouts):
        lines.extend(render_device(entry, index))
    lines.append("</svg>")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail when the committed SVG is stale")
    args = parser.parse_args()
    fixtures = json.loads(FIXTURES.read_text(encoding="utf-8"))
    output = render(fixtures)
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != output:
            print(f"{OUTPUT.relative_to(ROOT)} is stale; regenerate it.")
            return 1
        print("Modal layout visual reference is up to date.")
        return 0
    OUTPUT.write_text(output, encoding="utf-8")
    print(f"Generated {OUTPUT.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
