#!/usr/bin/env python3
"""Generate alarm UI lifetime tests with production PIN/defer handlers."""
import argparse,re
from pathlib import Path
from generate_climate_update_test import definition
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[2];s=(r/'components/espcontrol/button_grid_alarm.h').read_text()
t=Path(__file__).with_name('page_alarm_modal_test.cpp.in').read_text()
records=[]
for name in ['AlarmCardCtx','AlarmActionCtx','AlarmPinModalUi','AlarmControlModalUi','AlarmDeferredAction']:
    records.append(re.search(r'^struct '+name+r'[^\n]*\{\n.*?^\};',s,re.M|re.S)[0])
t=t.replace('// @contexts','\n'.join(records))
for name in re.findall(r'// @production-function (\w+)',t):t=t.replace('// @production-function '+name,definition(s,name))
a.output.write_text('// Generated from production alarm helpers.\n'+t)
