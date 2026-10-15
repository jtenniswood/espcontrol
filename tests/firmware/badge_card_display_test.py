"""Execute the shared badge display acceptance matrix against native policy."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
cases = json.loads((root / 'tests/fixtures/badge_card_display.json').read_text())
with tempfile.TemporaryDirectory(prefix='espcontrol-badge-display-') as directory:
    directory = Path(directory)
    source = directory / 'test.cpp'
    body = ['#include "badge_card_display_policy.h"', '#include <iostream>', 'int main() {', 'using namespace espcontrol::badge;']
    for case in cases:
        args = ', '.join(json.dumps(case[key]) for key in ['type', 'mode', 'precision', 'display'])
        body.append(f'if (card_content({args}) != CardContent::{case["content"].upper()}) {{ std::cerr << {json.dumps(case["name"])}; return 1; }}')
    body.append('}')
    source.write_text('\n'.join(body))
    binary = directory / 'test'
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I'+str(root/'components/espcontrol'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print(f'Native badge display policy passed {len(cases)} acceptance cases.')
