#!/usr/bin/env python3
"""Verify that Product Model v2 sources preserve generated output exactly."""

from __future__ import annotations

import argparse
import copy
import json
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from product_model_v2 import ProductModelV2Error, load_product_model_v2


def load_json(path: Path) -> dict:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def assert_equivalence() -> None:
    model = load_product_model_v2()
    cards = load_json(model.source_path("cardContract"))["cards"]
    devices = load_json(model.source_path("deviceCatalog"))["devices"]
    assert model.card_type in cards, "Product Model v2 sample card must exist in the legacy card contract"
    assert model.device_slug in devices, "Product Model v2 sample device must exist in the legacy device catalog"

    # Canonical JSON makes these byte-for-byte comparisons of each selected
    # legacy payload and the Product Model adapter view used by generators.
    for card_type in model.pilot_cards:
        legacy_card = json.dumps(cards[card_type], sort_keys=True, separators=(",", ":"))
        adapter_card = json.dumps(model.pilot_json("cards", card_type), sort_keys=True, separators=(",", ":"))
        assert legacy_card == adapter_card, f"{card_type} card differs from its Product Model source"
    for device_slug in model.pilot_devices:
        legacy_device = json.dumps(devices[device_slug], sort_keys=True, separators=(",", ":"))
        adapter_device = json.dumps(model.pilot_json("devices", device_slug), sort_keys=True, separators=(",", ":"))
        assert legacy_device == adapter_device, f"{device_slug} device differs from its Product Model source"

    legacy_contract = json.dumps(load_json(model.source_path("cardContract")), sort_keys=True, separators=(",", ":"))
    generated_contract = json.dumps(model.card_contract_data(), sort_keys=True, separators=(",", ":"))
    legacy_catalog = json.dumps(load_json(model.source_path("deviceCatalog")), sort_keys=True, separators=(",", ":"))
    generated_catalog = json.dumps(model.device_catalog_data(), sort_keys=True, separators=(",", ":"))
    assert legacy_contract == generated_contract, "Product Model card contract output differs from legacy output"
    assert legacy_catalog == generated_catalog, "Product Model device catalog output differs from legacy output"


def run_self_test() -> None:
    assert_equivalence()
    model_path = Path(__file__).resolve().parent.parent / "product" / "model_v2.json"
    data = load_json(model_path)
    invalid = copy.deepcopy(data)
    invalid["sources"].pop("icons")
    with TemporaryDirectory() as directory:
        path = Path(directory) / "model.json"
        path.write_text(json.dumps(invalid), encoding="utf-8")
        try:
            load_product_model_v2(path)
        except ProductModelV2Error as exc:
            assert "sources must define every Product Model v2 source exactly once" in str(exc)
        else:
            raise AssertionError("missing product source must fail validation")
    invalid = copy.deepcopy(data)
    invalid["pilots"]["cards"] = {}
    with TemporaryDirectory() as directory:
        path = Path(directory) / "model.json"
        path.write_text(json.dumps(invalid), encoding="utf-8")
        try:
            load_product_model_v2(path)
        except ProductModelV2Error as exc:
            assert "pilots.cards must be a non-empty object" in str(exc)
        else:
            raise AssertionError("a generated pilot must define its sample card")
    default_card = copy.deepcopy(data)
    default_card["pilots"]["cards"][""] = default_card["pilots"]["cards"]["sensor"]
    with TemporaryDirectory() as directory:
        path = Path(directory) / "model.json"
        path.write_text(json.dumps(default_card), encoding="utf-8")
        model = load_product_model_v2(path)
        assert "" in model.pilot_cards, "the default switch fallback must be a valid card pilot"
    invalid = copy.deepcopy(data)
    invalid["pilots"]["devices"][""] = invalid["pilots"]["devices"][next(iter(invalid["pilots"]["devices"]))]
    with TemporaryDirectory() as directory:
        path = Path(directory) / "model.json"
        path.write_text(json.dumps(invalid), encoding="utf-8")
        try:
            load_product_model_v2(path)
        except ProductModelV2Error as exc:
            assert "pilots.devices identifiers must be non-empty strings" in str(exc)
        else:
            raise AssertionError("an unnamed device pilot must fail validation")
    incomplete = copy.deepcopy(data)
    incomplete["pilots"]["cards"].pop(
        next(card_type for card_type in incomplete["pilots"]["cards"] if card_type != "sensor")
    )
    with TemporaryDirectory() as directory:
        path = Path(directory) / "model.json"
        path.write_text(json.dumps(incomplete), encoding="utf-8")
        try:
            load_product_model_v2(path)
        except ProductModelV2Error as exc:
            assert "generated-source cards must define every card-contract entry exactly once" in str(exc)
        else:
            raise AssertionError("an incomplete generated Product Model card source set must fail validation")
    incomplete = copy.deepcopy(data)
    incomplete["pilots"]["devices"].pop(
        next(device_slug for device_slug in incomplete["pilots"]["devices"] if device_slug != data["equivalenceSamples"]["deviceSlug"])
    )
    with TemporaryDirectory() as directory:
        path = Path(directory) / "model.json"
        path.write_text(json.dumps(incomplete), encoding="utf-8")
        try:
            load_product_model_v2(path)
        except ProductModelV2Error as exc:
            assert "generated-source devices must define every device-catalog entry exactly once" in str(exc)
        else:
            raise AssertionError("an incomplete generated Product Model device source set must fail validation")
    print("Product Model v2 self-test passed.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run Product Model v2 validator self-tests")
    args = parser.parse_args()
    try:
        if args.self_test:
            run_self_test()
        else:
            assert_equivalence()
            print("Product Model v2 generated pilots match their legacy card and device outputs.")
    except (AssertionError, ProductModelV2Error, KeyError) as exc:
        print(f"ERROR: {exc}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
