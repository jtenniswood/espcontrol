#!/usr/bin/env python3
"""Compare the pooled presenter against unchanged production climate helpers."""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--header', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
source = args.header.read_text()
text = Path(__file__).with_name('page_climate_parity_test.cpp.in').read_text()
for name in re.findall(r'// @production-function (\w+)', text):
    text = text.replace(f'// @production-function {name}', definition(source, name))
args.output.write_text('// Generated from unchanged production helpers.\n' + text)
