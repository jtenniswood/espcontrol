#!/usr/bin/env python3
"""Compile production climate callbacks with host display/network test doubles.

The climate UI header depends on LVGL and ESPHome. Copy complete definitions,
unchanged, into the host harness so callback wiring is tested without maintaining
a second implementation or adding test-only branches to the firmware.
"""

import argparse
from pathlib import Path
import re


def definition(source: str, name: str) -> str:
    # Production top-level definitions end at an unindented closing brace.
    # Require exactly one definition; missing/ambiguous functions fail generation.
    pattern = rf"^inline [^\n]*\b{re.escape(name)}\([^;]*?\{{\n.*?^\}}"
    matches = re.findall(pattern, source, re.MULTILINE | re.DOTALL)
    if len(matches) != 1:
        raise ValueError(f"Expected one production definition for {name}")
    return matches[0]


def generate(header: Path) -> str:
    source = header.read_text(encoding="utf-8")
    template = Path(__file__).with_name("climate_update_test.cpp.in").read_text(encoding="utf-8")
    context = re.search(r"^struct ClimateControlCtx \{\n.*?^\};", source, re.MULTILINE | re.DOTALL)
    if context is None:
        raise ValueError("Missing production ClimateControlCtx")
    constants = re.findall(
        r"^constexpr int CLIMATE_(?:DEFAULT_\w+|WHOLE_NUMBER_STEP_TENTHS) = [^;]+;",
        source, re.MULTILINE,
    )
    template = template.replace("// @production-context", "\n".join(constants) + "\n" + context[0])
    for name in re.findall(r"// @production-function (\w+)", template):
        template = template.replace(f"// @production-function {name}", definition(source, name))
    return "// Generated from button_grid_climate.h; do not edit.\n" + template


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--header", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(args.header), encoding="utf-8")
