#!/usr/bin/env python3
"""Exercise native modal commands with the unchanged canonical action helpers."""
import argparse
from pathlib import Path
import re
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_actions.h').read_text()
text = Path(__file__).with_name('page_native_modal_commands_test.cpp.in').read_text()
record = re.search(r'^struct ActionCardScriptField \{\n.*?^\};', source, re.M | re.S)
if record is None:
    raise ValueError('Missing production script field record')
text = text.replace('// @production-record', record[0])
text = re.sub(r'// @production-function (\w+)\b', lambda match: definition(source, match[1]), text)
bank = (root / 'components/espcontrol/page_native_modal_bank.h').read_text()
text = text.replace('// @production-modal-bank', bank[bank.index('namespace espcontrol::pages {'):])
args.output.write_text('// Generated from production action helpers.\n' + text)
