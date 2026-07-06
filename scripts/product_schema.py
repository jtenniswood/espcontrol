#!/usr/bin/env python3
"""Shared validation for EspControl product source files.

This module intentionally uses only Python's standard library and local project
modules so checks can run in CI, release workflows, and local environments
without extra dependencies.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from device_profiles import (
    DEVICE_MANIFEST,
    load_device_profiles as load_normalized_device_profiles,
    slot_devices as load_slot_devices,
    validate_manifest_data,
)


ROOT = Path(__file__).resolve().parent.parent
CARD_CONTRACT_JSON = ROOT / "common" / "config" / "card_contract.json"
ENTITY_NAMES_JSON = ROOT / "common" / "config" / "entity_names.json"
ENTITY_IDENTIFIER_RE = re.compile(r"^[a-z0-9_]+$")
ICONS_JSON = ROOT / "common" / "assets" / "icons.json"
COMPATIBILITY_FIXTURES_JSON = ROOT / "compatibility" / "fixtures" / "product_compatibility.json"
PRODUCT_SNAPSHOT_JSON = ROOT / "product" / "product_snapshot.json"
PRODUCT_SNAPSHOT_VERSION = 1

SAVED_CONFIG_FIELDS = [
    "entity",
    "label",
    "icon",
    "icon_on",
    "sensor",
    "unit",
    "type",
    "precision",
    "options",
]


class ProductSchemaError(RuntimeError):
    pass


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def load_json(path: Path) -> Any:
    try:
        with path.open(encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError as exc:
        raise ProductSchemaError(f"{rel(path)} was not found") from exc
    except json.JSONDecodeError as exc:
        raise ProductSchemaError(f"{rel(path)} is not valid JSON: {exc}") from exc


def path_error(path: str, message: str) -> str:
    return f"{path}: {message}"


def load_card_contract(path: Path = CARD_CONTRACT_JSON) -> dict[str, Any]:
    data = load_json(path)
    if not isinstance(data, dict):
        raise ProductSchemaError(f"{rel(path)} must contain a JSON object")
    return data


def load_entity_names(path: Path = ENTITY_NAMES_JSON) -> dict[str, Any]:
    data = load_json(path)
    if not isinstance(data, dict):
        raise ProductSchemaError(f"{rel(path)} must contain a JSON object")
    return data

def validate_entity_identifier_list(entry: dict[str, Any], field: str, entry_path: str, errors: list[str]) -> None:
    if field not in entry:
        return
    values = entry[field]
    if values == []:
        return
    if not isinstance(values, list) or not all(isinstance(value, str) and value for value in values):
        errors.append(path_error(f"{entry_path}.{field}", "must be a list of non-empty strings"))
        return

    seen: set[str] = set()
    for index, value in enumerate(values):
        value_path = f"{entry_path}.{field}[{index}]"
        if value in seen:
            errors.append(path_error(value_path, f"duplicates {value!r}"))
        seen.add(value)
        if not ENTITY_IDENTIFIER_RE.fullmatch(value):
            errors.append(path_error(value_path, "must use lowercase letters, numbers, and underscores only"))


def load_icon_registry(path: Path = ICONS_JSON) -> dict[str, Any]:
    data = load_json(path)
    if not isinstance(data, dict):
        raise ProductSchemaError(f"{rel(path)} must contain a JSON object")
    return data


def load_compatibility_fixtures(path: Path = COMPATIBILITY_FIXTURES_JSON) -> dict[str, Any]:
    data = load_json(path)
    if not isinstance(data, dict):
        raise ProductSchemaError(f"{rel(path)} must contain a JSON object")
    return data


def validate_entity_names(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    entries = data.get("entities")
    if not isinstance(entries, list):
        return [path_error("entities", "must be a list")]

    keys: set[str] = set()
    names_by_domain: dict[str, dict[str, list[str]]] = {}
    for index, entry in enumerate(entries):
        entry_path = f"entities[{index}]"
        if not isinstance(entry, dict):
            errors.append(path_error(entry_path, "must be an object"))
            continue

        key = entry.get("key")
        domain = entry.get("domain")
        name = entry.get("name")
        template = entry.get("template")
        if not isinstance(key, str) or not key:
            errors.append(path_error(f"{entry_path}.key", "must be a non-empty string"))
            continue
        if key in keys:
            errors.append(path_error(f"{entry_path}.key", f"duplicates {key!r}"))
        keys.add(key)
        if not isinstance(domain, str) or not domain:
            errors.append(path_error(f"{entry_path}.domain", "must be a non-empty string"))
        if bool(name) == bool(template):
            errors.append(path_error(entry_path, "define exactly one of name or template"))
        if template and "{slot}" not in template:
            errors.append(path_error(f"{entry_path}.template", "must contain {slot}"))
        value = name or template
        if isinstance(domain, str) and isinstance(value, str):
            names_by_domain.setdefault(domain, {}).setdefault(value, []).append(key)

        validate_entity_identifier_list(entry, "objectIds", entry_path, errors)
        validate_entity_identifier_list(entry, "groups", entry_path, errors)

    for domain, names in names_by_domain.items():
        for name, entry_keys in names.items():
            if len(entry_keys) > 1:
                errors.append(
                    path_error(
                        "entities",
                        f"duplicate entity name for {domain} {name!r}: {', '.join(entry_keys)}",
                    )
                )
    return errors


def validate_card_contract(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    fields = data.get("fields")
    if fields != SAVED_CONFIG_FIELDS:
        errors.append(path_error("fields", "must match the saved button config field order"))

    cards = data.get("cards")
    if not isinstance(cards, dict) or not cards:
        errors.append(path_error("cards", "must be a non-empty object"))
    else:
        for card_type, card in cards.items():
            card_path = f"cards.{card_type or '<switch>'}"
            if not isinstance(card_type, str):
                errors.append(path_error("cards", "keys must be strings"))
                continue
            if not isinstance(card, dict):
                errors.append(path_error(card_path, "must be an object"))
                continue
            if not isinstance(card.get("label"), str) or not card.get("label"):
                errors.append(path_error(f"{card_path}.label", "must be a non-empty string"))
            if not isinstance(card.get("allowInSubpage"), bool):
                errors.append(path_error(f"{card_path}.allowInSubpage", "must be a boolean"))
            domains = card.get("domains", [])
            if not isinstance(domains, list) or not all(isinstance(domain, str) for domain in domains):
                errors.append(path_error(f"{card_path}.domains", "must be a list of strings"))
            if "pickerKey" in card and not isinstance(card.get("pickerKey"), str):
                errors.append(path_error(f"{card_path}.pickerKey", "must be a string"))
            if "experimental" in card:
                errors.append(path_error(f"{card_path}.experimental", "is no longer supported; use a draft PR for testing"))
            if "hidden" in card and not isinstance(card.get("hidden"), bool):
                errors.append(path_error(f"{card_path}.hidden", "must be a boolean"))

            options = card.get("options", [])
            if "options" in card:
                if not isinstance(options, list):
                    errors.append(path_error(f"{card_path}.options", "must be a list"))
                else:
                    for idx, option in enumerate(options):
                        option_path = f"{card_path}.options[{idx}]"
                        if not isinstance(option, dict):
                            errors.append(path_error(option_path, "must be an object"))
                            continue
                        if not isinstance(option.get("name"), str) or not option.get("name"):
                            errors.append(path_error(f"{option_path}.name", "must be a non-empty string"))
                        if not isinstance(option.get("label"), str) or not option.get("label"):
                            errors.append(path_error(f"{option_path}.label", "must be a non-empty string"))
                        if "kind" in option and option.get("kind") not in {"choice", "flag", "number", "text"}:
                            errors.append(path_error(f"{option_path}.kind", "must be choice, flag, number, or text"))
                        for key in ("values", "storage"):
                            value = option.get(key)
                            if value is not None and (
                                not isinstance(value, list)
                                or not all(isinstance(item, str) for item in value)
                            ):
                                errors.append(path_error(f"{option_path}.{key}", "must be a list of strings"))
                        if "defaultValue" in option and not isinstance(option.get("defaultValue"), str):
                            errors.append(path_error(f"{option_path}.defaultValue", "must be a string"))
                        for key in ("min", "max", "step"):
                            if key in option and not isinstance(option.get(key), (int, float)):
                                errors.append(path_error(f"{option_path}.{key}", "must be a number"))
                        if "defaultValueByMode" in option:
                            value = option.get("defaultValueByMode")
                            if not isinstance(value, dict) or not all(
                                isinstance(k, str) and isinstance(v, str)
                                for k, v in value.items()
                            ):
                                errors.append(path_error(f"{option_path}.defaultValueByMode", "must be an object of strings"))
                        if "hidden" in option and not isinstance(option.get("hidden"), bool):
                            errors.append(path_error(f"{option_path}.hidden", "must be a boolean"))
                        if "migration" in option and option.get("migration") != "drop":
                            errors.append(path_error(f"{option_path}.migration", "must be drop"))
                        if "supportedWhen" in option:
                            value = option.get("supportedWhen")
                            if not isinstance(value, dict):
                                errors.append(path_error(f"{option_path}.supportedWhen", "must be an object"))
                            else:
                                for key in ("precision", "precisionNot", "entityDomains"):
                                    entries = value.get(key)
                                    if entries is not None and (
                                        not isinstance(entries, list)
                                        or not all(isinstance(item, str) for item in entries)
                                    ):
                                        errors.append(path_error(f"{option_path}.supportedWhen.{key}", "must be a list of strings"))
                                if "never" in value and not isinstance(value.get("never"), bool):
                                    errors.append(path_error(f"{option_path}.supportedWhen.never", "must be a boolean"))

            behavior = card.get("behavior")
            if behavior is not None:
                if not isinstance(behavior, dict):
                    errors.append(path_error(f"{card_path}.behavior", "must be an object"))
                else:
                    validate_card_behavior(card_path, behavior, errors)

            default = card.get("default")
            if not isinstance(default, dict):
                errors.append(path_error(f"{card_path}.default", "must be an object"))
            else:
                for field in SAVED_CONFIG_FIELDS:
                    if not isinstance(default.get(field), str):
                        errors.append(path_error(f"{card_path}.default.{field}", "must be a string"))

    aliases = data.get("migrationAliases", {})
    if not isinstance(aliases, dict):
        errors.append(path_error("migrationAliases", "must be an object"))
    else:
        for alias, target in aliases.items():
            alias_path = f"migrationAliases.{alias}"
            if not isinstance(alias, str) or not isinstance(target, dict):
                errors.append(path_error("migrationAliases", "keys must map to objects"))
                continue
            for field, value in target.items():
                if field not in SAVED_CONFIG_FIELDS:
                    errors.append(path_error(f"{alias_path}.{field}", "is not a saved config field"))
                if not isinstance(value, str):
                    errors.append(path_error(f"{alias_path}.{field}", "must be a string"))

    validate_subpage_type_codes(data, cards, errors)
    validate_option_select(data, errors)
    validate_card_groups(data, errors)

    large = data.get("largeNumbers")
    if not isinstance(large, dict):
        errors.append(path_error("largeNumbers", "must be an object"))
    return errors


def validate_card_behavior(card_path: str, behavior: dict[str, Any], errors: list[str]) -> None:
    light_temp = behavior.get("lightTemperature")
    if light_temp is not None:
        if not isinstance(light_temp, dict):
            errors.append(path_error(f"{card_path}.behavior.lightTemperature", "must be an object"))
        else:
            if not isinstance(light_temp.get("defaultRange"), str) or "-" not in light_temp.get("defaultRange", ""):
                errors.append(path_error(f"{card_path}.behavior.lightTemperature.defaultRange", "must be a range string"))
            for key in ("min", "max", "minMax", "step"):
                if not isinstance(light_temp.get(key), (int, float)):
                    errors.append(path_error(f"{card_path}.behavior.lightTemperature.{key}", "must be a number"))
            legacy = light_temp.get("legacySensorValues", [])
            if not isinstance(legacy, list) or not all(isinstance(item, str) for item in legacy):
                errors.append(path_error(f"{card_path}.behavior.lightTemperature.legacySensorValues", "must be a list of strings"))


def validate_subpage_type_codes(data: dict[str, Any], cards: Any, errors: list[str]) -> None:
    codes = data.get("subpageTypeCodes")
    if not isinstance(codes, dict) or not codes:
        errors.append(path_error("subpageTypeCodes", "must be a non-empty object"))
        return

    seen: dict[str, str] = {}
    for card_type, code in codes.items():
        code_path = f"subpageTypeCodes.{card_type}"
        if not isinstance(card_type, str) or not isinstance(code, str) or not code:
            errors.append(path_error("subpageTypeCodes", "keys and values must be non-empty strings"))
            continue
        if code in seen:
            errors.append(path_error(code_path, f"duplicates code {code!r} used by {seen[code]!r}"))
        seen[code] = card_type
        if isinstance(cards, dict) and card_type not in cards:
            errors.append(path_error(code_path, "must also be defined in cards"))


def validate_option_select(data: dict[str, Any], errors: list[str]) -> None:
    option_select = data.get("optionSelect")
    if not isinstance(option_select, dict):
        errors.append(path_error("optionSelect", "must be an object"))
        return

    canonical = option_select.get("canonicalAction")
    actions = option_select.get("actions")
    if not isinstance(canonical, str) or not canonical:
        errors.append(path_error("optionSelect.canonicalAction", "must be a non-empty string"))
    if not isinstance(actions, list) or canonical not in actions:
        errors.append(path_error("optionSelect.actions", "must include optionSelect.canonicalAction"))


def validate_card_groups(data: dict[str, Any], errors: list[str]) -> None:
    groups = data.get("cardGroups")
    if not isinstance(groups, dict):
        errors.append(path_error("cardGroups", "must be an object"))
        return

    if not isinstance(groups.get("brightnessSlider"), list):
        errors.append(path_error("cardGroups.brightnessSlider", "must be a list"))
    fan = groups.get("fan")
    if not isinstance(fan, dict) or not fan:
        errors.append(path_error("cardGroups.fan", "must be a non-empty object"))
    else:
        for card_type, config in fan.items():
            if not isinstance(config, dict) or not isinstance(config.get("defaultIcon"), str):
                errors.append(path_error(f"cardGroups.fan.{card_type}.defaultIcon", "must be a string"))


def validate_icon_registry(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    names: set[str] = set()

    def validate_icon(entry: Any, entry_path: str) -> None:
        if not isinstance(entry, dict):
            errors.append(path_error(entry_path, "must be an object"))
            return
        name = entry.get("name")
        codepoint = entry.get("codepoint")
        mdi = entry.get("mdi")
        if not isinstance(name, str) or not name:
            errors.append(path_error(f"{entry_path}.name", "must be a non-empty string"))
        elif name in names:
            errors.append(path_error(f"{entry_path}.name", f"duplicates {name!r}"))
        else:
            names.add(name)
        if not isinstance(codepoint, str) or not codepoint:
            errors.append(path_error(f"{entry_path}.codepoint", "must be a non-empty string"))
        if not isinstance(mdi, str) or not mdi:
            errors.append(path_error(f"{entry_path}.mdi", "must be a non-empty string"))
        comment = entry.get("comment")
        if comment is not None and not isinstance(comment, str):
            errors.append(path_error(f"{entry_path}.comment", "must be a string"))

    validate_icon(data.get("fallback"), "fallback")

    for section in ("structural", "icons"):
        entries = data.get(section)
        if not isinstance(entries, list) or not entries:
            errors.append(path_error(section, "must be a non-empty list"))
            continue
        for index, entry in enumerate(entries):
            validate_icon(entry, f"{section}[{index}]")

    defaults = data.get("domain_defaults")
    if not isinstance(defaults, dict) or not defaults:
        errors.append(path_error("domain_defaults", "must be a non-empty object"))
    else:
        for domain, icon_name in defaults.items():
            default_path = f"domain_defaults.{domain}"
            if not isinstance(domain, str) or not domain:
                errors.append(path_error("domain_defaults", "keys must be non-empty strings"))
                continue
            if not isinstance(icon_name, str) or not icon_name:
                errors.append(path_error(default_path, "must be a non-empty string"))
            elif icon_name not in names:
                errors.append(path_error(default_path, f"references unknown icon {icon_name!r}"))
    return errors


def validate_compatibility_fixtures(data: dict[str, Any], device_slugs: list[str]) -> list[str]:
    errors: list[str] = []
    current = data.get("current")
    legacy = data.get("legacy-v1")
    if not isinstance(current, dict):
        errors.append(path_error("current", "must be an object"))
    if not isinstance(legacy, dict):
        errors.append(path_error("legacy-v1", "must be an object"))
    if isinstance(current, dict):
        profiles = current.get("deviceProfiles")
        if not isinstance(profiles, dict):
            errors.append(path_error("current.deviceProfiles", "must be an object"))
        else:
            required_slugs = profiles.get("requiredSlugs")
            if required_slugs != device_slugs:
                errors.append(path_error("current.deviceProfiles.requiredSlugs", "must match devices/manifest.json device order"))
    return errors


def assert_card_contract_valid(data: dict[str, Any]) -> None:
    errors = validate_card_contract(data)
    if errors:
        raise ProductSchemaError("Card contract is invalid:\n" + "\n".join(f"  {error}" for error in errors))


def assert_entity_names_valid(data: dict[str, Any]) -> None:
    errors = validate_entity_names(data)
    if errors:
        raise ProductSchemaError("Entity name registry is invalid:\n" + "\n".join(f"  {error}" for error in errors))


def validate_product_sources() -> dict[str, list[str]]:
    results: dict[str, list[str]] = {}
    device_data = load_json(DEVICE_MANIFEST)
    device_slugs = list(device_data.get("devices", {}).keys()) if isinstance(device_data, dict) else []
    results[rel(DEVICE_MANIFEST)] = validate_manifest_data(device_data)
    results[rel(CARD_CONTRACT_JSON)] = validate_card_contract(load_card_contract())
    results[rel(ENTITY_NAMES_JSON)] = validate_entity_names(load_entity_names())
    results[rel(ICONS_JSON)] = validate_icon_registry(load_icon_registry())
    results[rel(COMPATIBILITY_FIXTURES_JSON)] = validate_compatibility_fixtures(load_compatibility_fixtures(), device_slugs)
    return results


def product_snapshot() -> dict[str, Any]:
    """Combined, generated view of the product sources for drift checks."""
    profiles = device_profiles()
    card_contract = load_card_contract()
    entity_names = load_entity_names()
    icon_registry = load_icon_registry()
    compatibility = load_compatibility_fixtures()
    return {
        "schemaVersion": PRODUCT_SNAPSHOT_VERSION,
        "generatedBy": "scripts/check_product_snapshot.py --update",
        "sources": {
            "deviceProfiles": rel(DEVICE_MANIFEST),
            "cardContract": rel(CARD_CONTRACT_JSON),
            "entityNames": rel(ENTITY_NAMES_JSON),
            "icons": rel(ICONS_JSON),
            "compatibilityFixtures": rel(COMPATIBILITY_FIXTURES_JSON),
        },
        "summary": {
            "deviceProfiles": len(profiles),
            "cardTypes": len(card_contract.get("cards", {})),
            "entityNames": len(entity_names.get("entities", [])),
            "icons": len(icon_registry.get("icons", [])),
            "structuralIcons": len(icon_registry.get("structural", [])),
            "compatibilityFixtureGroups": len(compatibility),
        },
        "deviceProfiles": profiles,
        "cardContract": card_contract,
        "entityNames": entity_names,
        "icons": icon_registry,
        "compatibilityFixtures": compatibility,
    }


def product_snapshot_text(snapshot: dict[str, Any] | None = None) -> str:
    return json.dumps(snapshot or product_snapshot(), indent=2) + "\n"


def device_profiles(path: Path = DEVICE_MANIFEST) -> dict[str, dict[str, Any]]:
    """Normalized device profiles for generators and checks."""
    return load_normalized_device_profiles(path)


def slot_devices(path: Path = DEVICE_MANIFEST) -> list[dict[str, Any]]:
    """Device slot-generation profiles derived from the product schema."""
    return load_slot_devices(path)
