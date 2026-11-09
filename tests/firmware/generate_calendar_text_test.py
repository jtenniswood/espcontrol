#!/usr/bin/env python3
"""Extract current calendar and clock text helpers for presentation parity."""
import argparse
import re
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
calendar = (root / 'components/espcontrol/button_grid_datetime_cards.h').read_text()
clock = (root / 'components/espcontrol/clock_bar.h').read_text()
reference = ''
for name in ['CalendarCardRef', 'CalendarDateState']:
    reference += re.search(rf'^struct {name} \{{.*?^\}};', calendar, re.M | re.S)[0] + '\n'
for source, name in [(clock, 'format_clock_time_without_suffix'), (calendar, 'calendar_month_name'), (calendar, 'apply_calendar_card_text')]:
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
    assert len(functions) == 1, name
    reference += functions[0] + '\n'
template = Path(__file__).with_name('calendar_card_text_test.cpp.in').read_text()
assert template.count('// @legacy-calendar-text') == 1
args.output.write_text(template.replace('// @legacy-calendar-text', reference))
