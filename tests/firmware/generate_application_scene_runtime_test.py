#!/usr/bin/env python3
"""Compile complete production application scene lifecycle methods in a host harness."""
import argparse
from pathlib import Path
import re
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'components/espcontrol/espcontrol_app.cpp').read_text()
text=Path(__file__).with_name('application_scene_runtime_test.cpp.in').read_text()
for name in re.findall(r'// @production-app-method (\w+)',text):
    pattern=rf'^(?:bool|void|configuration::ConfigurationRuntimeAdapter\s*\*)\s*EspControlApp::{name}\([^;]*?\{{\n.*?^\}}'
    matches=re.findall(pattern,source,re.M|re.S)
    if len(matches)!=1: raise ValueError(f'Expected one production definition for {name}')
    text=text.replace('// @production-app-method '+name,matches[0])
args.output.write_text('// Generated from production application lifecycle methods.\n'+text)
