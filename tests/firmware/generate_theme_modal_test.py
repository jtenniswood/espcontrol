"""Extract the production modal theme adapter for the existing LVGL host test."""

import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--header", required=True, type=Path)
parser.add_argument("--output", required=True, type=Path)
args = parser.parse_args()
source = args.header.read_text(encoding="utf-8")
start = source.index("constexpr uint8_t CONTROL_MODAL_THEME_PRESSED_CAPACITY")
end = source.index("struct ControlModalToastShell {", start)
definitions = [source[start:end]]
for name in ("control_modal_apply_pressed_fill", "control_modal_apply_pressed_fill_color"):
    start = source.index(f"inline void {name}(")
    end = source.index("\n}", start) + 2
    definitions.append(source[start:end])
subscriptions = (args.header.parent / "button_grid_subscriptions.h").read_text(encoding="utf-8")
start = subscriptions.index("inline void apply_sensor_active_color(")
end = subscriptions.index("\n}", start) + 2
definitions.append(subscriptions[start:end])
media = (args.header.parent / "button_grid_media.h").read_text(encoding="utf-8")
start = media.index("inline void media_control_style_playback_mode_button(")
end = media.index("\n}", start) + 2
definitions.append(media[start:end])
availability = (args.header.parent / "card_availability.h").read_text(encoding="utf-8")
start = availability.index("inline void set_card_content_disabled(")
end = availability.index("\n}", start) + 2
definitions.append(availability[start:end])
args.output.write_text(
    "// Extracted from button_grid_modal.h; do not edit.\n" + "\n".join(definitions),
    encoding="utf-8",
)
