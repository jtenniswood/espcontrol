#!/usr/bin/env python3
"""Extract legacy clock layout branches for reusable-renderer parity."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
r = Path(__file__).resolve().parents[2]
source = (r / 'components/espcontrol/button_grid_date_time_driver.h').read_text()
reference = ''
for name in ['date_time_driver_large_layout', 'date_time_driver_refresh_layout']:
    matches = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
    assert len(matches) == 1, name
    reference += matches[0] + '\n'
t = Path(__file__).with_name('clock_card_layout_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-clock-layout', reference))
