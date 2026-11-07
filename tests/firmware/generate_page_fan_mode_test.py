#!/usr/bin/env python3
"""Generate fan mode parity checks using the actual legacy parsing helpers."""
import argparse
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_fan.h').read_text()
names = ['fan_trim', 'fan_lower', 'fan_token_is_header', 'fan_parse_options', 'fan_option_label', 'fan_preset_active']
# Keep the extraction strategy shared with the existing production-function tests.
import re
functions = []
for name in names:
    matches = re.findall(rf"^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}", source, re.M | re.S)
    assert len(matches) == 1, name
    functions.append(matches[0])
text = Path(__file__).with_name('page_fan_mode_test.cpp.in').read_text()
a.output.write_text(text.replace('// @legacy-fan-parser', '\n'.join(functions)))
