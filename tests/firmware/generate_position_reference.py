#!/usr/bin/env python3
"""Use production slider setup/geometry as a native comparison reference."""
import argparse
from pathlib import Path
from generate_climate_update_test import definition
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'components/espcontrol/button_grid_sliders.h').read_text()
text='#pragma once\nnamespace legacy_position_reference {\ninline void theme_set_content_background(lv_obj_t *){}\n'
for name in ['setup_slider_widget','slider_fit_to_button','slider_update_fill']:
    text+=definition(source,name)+'\n'
text+='}\n'
args.output.write_text(text)
