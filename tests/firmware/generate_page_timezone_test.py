#!/usr/bin/env python3
"""Extract the production timezone table, option resolution and clock presentation."""
import argparse
import re
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
sun = (root / 'components/espcontrol/sun_calc.h').read_text()
calendar = (root / 'components/espcontrol/button_grid_datetime_cards.h').read_text()
clock = (root / 'components/espcontrol/clock_bar.h').read_text()
reference = sun[sun.index('struct TzCoord {'):sun.index('inline const char* apply_timezone')]
reference = reference.replace('#if defined(USE_TIME_TIMEZONE)\ninline bool set_global_timezone_from_posix(const char *posix);\n#endif', '')
def function(source, name):
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
    assert len(functions) == 1, name
    return functions[0] + '\n'
reference += '''\nstatic std::string global_posix = "UTC0";
inline bool posix_timezone_matches_global(const char* posix) { return global_posix == posix; }
'''
for name in ['effective_timezone_option', 'effective_timezone_view', 'timezone_id_view', 'timezone_posix_view', 'timezone_offset_minutes_at_utc']:
    reference += function(sun, name)
for name in ['TimezoneCardRef', 'TimezoneCardState']:
    reference += re.search(rf'^struct {name} \{{.*?^\}};', calendar, re.M | re.S)[0] + '\n'
for source, name in [(clock, 'format_clock_time_without_suffix'), (calendar, 'timezone_card_state'), (calendar, 'timezone_city_label'), (calendar, 'timezone_localtime'), (calendar, 'apply_timezone_card_text'), (calendar, 'update_timezone_cards')]:
    reference += function(source, name)
template = Path(__file__).with_name('page_timezone_runtime_test.cpp.in').read_text()
assert template.count('// @timezone-reference') == 1
args.output.write_text(template.replace('// @timezone-reference', reference))
