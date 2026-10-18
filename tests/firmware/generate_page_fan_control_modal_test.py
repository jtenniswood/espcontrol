#!/usr/bin/env python3
"""Generate native fan-panel ownership tests using production context and handlers."""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
fan = (root / 'components/espcontrol/button_grid_fan.h').read_text()
text = Path(__file__).with_name('page_fan_control_modal_test.cpp.in').read_text()
records = []
for kind, name in [('struct', 'FanCardCtx'), ('enum class', 'FanControlTab'),
                   ('struct', 'FanControlVisibleTabs'), ('struct', 'FanControlPresetClick'),
                   ('struct', 'FanControlModalUi')]:
    records.append(re.search(r'^' + kind + ' ' + name + r'[^\n]*\{\n.*?^\};', fan,
                             re.MULTILINE | re.DOTALL)[0])
text = text.replace('// @contexts', '\n'.join(records))
text = text.replace('// @feature-policy', fan[fan.index('constexpr int FAN_FEATURE_SET_SPEED'):fan.index('inline lv_obj_t *fan_control_tab_button')])
for name in re.findall(r'// @production-function (\w+)', text):
    text = text.replace('// @production-function ' + name, definition(fan, name))
args.output.write_text('// Generated from production fan panel helpers.\n' + text)
