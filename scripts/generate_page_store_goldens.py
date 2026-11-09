#!/usr/bin/env python3
"""Expose the authored card normalization corpus to PageStore integration tests."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from generate_saved_config_parser_test import FIELDS, fixture_groups


def generate() -> str:
    rows = []
    for _group, fixtures in fixture_groups():
        for fixture in fixtures:
            expected = ", ".join(json.dumps(fixture["expected"][field], ensure_ascii=False)
                                 for field in FIELDS)
            for source in ("input", "canonical"):
                if source in fixture:
                    encoded = json.dumps(fixture[source], ensure_ascii=False)
                    rows.append(f"  {{{encoded}, {{{{{expected}}}}}}},")
    return "\n".join([
        "// Generated from the authored common/config normalization fixtures.",
        "struct PageStoreGolden { const char *input; std::array<const char *, 9> expected; };",
        "constexpr PageStoreGolden PAGE_STORE_GOLDENS[] = {",
        *rows,
        "};", "",
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(), encoding="utf-8")
