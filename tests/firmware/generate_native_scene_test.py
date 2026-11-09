#!/usr/bin/env python3
"""Run the complete production scene with real state/LVGL and doubled modal UI."""
import argparse
import re
from pathlib import Path

from generate_media_lifecycle_integration import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/page_native_scene.h').read_text()
template = Path(__file__).with_name('page_native_scene_test.cpp.in').read_text()
headers = root / 'components/espcontrol'

def struct(filename, name):
    text = (headers / filename).read_text()
    match = re.search(rf'^struct {name} \{{[\s\S]*?^\}};', text, re.M)
    if not match:
        raise ValueError(f'Missing production struct: {name}')
    return match.group()

environments = 'namespace espcontrol::cards {\n' + struct(
    'button_grid_climate_control_driver.h', 'ClimateControlDriverEnvironment') + '\n}\n'
environments += 'namespace espcontrol::pages {\n'
for family, name in (
        ('light_cover', 'PageControlModalEnvironment'),
        ('fan_control', 'PageFanControlModalEnvironment'),
        ('option_select', 'PageOptionSelectModalEnvironment'),
        ('media_volume', 'PageMediaVolumeModalEnvironment'),
        ('media_control', 'PageMediaControlModalEnvironment'),
        ('weather', 'PageWeatherModalStyle'),
        ('confirmation', 'PageConfirmationModalEnvironment'),
        ('alarm', 'PageAlarmModalEnvironment'),
        ('expanded_image', 'PageExpandedImageModalEnvironment'),
        ('wifi_qr', 'PageWifiQrModalEnvironment'),
        ('network_status', 'PageNetworkStatusEnvironment')):
    environments += struct(f'page_{family}_modal.h', name) + '\n'
environments += '}\n'

display = (headers / 'button_grid_display.h').read_text()
helpers = display[display.index('struct DisplayFontRoles'):display.index('inline int display_width_percent')]
for filename, names in (
        ('button_grid_layout.h', ('parse_hex_color', 'normalize_width_compensation_percent')),
        ('button_grid_display.h', ('display_width_percent', 'display_main_width_percent',
          'display_volume_width_percent', 'display_text_width_percent', 'display_correct_color'))):
    for name in names:
        helpers += definition((headers / filename).read_text(), name)[1] + '\n'
# Use all actual font fallback functions in their production dependency order.
for name in re.findall(r'^inline const lv_font_t \*(display_\w+)\s*\(', display, re.M):
    if name != 'display_modal_volume_number_font':
        helpers += definition(display, name)[1] + '\n'
helpers += struct('button_grid_style.h', 'CardPalette') + '\n'
helpers += definition((headers / 'button_grid_grid.h').read_text(), 'card_palette_for_config')[1] + '\n'
helpers += definition((headers / 'icons.h').read_text(), 'domain_default_icon')[1] + '\n'

styles = (headers / 'page_native_device_styles.h').read_text()
device = (headers / 'page_native_device.h').read_text()
bank = device[device.index('namespace espcontrol::pages {'):device.index('inline PageNativeDeviceStyleProfile')]
template = template.replace('// @production-style-helpers', helpers)
template = template.replace('// @production-style-types', environments)
template = template.replace('// @production-device-styles', styles[styles.index('namespace espcontrol::pages {'):])
template = template.replace('// @production-widget-bank', bank + '\n}\n')
args.output.write_text('// Generated from the complete production native scene.\n' +
                       template.replace('// @production-scene', source[source.index('namespace espcontrol::pages {'):]))
