#!/usr/bin/env python3
"""Extract complete production device send helpers for host transport tests."""
import argparse
from pathlib import Path
import re
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_actions.h').read_text()
# Limit the unique-name extractor to the ParsedCfg overload and its dependencies.
source = source.split('\ninline void send_cover_command_action(const std::string', 1)[0]
text = Path(__file__).with_name('device_action_helpers_test.cpp.in').read_text()
for name in re.findall(r'// @production-function (\w+)', text):
    text = text.replace('// @production-function ' + name, definition(source, name))
args.output.write_text('// Generated from production device action helpers.\n' + text)
