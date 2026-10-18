#!/usr/bin/env python3
"""Extract legacy alarm pending-action acknowledgement functions."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
r = Path(__file__).resolve().parents[2]
s = (r / 'components/espcontrol/button_grid_alarm.h').read_text()
ref = ''
for name in ['alarm_action_achieved_state', 'alarm_normalized_arm_mode', 'alarm_effective_state', 'alarm_action_state_matches', 'alarm_action_state_progressed', 'alarm_clear_pending_action', 'alarm_clear_pending_action_if_progressed', 'alarm_pending_action_timer_cb', 'alarm_start_pending_action']:
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', s, re.M | re.S)
    assert len(functions) == 1, name
    ref += functions[0] + '\n'
t = Path(__file__).with_name('page_alarm_ack_runtime_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-alarm-ack', ref))
