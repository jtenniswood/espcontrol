#!/usr/bin/env python3
"""Validate EspControl product source files together."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from product_schema import ProductSchemaError, load_json, validate_product_sources


def self_test() -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        valid = root / "valid.json"
        valid.write_text('{"outer": {"name": "primary"}, "items": [{"name": "nested"}]}\n', encoding="utf-8")
        parsed = load_json(valid)
        assert parsed["outer"]["name"] == "primary"
        assert parsed["items"][0]["name"] == "nested"

        duplicate = root / "duplicate.json"
        duplicate.write_text('{"outer": {"name": "first", "name": "second"}}\n', encoding="utf-8")
        try:
            load_json(duplicate)
        except ProductSchemaError as exc:
            assert "duplicate JSON key 'name'" in str(exc)
        else:
            raise AssertionError("duplicate JSON keys must fail validation")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true", help="run local checker self-tests")
    args = parser.parse_args(argv)

    try:
        if args.self_test:
            self_test()
            print("Product schema self-test passed.")
            return 0

        results = validate_product_sources()
    except ProductSchemaError as exc:
        print(f"ERROR: {exc}")
        return 1

    failed = False
    for source, errors in results.items():
        if not errors:
            continue
        failed = True
        print(f"ERROR: {source} failed validation:")
        for error in errors:
            print(f"  - {error}")

    if failed:
        return 1

    print("Product schema sources passed validation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
