#!/usr/bin/env python3
"""Exercise the actual pooled adapter with real subscriptions and native LVGL.

Only the existing visual panel/action transport boundary is replaced. Context,
subscription callbacks, timer/command rules, teardown, ownership and gestures
are the production definitions.
"""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'components/espcontrol/button_grid_climate.h').read_text()
text=Path(__file__).with_name('page_climate_modal_test.cpp.in').read_text()
context=re.search(r'^struct ClimateControlCtx \{\n.*?^\};',source,re.MULTILINE|re.DOTALL)
constants=re.findall(r'^constexpr int CLIMATE_(?:DEFAULT_\w+|WHOLE_NUMBER_STEP_TENTHS) = [^;]+;',source,re.MULTILINE)
text=text.replace('// @production-context','\n'.join(constants)+'\nconstexpr uint32_t CLIMATE_TEMP_DEBOUNCE_MS=450;\n'+context[0])
for name in re.findall(r'// @production-function (\w+)',text):
    text=re.sub(r'^// @production-function '+re.escape(name)+r'$',lambda _:definition(source,name),text,flags=re.MULTILINE)
driver=(root/'components/espcontrol/button_grid_climate_control_driver.h').read_text()
environment=re.search(r'^struct ClimateControlDriverEnvironment \{.*?^\};',driver,re.MULTILINE|re.DOTALL)
text=text.replace('// @production-environment','namespace espcontrol::cards {\n'+environment[0]+'\n}')
args.output.write_text('// Generated from production modal helpers.\n'+text)
