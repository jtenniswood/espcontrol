#!/usr/bin/env python3
"""Extract complete production media send helpers for host transport tests."""
import argparse
from pathlib import Path
import re
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_actions.h').read_text()
text = Path(__file__).with_name('media_action_helpers_test.cpp.in').read_text()
for name in re.findall(r'// @production-function (\w+)', text):
    text = text.replace('// @production-function ' + name, definition(source, name))
args.output.write_text('// Generated from production media action helpers.\n' + text)
