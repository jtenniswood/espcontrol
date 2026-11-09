#!/usr/bin/env python3
"""Extract current sensor/climate layout functions for pooled font parity."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
r = Path(__file__).resolve().parents[2]
ref = ''
for file, name in [('button_grid_sensor_driver.h', 'sensor_driver_refresh_layout'), ('button_grid_climate_control_driver.h', 'climate_control_driver_refresh_layout')]:
    s = (r / 'components/espcontrol' / file).read_text()
    functions = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', s, re.M | re.S)
    assert len(functions) == 1, name
    ref += functions[0] + '\n'
t = Path(__file__).with_name('basic_number_layout_test.cpp.in').read_text()
a.output.write_text(t.replace('// @legacy-number-layout', ref))
