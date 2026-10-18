#!/usr/bin/env python3
"""Extract current alarm visibility/PIN policy for deferred-command parity."""
import argparse
import re
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args()
r=Path(__file__).resolve().parents[2]
s=(r/'components/espcontrol/button_grid_alarm.h').read_text()
reference=''
for name in ['alarm_pin_arm_required','alarm_pin_disarm_required','alarm_action_valid','alarm_action_mode_count','alarm_action_mode_at','alarm_action_default_visible','alarm_action_visible','alarm_visible_action_modes','alarm_action_requires_pin']:
    functions=re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}',s,re.M|re.S)
    assert len(functions)==1,name
    reference+=functions[0]+'\n'
t=Path(__file__).with_name('page_alarm_commands_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-alarm-command-policy',reference))
