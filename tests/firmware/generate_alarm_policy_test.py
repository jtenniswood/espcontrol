#!/usr/bin/env python3
"""Extract current alarm policy and delay arithmetic for bounded runtime parity."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
r = Path(__file__).resolve().parents[2]
s = (r / 'components/espcontrol/button_grid_alarm.h').read_text()
ref = ''
for name in ['alarm_state_label', 'alarm_state_is_armed', 'alarm_state_is_active', 'alarm_state_is_delay', 'alarm_action_achieved_state', 'alarm_normalized_arm_mode', 'alarm_effective_state', 'alarm_state_icon', 'alarm_action_state_matches', 'alarm_action_state_progressed', 'alarm_state_releases_label', 'alarm_parse_delay_seconds', 'alarm_remaining_delay_seconds', 'alarm_delay_progress_percent']:
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', s, re.M | re.S)
    assert len(functions) == 1, name
    ref += functions[0] + '\n'
t = Path(__file__).with_name('page_alarm_policy_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-alarm-policy', ref))
