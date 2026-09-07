#!/usr/bin/env python3
"""Build a host harness from the production climate subscription functions.

Only LVGL rendering/transport boundaries are faked. Copy complete definitions
verbatim, with source locations, so registration, replay and deletion execute
the firmware code without compiling the unrelated modal renderer.
"""

import argparse
import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


def generate() -> str:
    source = ROOT / "components/espcontrol/button_grid_climate.h"
    text = source.read_text()

    def definition(pattern: str) -> str:
        matches = list(re.finditer(pattern, text, re.MULTILINE | re.DOTALL))
        if len(matches) != 1:
            raise ValueError(f"Expected one complete production definition: {pattern}")
        match = matches[0]
        line = text.count("\n", 0, match.start()) + 1
        return f'#line {line} {json.dumps(str(source))}\n{match.group()}\n'

    def function(name: str) -> str:
        return definition(r"^inline [^\n]*\b" + re.escape(name) +
                          r"\([^;{]*\)\s*\{.*?^\}")

    context = definition(r"^constexpr int CLIMATE_DEFAULT_TARGET_TENTHS.*?(?=^constexpr uint32_t CLIMATE_TEMP_DEBOUNCE_MS)")
    context += definition(r"^struct ClimateControlCtx \{.*?^\};")
    for name in (
        "climate_control_refs", "climate_control_ref_count", "reset_climate_control_refs",
        "climate_lower", "climate_trim", "climate_unavailable_value",
        "climate_parse_tenths", "climate_parse_supported_features",
        "climate_target_kind", "climate_target_values_complete",
        "climate_temperature_target_available", "climate_clean_option_token",
        "climate_hvac_service_value", "climate_parse_options",
    ):
        context += function(name)

    # This contiguous block contains the complete registration and maintenance
    # implementation, including forward declarations and all optional fields.
    runtime = definition(r"^inline lv_timer_t \*&climate_optional_subscription_timer\(\).*\Z")
    runtime += function("delete_climate_control_context")

    fixture = json.loads((ROOT / "tests/firmware/fixtures/issue_1837_climate_subpage.json").read_text())
    cards = fixture["subpage_cards"]
    if not cards or any(card["type"] != "climate_control" for card in cards):
        raise ValueError("Expected climate control fixture cards")
    fixture_code = "const FixtureCard fixture_cards[] = {\n"
    fixture_code += "".join(
        f'  {{{json.dumps(card["entity"])}, {json.dumps(card["climate_tabs"])}}},\n'
        for card in cards
    )
    fixture_code += "};\n"
    fixture_code += f'constexpr size_t fixture_other_channels = {int(fixture["other_subscription_channels"])};\n'
    fixture_code += f'constexpr size_t fixture_expected_channels = {int(fixture["expected_initial_subscription_channels"])};\n'

    harness = ROOT / "tests/firmware/climate_subscription_runtime_test.cpp"
    result = harness.read_text()
    for marker, content in (("context", context), ("runtime", runtime), ("fixture", fixture_code)):
        token = f"// @production-{marker}"
        if result.count(token) != 1:
            raise ValueError(f"Expected one harness marker: {token}")
        line = harness.read_text().splitlines().index(token) + 2
        result = result.replace(token, content + f'\n#line {line} {json.dumps(str(harness))}')
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(generate())
