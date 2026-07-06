#!/usr/bin/env python3
"""Validate the generated combined EspControl product snapshot."""

from __future__ import annotations

import argparse
import sys

from product_schema import (
    PRODUCT_SNAPSHOT_JSON,
    ProductSchemaError,
    product_snapshot_text,
    rel,
    validate_product_sources,
)


def print_source_errors(results: dict[str, list[str]]) -> bool:
    failed = False
    for source, errors in results.items():
        if not errors:
            continue
        failed = True
        print(f"ERROR: {source} failed validation:")
        for error in errors:
            print(f"  - {error}")
    return failed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--update", action="store_true", help="rewrite product/product_snapshot.json")
    args = parser.parse_args()

    try:
        source_results = validate_product_sources()
        if print_source_errors(source_results):
            return 1
        expected = product_snapshot_text()
    except ProductSchemaError as exc:
        print(f"ERROR: {exc}")
        return 1

    if args.update:
        PRODUCT_SNAPSHOT_JSON.write_text(expected, encoding="utf-8")
        print(f"Updated {rel(PRODUCT_SNAPSHOT_JSON)}.")
        return 0

    if not PRODUCT_SNAPSHOT_JSON.exists():
        print("ERROR: product/product_snapshot.json is missing. Run 'python3 scripts/check_product_snapshot.py --update'.")
        return 1

    actual = PRODUCT_SNAPSHOT_JSON.read_text(encoding="utf-8")
    if actual != expected:
        print("ERROR: product/product_snapshot.json is stale. Run 'python3 scripts/check_product_snapshot.py --update'.")
        return 1

    print("Product snapshot is current.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
