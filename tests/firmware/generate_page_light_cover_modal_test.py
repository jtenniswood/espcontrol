#!/usr/bin/env python3
"""Generate real light/cover contexts and subscriptions with a native LVGL panel double."""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'components/espcontrol/button_grid_sliders.h').read_text()
text=Path(__file__).with_name('page_light_cover_modal_test.cpp.in').read_text()
contexts=[]
for name in ('LightControlCtx','CoverControlCtx','LightColorPreset'):
 contexts.append(re.search(r'^struct '+name+r' \{\n.*?^\};',source,re.MULTILINE|re.DOTALL)[0])
text=text.replace('// @contexts','\n'.join(contexts))
for name in re.findall(r'// @production-function (\w+)',text):
 text=re.sub(r'^// @production-function '+re.escape(name)+r'$',lambda _:definition(source,name),text,flags=re.MULTILINE)
args.output.write_text('// Generated from production control helpers.\n'+text)
