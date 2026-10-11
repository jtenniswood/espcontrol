"""Render the production compact forecast template against provider edge cases."""
from pathlib import Path
import ast
import re
from datetime import datetime, timedelta, timezone
from jinja2 import Environment

root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_weather_daily_data.h').read_text()
body = source[source.index('return std::string('):]
literals = re.findall(r'"(?:\\.|[^"\\])*"', body)
template = ast.literal_eval(literals[0]) + 'weather.home' + ''.join(ast.literal_eval(v) for v in literals[1:])
now = datetime(2026, 10, 11, 12, tzinfo=timezone.utc)
def as_datetime(value):
    try: return datetime.fromisoformat(value)
    except (TypeError, ValueError): return None
units = {'temperature_unit': '°C', 'precipitation_unit': 'mm', 'wind_speed_unit': 'km/h'}
env = Environment()
env.globals.update(now=lambda: now, timedelta=timedelta, as_datetime=as_datetime,
                   as_local=lambda dt: dt.astimezone(timezone.utc),
                   state_attr=lambda entity, attr: units.get(attr))
def render(entries, keyed=True):
    response = {'forecast': entries}
    if keyed: response = {'weather.home': response}
    return env.from_string(template).render(response=response)
items = [{'datetime': f'2026-10-{d:02d}T00:00:00+00:00', 'temperature': -2,
          'templow': -8, 'condition': 'rainy', 'precipitation_probability': 0} for d in range(12, 25)]
output = render(list(reversed(items)))
lines = output.splitlines()
assert lines[0] == 'WF1|2026-10-11|2026-10-12|°C|mm|km/h', output
assert len(lines) == 8 and lines[1] == '2026-10-12|rainy|-2|-8|0|||', output
assert lines[-1].startswith('2026-10-18|') and len(output.encode()) < 2048
assert render(items, False) == render(items)
assert render([]).splitlines() == [lines[0]]
assert render([items[0],items[0]]).count('2026-10-12|rainy') == 1
missing = render([{'datetime':'2026-10-13T00:00:00+00:00'}])
assert missing.splitlines()[1] == '2026-10-13|||||||', missing
aliases = render([{'datetime':'2026-10-12T00:00:00+00:00', 'max_temp':18, 'temperature_min':0}])
assert aliases.splitlines()[1] == '2026-10-12||18|0||||', aliases
print('Daily forecast template: sorted/capped provider dates, duplicate dates, tomorrow-first, negative/zero/missing fields, empty and both response shapes passed.')
