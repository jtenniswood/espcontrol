#!/usr/bin/env python3
"""Exercise the production clock-bar title and refresh code with LVGL doubles."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--header', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
source = args.header.read_text()
formatters = source[source.index('inline void format_fixed_decimal('):source.index('// ── Clock-bar page visibility')]
labels = source[source.index('inline void format_clock_bar_temperature_single('):source.index('// ── Fixed clock-bar placement')]
modal = args.header.with_name('button_grid_modal.h').read_text()
registry = modal[modal.index('struct ControlModalCardLabel {'):modal.index('inline ControlModalShell control_modal_open_shell(')]
template = Path(__file__).with_name('clock_bar_title_test.cpp.in').read_text()
args.output.write_text(template.replace('// @production', formatters + labels + registry))
