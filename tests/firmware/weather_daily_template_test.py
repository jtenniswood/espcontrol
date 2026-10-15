"""Render the production compact forecast template against provider edge cases."""
from pathlib import Path
import ast
import re
from datetime import datetime, timedelta, timezone
from jinja2.sandbox import ImmutableSandboxedEnvironment

root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_weather_daily_data.h').read_text()
body = source[source.index('return std::string('):]
literals = re.findall(r'"(?:\\.|[^"\\])*"', body)
template = ast.literal_eval(literals[0]) + 'weather.home' + ''.join(ast.literal_eval(v) for v in literals[1:])
now = datetime(2026, 10, 11, 12, tzinfo=timezone.utc)
def as_datetime(value, default=None):
    try: return datetime.fromisoformat(value)
    except (TypeError, ValueError): return default
units = {'temperature_unit': '°C', 'precipitation_unit': 'mm', 'wind_speed_unit': 'km/h'}
env = ImmutableSandboxedEnvironment()
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
assert len(lines) == 9 and lines[1] == '2026-10-12|rainy|-2|-8|0|||', output
assert lines[-1].startswith('2026-10-19|') and len(output.encode()) < 2048
assert render(items, False) == render(items)
assert render([]).splitlines() == [lines[0]]
assert render([items[0],items[0]]).count('2026-10-12|rainy') == 1
missing = render([{'datetime':'2026-10-13T00:00:00+00:00'}])
assert missing.splitlines()[1] == '2026-10-13|||||||', missing
aliases = render([{'datetime':'2026-10-12T00:00:00+00:00', 'max_temp':18, 'temperature_min':0}])
assert aliases.splitlines()[1] == '2026-10-12||18|0||||', aliases
print('Daily forecast template: sorted/capped provider dates, duplicate dates, tomorrow-first, negative/zero/missing fields, empty and both response shapes passed.')

# Date-only integrations, mixed date fields, invalid timestamps and local dates.
dated = [{'date':'2026-10-13','temperature':13},
         {'datetime':'bad','date':'2026-10-12','temperature':12},
         {'datetime':'2026-10-11T23:00:00-02:00','temperature':14},
         {'date':'2026-10-10','temperature':10}, {'date':'invalid'}, {}]
result = render(dated).splitlines()
assert len(result)==3 and result[1].startswith('2026-10-12||12|') and result[2].startswith('2026-10-13||13|'), result
assert render(list(reversed([{'date':f'2026-10-{d:02d}','temperature':d} for d in range(12,25)]))).splitlines()[-1].startswith('2026-10-19|')

# Source units follow the aliases, with item, response and entity fallbacks.
native = [{'date':'2026-10-12','native_temperature':68,'native_templow':50}]
units['native_temperature_unit']='°F'  # Display/entity temperature_unit is °C.
result=render(native).splitlines()
assert result[0].split('|')[3]=='°F' and result[1].split('|')[2:4]==['68','50'], result
units.pop('native_temperature_unit')
response={'weather.home':{'forecast':native,'native_temperature_unit':'°F'}}
result=env.from_string(template).render(response=response).splitlines()
assert result[0].split('|')[3]=='°F' and result[1].split('|')[2]=='68', result
native[0]['native_temperature_unit']='°F'
result=render(native,False).splitlines()
assert result[0].split('|')[3]=='°F', result
normal=[{'date':'2026-10-12','temperature':20,'templow':10,'native_temperature_unit':'°F'}]
assert render(normal).splitlines()[0].split('|')[3]=='°C'
# Different days/aliases are converted into one consistent snapshot unit.
mixed=normal+[dict(native[0],date='2026-10-13')]
result=render(mixed).splitlines()
assert result[0].split('|')[3]=='°C' and float(result[2].split('|')[2])==20 and float(result[2].split('|')[3])==10, result
mixed=[dict(native[0],date='2026-10-12'),dict(normal[0],date='2026-10-13')]
result=render(mixed).splitlines()
assert result[0].split('|')[3]=='°F' and float(result[2].split('|')[2])==68 and float(result[2].split('|')[3])==50, result
mixed=[{'date':'2026-10-12','temperature':20,'native_templow':50,'native_temperature_unit':'°F'}]
result=render(mixed).splitlines()
assert result[0].split('|')[3]=='°C' and float(result[1].split('|')[3])==10, result
print('Date-only, mixed/local dates and item/response/entity native-unit regressions passed.')
