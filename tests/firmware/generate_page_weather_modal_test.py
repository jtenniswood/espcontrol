#!/usr/bin/env python3
"""Generate native lifetime coverage using the production forecast modal UI."""
import argparse
import re
from pathlib import Path

from generate_climate_update_test import definition


def generate(root: Path) -> str:
    ui = (root / 'components/espcontrol/button_grid_weather_daily_ui.h').read_text()
    modal = (root / 'components/espcontrol/button_grid_weather_modal.h').read_text()
    text = Path(__file__).with_name('page_weather_modal_test.cpp.in').read_text()
    structures = 'using WeatherForecastView=espcontrol::weather::View;\n'
    for name, source in [
        ('WeatherDailyColumn', ui), ('WeatherDailyCard', ui),
        ('WeatherDailyModalColumn', modal), ('WeatherDailyModal', modal),
    ]:
        match = re.search(r'^struct ' + name + r' \{(?:[^\n]*\};|.*?^\};)', source, re.M | re.S)
        if match is None:
            raise ValueError(f'Missing production structure {name}')
        structures += match[0] + '\n'
    text = text.replace('// @production-context', structures)
    for name in re.findall(r'// @production-function (\w+)', text):
        source = ui if re.search(r'^inline [^\n]*\b' + re.escape(name) + r'\(', ui, re.M) else modal
        one_line = re.findall(
            r'^inline [^\n]*\b' + re.escape(name) + r'\([^\n]*\{[^\n]*\}[^\n]*$', source, re.M
        )
        extracted = one_line[0] if len(one_line) == 1 else definition(source, name)
        text = re.sub(
            r'^// @production-function ' + re.escape(name) + r'$',
            lambda _: extracted, text, flags=re.M,
        )
    return '// Generated from production weather modal helpers.\n' + text


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(Path(__file__).resolve().parents[2]))
