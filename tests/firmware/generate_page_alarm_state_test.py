#!/usr/bin/env python3
"""Extract production alarm report handlers and status-label history for owner parity."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
r = Path(__file__).resolve().parents[2]
def function(file, name):
    s = (r / 'components/espcontrol' / file).read_text()
    matches = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', s, re.M | re.S)
    assert len(matches) == 1, name
    return matches[0] + '\n'
s = (r / 'components/espcontrol/button_grid_access_cards.h').read_text()
reference = re.search(r'^struct TransientStatusLabel \{.*?^\};', s, re.M | re.S)[0] + '\n'
for name in ['transient_status_label_revert_cb', 'transient_status_label_show_if_changed']:
    reference += function('button_grid_access_cards.h', name)
for name in ['alarm_state_label', 'alarm_state_is_armed', 'alarm_state_is_active', 'alarm_state_is_delay', 'alarm_state_releases_label', 'alarm_action_achieved_state', 'alarm_normalized_arm_mode', 'alarm_effective_state', 'alarm_action_state_matches', 'alarm_action_state_progressed', 'alarm_parse_delay_seconds', 'alarm_remaining_delay_seconds', 'alarm_delay_progress_percent', 'alarm_apply_home_state', 'alarm_apply_home_arm_mode', 'alarm_apply_home_arm_delay', 'alarm_apply_action_availability', 'alarm_apply_action_state', 'alarm_apply_action_arm_mode']:
    reference += function('button_grid_alarm.h', name)
t = Path(__file__).with_name('page_alarm_state_runtime_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-alarm-state', reference))
