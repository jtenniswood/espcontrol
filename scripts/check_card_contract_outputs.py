#!/usr/bin/env python3
"""Cross-check generated card contract outputs against the authored contract."""

from __future__ import annotations

import json
import re
import sys

from product_schema import CARD_CONTRACT_JSON, ROOT, load_card_contract, validate_card_contract


CARD_CONTRACT_TS = ROOT / "src" / "webserver" / "generated" / "card_contract.ts"
CARD_CONTRACT_H = ROOT / "components" / "espcontrol" / "button_grid_contract_generated.h"
CARD_CAPABILITY_DOCS = ROOT / "docs" / "generated" / "cards" / "capabilities.md"
OPTION_CONSTANT_RE = re.compile(r'^constexpr const char \*(CARD_CONTRACT_OPTION_NAME_[A-Z0-9_]+) = ("(?:[^"\\]|\\.)*");$', re.M)


def cpp_string(value: str) -> str:
    return json.dumps(value)


def card_type_name(card_type: str) -> str:
    return card_type or "switch"


def option_names(data: dict) -> dict[str, str]:
    names: dict[str, str] = {}
    for name in data.get("optionNames", []):
        if name:
            names[name] = name
    for card in data["cards"].values():
        for option in card.get("options", []):
            name = option.get("name")
            if name:
                names[name] = name
            for storage_name in option.get("storage", []):
                names[storage_name] = storage_name
    return dict(sorted(names.items()))


def option_constant_name(option_name: str) -> str:
    normalized = re.sub(r"[^A-Za-z0-9]+", "_", option_name).strip("_").upper()
    return f"CARD_CONTRACT_OPTION_NAME_{normalized}"


def header_option_constants(header: str) -> dict[str, str]:
    return {
        name: json.loads(value)
        for name, value in OPTION_CONSTANT_RE.findall(header)
    }


def assert_contains(text: str, needle: str, label: str) -> None:
    assert needle in text, f"{label} missing {needle!r}"


def assert_ts_contract(data: dict, ts: str) -> None:
    assert_contains(
        ts,
        f"export const CARD_CONTRACT_VERSION = {data['contractVersion']} as const;",
        "typed web card contract version",
    )
    assert_contains(
        ts,
        "export const CARD_CONTRACT_MIGRATION_ACTIONS:",
        "typed web card contract migration actions",
    )
    for hook in data["normalizationHooks"]:
        assert_contains(ts, json.dumps(hook), f"typed web card contract hook {hook}")
    assert_contains(
        ts,
        f"export const CARD_CONFIG_FIELDS = {json.dumps(data['fields'])} as const",
        "typed web card contract",
    )
    assert_contains(
        ts,
        f"export const CARD_CONTRACT_OPTION_NAMES: Readonly<Record<string, string>> = {json.dumps(option_names(data), indent=2)};",
        "typed web card contract option names",
    )
    for card_type, card in data["cards"].items():
        assert_contains(ts, json.dumps(card_type), f"web card contract card {card_type_name(card_type)}")
        assert_contains(ts, json.dumps(card["label"]), f"web card contract card {card_type_name(card_type)} label")
        for field, value in card["default"].items():
            assert_contains(ts, json.dumps(field), f"web card contract card {card_type_name(card_type)} default field {field}")
            if value:
                assert_contains(ts, json.dumps(value), f"web card contract card {card_type_name(card_type)} default value {field}")
    for alias, target in data.get("migrationAliases", {}).items():
        assert_contains(ts, json.dumps(alias), f"web card contract migration alias {alias}")
        for field, value in target.items():
            assert_contains(ts, json.dumps(field), f"web card contract migration alias {alias} field {field}")
            assert_contains(ts, json.dumps(value), f"web card contract migration alias {alias} value {field}")
    for card_type, code in data["subpageTypeCodes"].items():
        assert_contains(ts, json.dumps(card_type), f"web card contract subpage type {card_type}")
        assert_contains(ts, json.dumps(code), f"web card contract subpage code {code}")


def assert_h_contract(data: dict, header: str) -> None:
    assert_contains(
        header,
        f"constexpr int CARD_CONTRACT_VERSION = {data['contractVersion']};",
        "firmware card contract version",
    )
    for card_type, card in data["cards"].items():
        escaped_type = re.escape(cpp_string(card_type))
        escaped_label = re.escape(cpp_string(card["label"]))
        assert re.search(
            rf"if \(type == {escaped_type}\) return {escaped_label};",
            header,
        ), f"firmware card contract label missing for {card_type_name(card_type)}"

        escaped_icon = re.escape(cpp_string(card["default"]["icon"]))
        assert re.search(
            rf"if \(type == {escaped_type}\) return {escaped_icon};",
            header,
        ), f"firmware card contract default icon missing for {card_type_name(card_type)}"

        expected_subpage = "true" if card["allowInSubpage"] else "false"
        assert re.search(
            rf"if \(type == {escaped_type}\) return {expected_subpage};",
            header,
        ), f"firmware card contract subpage rule missing for {card_type_name(card_type)}"

    for card_type, code in data["subpageTypeCodes"].items():
        assert_contains(
            header,
            f'if (type == "{card_type}") return "{code}";',
            f"firmware card contract subpage code {card_type}",
        )
        assert_contains(
            header,
            f'if (code == "{code}") return "{card_type}";',
            f"firmware card contract subpage decode {code}",
        )

    for action in data["optionSelect"]["actions"]:
        assert_contains(header, cpp_string(action), f"firmware option-select action {action}")

    expected_constants = {
        option_constant_name(option_name): option_name
        for option_name in option_names(data)
    }
    actual_constants = header_option_constants(header)
    missing = sorted(set(expected_constants) - set(actual_constants))
    extra = sorted(set(actual_constants) - set(expected_constants))
    wrong = sorted(
        name for name in set(expected_constants) & set(actual_constants)
        if actual_constants[name] != expected_constants[name]
    )
    assert not missing and not extra and not wrong, (
        "firmware option name constants differ "
        f"(missing: {missing or 'none'}, extra: {extra or 'none'}, wrong: {wrong or 'none'})"
    )


def assert_docs_contract(data: dict, docs: str) -> None:
    assert_contains(docs, "Generated by scripts/build.py from common/config/card_contract.json.", "card capability docs")
    for card_type, card in data["cards"].items():
        assert_contains(docs, f"| {card['label']} | {card_type_name(card_type)} |", f"card capability docs {card_type_name(card_type)}")


def main() -> int:
    data = load_card_contract()
    errors = validate_card_contract(data)
    if errors:
        print(f"ERROR: {CARD_CONTRACT_JSON.relative_to(ROOT)} failed validation:")
        for error in errors:
            print(f"  - {error}")
        return 1

    ts = CARD_CONTRACT_TS.read_text(encoding="utf-8")
    header = CARD_CONTRACT_H.read_text(encoding="utf-8")
    docs = CARD_CAPABILITY_DOCS.read_text(encoding="utf-8")
    assert_ts_contract(data, ts)
    assert_h_contract(data, header)
    assert_docs_contract(data, docs)
    print("Generated card contract outputs match the authored contract.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as exc:
        print(f"ERROR: {exc}")
        sys.exit(1)
