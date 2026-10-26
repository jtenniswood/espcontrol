#!/usr/bin/env python3
"""Extract legacy alarm captions and glyphs for shared-view parity."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
r = Path(__file__).resolve().parents[2]
s = (r / 'components/espcontrol/button_grid_alarm.h').read_text()
reference = ''
for name in ['alarm_action_label', 'alarm_normalized_arm_mode', 'alarm_effective_state', 'alarm_state_icon']:
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', s, re.M | re.S)
    assert len(functions) == 1, name
    reference += functions[0] + '\n'
t = Path(__file__).with_name('alarm_card_view_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-alarm-view', reference))
