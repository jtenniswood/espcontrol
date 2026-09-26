"""Run the production modal resolver and driver hooks against LVGL/HA doubles."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HEADERS = ROOT / 'components/espcontrol'


def function(file, name):
    source = (HEADERS / file).read_text()
    search_from = 0
    while True:
        signature = source.find(name + '(', search_from)
        if signature < 0:
            raise ValueError(f'Could not find definition of {name} in {file}')
        open_paren = source.index('(', signature)
        depth = 1
        close_paren = open_paren + 1
        while depth:
            depth += (source[close_paren] == '(') - (source[close_paren] == ')')
            close_paren += 1
        body = source.find('{', close_paren)
        declaration_end = source.find(';', close_paren)
        if body >= 0 and (declaration_end < 0 or body < declaration_end):
            start = source.rfind('inline ', 0, signature)
            break
        search_from = close_paren
    depth = 1
    end = body + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


source = (ROOT / 'tests/firmware/remote_modal_test.cpp.in').read_text()
helpers = ''
for file, name in [
    ('button_grid_actions.h', 'media_card_mode'),
    ('button_grid_media.h', 'media_control_modal_mode'),
    ('guest_wifi_state.h', 'guest_wifi_valid_entity'),
    ('button_grid_image.h', 'image_card_page_visible'),
    ('button_grid_image.h', 'image_card_context_visible_on_active_screen'),
    ('button_grid_image.h', 'image_card_context_on_active_screen'),
]:
    helpers += function(file, name)
for file, name in [
    ('button_grid_sliders.h', 'light_control'),
    ('button_grid_sliders.h', 'cover_control'),
    ('button_grid_sliders.h', 'media_volume'),
    ('button_grid_climate.h', 'climate_control'),
    ('button_grid_media.h', 'media_control'),
    ('button_grid_alarm.h', 'alarm_control'),
    ('button_grid_option_select.h', 'option_select'),
    ('button_grid_fan.h', 'fan_control'),
    ('button_grid_image.h', 'image_card'),
]:
    helpers += function(file, name + '_can_open_modal')
helpers += function('button_grid_fan.h', 'fan_control_supported')
helpers += 'namespace espcontrol::cards {\n'
for driver in ['light_control', 'cover_modal', 'climate_control', 'fan_control',
               'alarm', 'image', 'media', 'wifi_qr']:
    helpers += function('button_grid_' + driver + '_driver.h', driver + '_driver_matches')
helpers += function('button_grid_numeric_selectable_driver.h', 'numeric_selectable_driver_option_select')
for driver in ['light_control', 'cover_modal', 'climate_control', 'fan_control',
               'numeric_selectable', 'media', 'image', 'alarm', 'wifi_qr']:
    helpers += function('button_grid_' + driver + '_driver.h', driver + '_driver_modal_target')
helpers += '}\n'
helpers += function('button_grid_grid.h', 'grid_release_runtime_allocations')
source = source.replace('// PRODUCTION_HOOKS', helpers)
with tempfile.TemporaryDirectory(prefix='remote-modal-test-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra',
                    '-Werror', '-UNDEBUG', '-I', str(HEADERS), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Remote modal lookup, driver dispatch, and lifecycle tests passed.')
