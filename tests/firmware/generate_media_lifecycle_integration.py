#!/usr/bin/env python3
"""Compile production lifecycle functions against the host LVGL/HA boundary.

Extract whole definitions without rewriting them. This avoids copying the
ownership/binding logic into a test while leaving rendering and hardware out.
The generated header is build output, never a checked-in firmware dependency.
"""
from pathlib import Path
import argparse
import re

# Ordered by dependencies. Every selected function is compiled unchanged.
GROUPS = (
    ("button_grid_sliders.h", "", (
        "slider_deferred_geometry_refresh_cb", "slider_geometry_refresh_event_cb",
        "slider_detach_runtime", "slider_geometry_delete_event_cb",
        "slider_bind_geometry_refresh",
    )),
    ("button_grid_media.h", "", (
        "media_deferred_position_refresh_cb", "media_schedule_position_refresh",
        "delete_media_now_playing_context", "delete_media_slider_context",
    )),
    ("button_grid_grid.h", "", (
        "grid_runtime_allocations", "grid_delete_media_now_playing_runtime_ptr",
        "grid_delete_media_slider_runtime_ptr",
        "grid_track_media_now_playing_runtime", "grid_track_media_slider_runtime",
        "grid_delete_media_now_playing_with_owner", "grid_delete_media_slider_with_owner",
        "grid_prepare_media_runtime_for_visual_reset", "grid_release_runtime_allocations",
    )),
    ("button_grid_media_driver.h", "espcontrol::cards", (
        "media_driver_matches", "media_driver_track_now_playing",
        "media_driver_track_slider", "media_driver_setup_visual",
        "media_driver_cleanup", "media_driver_bind_data",
    )),
    ("button_grid_grid.h", "", ("grid_release_main_runtime_allocations",)),
)


def definition(text: str, name: str) -> tuple[int, str]:
    match = re.search(rf"^inline\s+[^;{{}}]*\b{re.escape(name)}\s*\([^;{{}}]*\)\s*\{{", text, re.M)
    if not match:
        raise ValueError(f"Missing production definition: {name}")
    # Ignore comments and literals while finding the matching closing brace.
    tokens = re.finditer(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', text[match.end()-1:])
    depth = 0
    for token in tokens:
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                end = match.end() - 1 + token.end()
                return text.count("\n", 0, match.start()) + 1, text[match.start():end]
    raise ValueError(f"Unclosed production definition: {name}")


def generate(root: Path) -> str:
    output = ["// Generated production functions; do not edit."]
    for filename, namespace, names in GROUPS:
        path = root / "components" / "espcontrol" / filename
        source = path.read_text()
        if namespace:
            output.append(f"namespace {namespace} {{")
        for name in names:
            line, code = definition(source, name)
            output.extend((f'#line {line} "{path.as_posix()}"', code))
        if namespace:
            output.append("}")
    return "\n\n".join(output) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(generate(args.root.resolve()))
