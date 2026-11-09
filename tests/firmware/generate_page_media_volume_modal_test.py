#!/usr/bin/env python3
"""Exercise production volume UI, commands and snapshot policy with native LVGL."""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
sliders=(root/'components/espcontrol/button_grid_sliders.h').read_text()
media=(root/'components/espcontrol/button_grid_media.h').read_text()
text=Path(__file__).with_name('page_media_volume_modal_test.cpp.in').read_text()
contexts=[]
for name in ('MediaVolumeCtx','MediaVolumeModalUi'):
 contexts.append(re.search(r'^struct '+name+r' \{\n.*?^\};',sliders,re.MULTILINE|re.DOTALL)[0])
text=text.replace('// @contexts','\n'.join(contexts))
text=text.replace('// @snapshot',re.search(r'^struct MediaVolumeStateSnapshot \{\n.*?^\};',media,re.MULTILINE|re.DOTALL)[0])
for name in re.findall(r'// @production-function (\w+)',text):
 source=media if name in ('media_volume_apply_snapshot','create_media_volume_context') else sliders
 text=text.replace('// @production-function '+name,definition(source,name))
args.output.write_text('// Generated from production volume helpers.\n'+text)
