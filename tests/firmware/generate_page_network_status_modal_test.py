#!/usr/bin/env python3
"""Exercise demanded Settings with production card construction and wrapping."""
import argparse
import json
from pathlib import Path
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
cards = (root / 'components/espcontrol/button_grid_subpages.h').read_text()
layout = (root / 'components/espcontrol/button_grid_layout.h').read_text()
template = Path(__file__).with_name('page_network_status_modal_test.cpp.in').read_text()
manifest = json.loads((root / 'devices/manifest.json').read_text())['devices']
geometries = '\n'.join('{"%s", %s, %s},' % (name, *device['public']['resolution'].split(' x ')) for name, device in manifest.items())
template = template.replace('// @device-geometries', 'struct DeviceGeometry {const char* name; int width,height;};\nconst DeviceGeometry geometries[]={\n' + geometries + '\n};')
args.output.write_text('// Generated production card helpers.\n' + template.replace(
    '// @card-factory', definition(cards, 'create_grid_card_button')).replace(
    '// @label-wrap', definition(layout, 'configure_button_label_wrap')))
