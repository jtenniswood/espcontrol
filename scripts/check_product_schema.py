#!/usr/bin/env python3
"""Validate EspControl product source files together."""

from __future__ import annotations

import argparse
import copy
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

from product_schema import (
    ProductSchemaError,
    load_card_contract,
    load_entity_names,
    load_json,
    validate_card_contract,
    validate_entity_names,
    validate_product_sources,
)


def expect_error(errors: list[str], expected: str) -> None:
    if not any(expected in error for error in errors):
        raise AssertionError(f"expected error containing {expected!r}, got {errors!r}")


def first_card_with_options(cards: dict) -> tuple[str, dict]:
    for card_type, card in cards.items():
        if card.get("options"):
            return card_type, card
    raise AssertionError("card contract fixture needs at least one card with options")


def first_card_with_normalization(cards: dict) -> tuple[str, dict]:
    for card_type, card in cards.items():
        if card.get("normalization"):
            return card_type, card
    raise AssertionError("card contract fixture needs at least one normalized card")


def run_self_test() -> int:
    results = validate_product_sources()
    assert all(not errors for errors in results.values())

    entity_names = load_entity_names()
    invalid_entities = copy.deepcopy(entity_names)
    invalid_entities["entities"].append(copy.deepcopy(invalid_entities["entities"][0]))
    expect_error(validate_entity_names(invalid_entities), "duplicates")

    invalid_template = copy.deepcopy(entity_names)
    invalid_template["entities"].append(
        {"key": "self_test_template", "domain": "text", "template": "Button Config"}
    )
    expect_error(validate_entity_names(invalid_template), "must contain {slot}")

    card_contract = load_card_contract()
    invalid_fields = copy.deepcopy(card_contract)
    invalid_fields["fields"] = list(reversed(invalid_fields["fields"]))
    expect_error(validate_card_contract(invalid_fields), "must match the saved button config field order")

    invalid_version = copy.deepcopy(card_contract)
    invalid_version["contractVersion"] = 2
    expect_error(validate_card_contract(invalid_version), "contractVersion: must be 1")

    invalid_option = copy.deepcopy(card_contract)
    _, card = first_card_with_options(invalid_option["cards"])
    card["options"][0]["kind"] = "toggle"
    expect_error(validate_card_contract(invalid_option), "must be choice, flag, number, or text")

    missing_option_default = copy.deepcopy(card_contract)
    _, card = first_card_with_options(missing_option_default["cards"])
    choice = next(option for option in card["options"] if option.get("kind") in {"choice", "number"})
    choice.pop("defaultValue", None)
    expect_error(validate_card_contract(missing_option_default), "is required for choice and number options")

    duplicate_storage = copy.deepcopy(card_contract)
    action_options = duplicate_storage["cards"]["action"]["options"]
    action_options[1]["storage"] = ["large_numbers"]
    expect_error(validate_card_contract(duplicate_storage), "duplicates storage name")

    invalid_hook = copy.deepcopy(card_contract)
    _, normalized_card = first_card_with_normalization(invalid_hook["cards"])
    normalized_card["normalization"]["fields"]["icon"] = {"policy": "hook", "hook": "run_arbitrary_code"}
    expect_error(validate_card_contract(invalid_hook), "must name an allow-listed normalization hook")

    invalid_hook_data = copy.deepcopy(card_contract)
    invalid_hook_data["cards"]["vacuum"]["normalization"]["hookData"]["unlisted_hook"] = {}
    expect_error(validate_card_contract(invalid_hook_data), "contains hooks outside the allow-list")

    missing_vacuum_default_icon = copy.deepcopy(card_contract)
    del missing_vacuum_default_icon["cards"]["vacuum"]["normalization"]["hookData"]["normalize_vacuum_fields"]["defaultIcons"]["default"]
    expect_error(validate_card_contract(missing_vacuum_default_icon), "must include default")

    invalid_allowed_alias = copy.deepcopy(card_contract)
    invalid_allowed_alias["cards"]["vacuum"]["normalization"]["fields"]["sensor"]["aliases"]["vacuum.old"] = "missing_mode"
    expect_error(validate_card_contract(invalid_allowed_alias), "must map to an allowed value")

    invalid_condition = copy.deepcopy(card_contract)
    invalid_condition["cards"]["sensor"]["options"][0]["applicability"][0]["operator"] = "matches"
    expect_error(validate_card_contract(invalid_condition), "must be equals, in, or present")

    invalid_subpage_codes = copy.deepcopy(card_contract)
    code_items = list(invalid_subpage_codes["subpageTypeCodes"].items())
    invalid_subpage_codes["subpageTypeCodes"][code_items[1][0]] = code_items[0][1]
    expect_error(validate_card_contract(invalid_subpage_codes), "duplicates code")

    invalid_alias = copy.deepcopy(card_contract)
    invalid_alias.setdefault("migrationAliases", {})["self_test_alias"] = {"not_a_field": "value"}
    expect_error(validate_card_contract(invalid_alias), "is not a saved config field")

    missing_alias_target = copy.deepcopy(card_contract)
    missing_alias_target["migrationAliases"]["self_test_alias"] = {"type": "missing_card"}
    expect_error(validate_card_contract(missing_alias_target), "must reference a card defined in cards")

    reused_retired_code = copy.deepcopy(card_contract)
    reused_retired_code["retiredSubpageTypeCodes"] = [next(iter(reused_retired_code["subpageTypeCodes"].values()))]
    expect_error(validate_card_contract(reused_retired_code), "reuses active codes")

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

    print("Product schema self-test passed.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run validator self-tests")
    args = parser.parse_args(argv)

    if args.self_test:
        return run_self_test()
    try:
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
