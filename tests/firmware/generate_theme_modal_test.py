"""Extract the production modal theme adapter for the existing LVGL host test."""

import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--header", required=True, type=Path)
parser.add_argument("--output", required=True, type=Path)
args = parser.parse_args()
source = args.header.read_text(encoding="utf-8")
start = source.index("struct ControlModalThemeTargets {")
end = source.index("struct ControlModalToastShell {", start)
args.output.write_text(
    "// Extracted from button_grid_modal.h; do not edit.\n" + source[start:end],
    encoding="utf-8",
)
