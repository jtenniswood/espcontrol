#!/usr/bin/env python3
"""Extract current calendar producers and parsers for state/order parity checks."""
import argparse
import re
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_datetime_cards.h').read_text()
reference = re.search(r'^struct CalendarDateState \{.*?^\};', source, re.M | re.S)[0]
for name in ['calendar_date_state', 'calendar_date_valid', 'update_calendar_cards', 'update_calendar_cards_time', 'parse_calendar_date_text', 'parse_calendar_date_text_dmy', 'update_calendar_cards_from_date_text']:
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
    assert len(functions) == 1, name
    reference += '\n' + functions[0]
template = Path(__file__).with_name('page_calendar_runtime_test.cpp.in').read_text()
assert template.count('// @legacy-calendar-reference') == 1
args.output.write_text(template.replace('// @legacy-calendar-reference', reference))
