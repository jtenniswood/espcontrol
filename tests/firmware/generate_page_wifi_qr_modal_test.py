#!/usr/bin/env python3
"""Use production common modal geometry and widget helpers in the QR test."""
import argparse
from pathlib import Path
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_modal.h').read_text()
sliders = (root / 'components/espcontrol/button_grid_sliders.h').read_text()

def declaration(name):
    start = source.index('struct ' + name + ' {')
    return source[start:source.index('\n};', start) + 3]

structs = '\n'.join(declaration(name) for name in ('ControlModalLayout', 'ControlModalTabLayout', 'ControlModalShell'))
geometry = '\n'.join(definition(source, name) for name in (
    'control_modal_geometry_frame', 'control_modal_calc_layout', 'control_modal_calc_tab_layout',
    'control_modal_calc_content_layout', 'control_modal_apply_panel_layout',
    'control_modal_apply_back_button_layout', 'control_modal_apply_tab_row',
    'control_modal_create_tab_row', 'control_modal_center_tab_icon', 'control_modal_tab_icon_zoom',
    'control_modal_layout_tab_button', 'control_modal_icon_label', 'control_modal_create_flat_icon_button'))
power = definition(sliders, 'light_control_layout_power')
template = Path(__file__).with_name('page_wifi_qr_modal_test.cpp.in').read_text()
args.output.write_text('// Generated common modal helpers from production.\n' + template.replace('// @common-structs', structs).replace('// @common-geometry', geometry).replace('// @common-power', power))
