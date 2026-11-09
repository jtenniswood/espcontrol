#!/usr/bin/env python3
"""Use production modal geometry/text buttons in the demanded confirmation test."""
import argparse
from pathlib import Path
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_modal.h').read_text()

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
legacy = (root / 'components/espcontrol/button_grid_confirm.h').read_text()
start = legacy.index('  lv_obj_update_layout(ui.message_lbl);')
end = legacy.index('  lv_obj_add_event_cb(ui.no_btn', start)
reference = 'void arrange_reference(lv_obj_t* panel, lv_obj_t* message, lv_obj_t* no, lv_obj_t* yes,\n                       ControlModalLayout& layout, int button_gap, int message_button_gap) {\n  struct {lv_obj_t *panel,*message_lbl,*no_btn,*confirm_btn;} ui{panel,message,no,yes};\n' + legacy[start:end] + '}\n'
template = Path(__file__).with_name('page_confirmation_modal_test.cpp.in').read_text()
args.output.write_text('// Generated common modal helpers from production.\n' + template.replace('// @reference-arrange', reference).replace(
    '// @common-structs', structs).replace('// @common-geometry', geometry).replace(
    '// @text-button-layout', definition(source, 'control_modal_layout_text_button')).replace(
    '// @text-button-factory', definition(source, 'control_modal_create_text_button')))
