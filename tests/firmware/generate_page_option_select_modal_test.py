#!/usr/bin/env python3
"""Generate native option-panel tests from production context, parser and callbacks."""
import argparse,re
from pathlib import Path
from generate_climate_update_test import definition
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[2];source=(root/'components/espcontrol/button_grid_option_select.h').read_text();text=Path(__file__).with_name('page_option_select_modal_test.cpp.in').read_text()
context=source[source.index('constexpr int OPTION_SELECT_MAX_OPTIONS'):source.index('inline OptionSelectModalUi')]
text=text.replace('// @contexts',context)
for name in re.findall(r'// @production-function (\w+)',text):text=text.replace('// @production-function '+name,definition(source,name))
a.output.write_text('// Generated option panel ownership tests.\n'+text)
