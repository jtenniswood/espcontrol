#!/usr/bin/env python3
"""Compare bounded cleaning policy with current production contexts and helpers."""
import argparse
import re
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
reference = ''
for file, context, names in [
    ('button_grid_vacuum.h', 'VacuumCardCtx', ['vacuum_card_mode', 'vacuum_card_mode_needs_state', 'vacuum_card_mode_label', 'vacuum_card_default_icon_name', 'vacuum_state_icon_name', 'vacuum_state_label', 'create_vacuum_card_context', 'apply_vacuum_card_state', 'vacuum_service_for_card']),
    ('button_grid_lawn_mower.h', 'LawnMowerCardCtx', ['lawn_mower_card_mode', 'lawn_mower_card_mode_needs_state', 'lawn_mower_card_mode_label', 'lawn_mower_card_default_icon_name', 'lawn_mower_state_icon_name', 'lawn_mower_state_label', 'create_lawn_mower_card_context', 'apply_lawn_mower_card_state', 'lawn_mower_service_for_card']),
]:
    source = (root / 'components/espcontrol' / file).read_text()
    reference += re.search(rf'^struct {context} \{{.*?^\}};', source, re.M | re.S)[0] + '\n'
    for name in names:
        match = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
        assert len(match) == 1, name
        reference += match[0] + '\n'
template = Path(__file__).with_name('cleaning_card_policy_test.cpp.in').read_text()
assert template.count('// @legacy-cleaning-reference') == 1
args.output.write_text(template.replace('// @legacy-cleaning-reference', reference))
