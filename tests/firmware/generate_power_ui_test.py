"""Generate production Power UI checks for host doubles or native LVGL surfaces."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--native-surface', action='store_true')
args = parser.parse_args()
headers = args.root / 'components/espcontrol'
layout_source = (headers / 'button_grid_layout.h').read_text(encoding='utf-8')
layout = layout_source[layout_source.index('inline void configure_button_label_wrap('):]
clock = (headers / 'clock_bar.h').read_text(encoding='utf-8')
formatter = clock[clock.index('inline void format_fixed_decimal('):clock.index('inline void format_fixed_decimal_unit(')]
navigation = (headers / 'button_grid_navigation_driver.h').read_text(encoding='utf-8')
def function(name, source=navigation):
    match = re.search(r'^inline [^\n]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise ValueError('Missing production function: ' + name)
    start = match.start()
    brace = match.end() - 1
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
nav_helpers = '\n'.join(function(name) for name in (
    'navigation_driver_matches', 'navigation_driver_refresh_layout', 'navigation_driver_handle_main_click'))
contrast = '\n'.join(function(name, layout_source) for name in (
    'apply_button_colors', 'apply_card_descendant_text_color', 'sync_card_checked_text_color',
    'card_text_contrast_event_cb', 'bind_card_text_contrast_events'))
sensor = '\n'.join(function(name, (headers / 'button_grid_subpages.h').read_text(encoding='utf-8')) for name in ('create_grid_card_button', 'create_card_sensor_container', 'create_dynamic_card_slot', 'setup_subpage_back_card'))
theme_source = (headers / 'theme_runtime_ui.h').read_text(encoding='utf-8')
theme_helpers = '\n'.join(function(name, theme_source) for name in ('theme_grid_state_text_color', 'theme_apply_grid_button'))

display_source = (headers / 'button_grid_display.h').read_text(encoding='utf-8')
grid_source = (headers / 'button_grid_grid.h').read_text(encoding='utf-8')
font_roles = re.search(r'struct DisplayFontRoles \{[\s\S]*?^\};', display_source, re.M)[0]
grid_config = re.search(r'struct GridConfig \{[\s\S]*?^\};', grid_source, re.M)[0]
small_font = function('display_small_text_font', display_source)
sensor += '\n' + function('apply_standard_sensor_number_style', grid_source)
sensor += '\nnamespace espcontrol::cards {\n' + function('sensor_driver_apply_background',
    (headers / 'button_grid_sensor_driver.h').read_text(encoding='utf-8')) + '\n}'
modal_source = (headers / 'button_grid_modal.h').read_text(encoding='utf-8')
modal_shell = re.search(r'struct ControlModalShell \{[\s\S]*?^\};', modal_source, re.M)[0]
if args.native_surface:
    power = (headers / 'button_grid_power.h').read_text(encoding='utf-8')
    helpers = '\n'.join((
        function('display_correct_color', display_source),
        function('create_grid_card_button', (headers / 'button_grid_subpages.h').read_text(encoding='utf-8')),
        'namespace espcontrol::cards {\n' + function('sensor_driver_apply_background',
            (headers / 'button_grid_sensor_driver.h').read_text(encoding='utf-8')) + '\n}',
        function('power_flow_style_status', power),
        function('power_flow_layout_status', power),
        function('power_flow_apply_surfaces', power),
    ))
    template = Path(__file__).parent.parent / 'native_power_surface/power_surface_render_test.cpp.in'
    args.output.write_text(template.read_text(encoding='utf-8').replace('// @surface_helpers', helpers), encoding='utf-8')
    raise SystemExit(0)

template = Path(__file__).with_name('power_ui_test.cpp.in').read_text(encoding='utf-8')
args.output.write_text(template.replace('// @control_modal_shell', modal_shell).replace('// @display_font_roles', font_roles).replace('// @grid_config', grid_config).replace('// @small_text_helper', small_font).replace('// @production_helpers', 'inline void bind_card_text_contrast_events(lv_obj_t *);\n' + formatter + contrast + layout + sensor + theme_helpers).replace('// @navigation_helpers', 'namespace espcontrol::cards {\n' + nav_helpers + '\n}'), encoding='utf-8')
