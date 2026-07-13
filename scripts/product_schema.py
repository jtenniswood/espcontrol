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
NORMALIZATION_FIELD_POLICIES = {"keep", "clear", "default", "allowed", "alias", "hook"}
NORMALIZATION_CONDITION_OPERATORS = {"equals", "in", "present"}


class ProductSchemaError(RuntimeError):
    pass


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def reject_duplicate_json_keys(path: Path, pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ProductSchemaError(f"{rel(path)} contains duplicate JSON key {key!r}")
        result[key] = value
    return result


def load_json(path: Path) -> Any:
    try:
        with path.open(encoding="utf-8") as f:
            return json.load(f, object_pairs_hook=lambda pairs: reject_duplicate_json_keys(path, pairs))
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
    if data.get("contractVersion") != 1:
        errors.append(path_error("contractVersion", "must be 1"))

    hooks = data.get("normalizationHooks")
    hook_names: set[str] = set()
    if not isinstance(hooks, list) or not all(isinstance(hook, str) and hook for hook in hooks):
        errors.append(path_error("normalizationHooks", "must be a list of non-empty strings"))
    else:
        hook_names = set(hooks)
        if len(hook_names) != len(hooks):
            errors.append(path_error("normalizationHooks", "must not contain duplicates"))

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
                    option_names: dict[str, str] = {}
                    storage_names: dict[str, str] = {}
                    condition_option_names = {
                        name for name in data.get("optionNames", []) if isinstance(name, str)
                    }
                    for candidate in options:
                        if not isinstance(candidate, dict):
                            continue
                        candidate_name = candidate.get("name")
                        if isinstance(candidate_name, str):
                            condition_option_names.add(candidate_name)
                        candidate_storage = candidate.get("storage", [])
                        if isinstance(candidate_storage, list):
                            condition_option_names.update(
                                name for name in candidate_storage if isinstance(name, str)
                            )
                    for idx, option in enumerate(options):
                        option_path = f"{card_path}.options[{idx}]"
                        if not isinstance(option, dict):
                            errors.append(path_error(option_path, "must be an object"))
                            continue
                        name = option.get("name")
                        if not isinstance(name, str) or not name:
                            errors.append(path_error(f"{option_path}.name", "must be a non-empty string"))
                        elif name in option_names:
                            errors.append(path_error(f"{option_path}.name", f"duplicates {option_names[name]}"))
                        else:
                            option_names[name] = f"{option_path}.name"
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
                        names = option.get("storage", [name])
                        if isinstance(names, list):
                            for storage_name in names:
                                if not isinstance(storage_name, str) or not storage_name:
                                    errors.append(path_error(f"{option_path}.storage", "must contain non-empty strings"))
                                elif storage_name in storage_names:
                                    errors.append(path_error(f"{option_path}.storage", f"duplicates storage name used by {storage_names[storage_name]}"))
                                else:
                                    storage_names[storage_name] = option_path
                        if "defaultValue" in option and not isinstance(option.get("defaultValue"), str):
                            errors.append(path_error(f"{option_path}.defaultValue", "must be a string"))
                        if option.get("kind") in {"choice", "number"} and "defaultValue" not in option:
                            errors.append(path_error(f"{option_path}.defaultValue", "is required for choice and number options"))
                        if "omitDefault" in option and not isinstance(option.get("omitDefault"), bool):
                            errors.append(path_error(f"{option_path}.omitDefault", "must be a boolean"))
                        if "storageField" in option and option.get("storageField") not in SAVED_CONFIG_FIELDS:
                            errors.append(path_error(f"{option_path}.storageField", "must be a saved config field"))
                        aliases = option.get("aliases")
                        if aliases is not None and (
                            not isinstance(aliases, dict)
                            or not all(isinstance(key, str) and isinstance(value, str) for key, value in aliases.items())
                        ):
                            errors.append(path_error(f"{option_path}.aliases", "must be an object of strings"))
                        validate_normalization_conditions(
                            option.get("applicability"),
                            f"{option_path}.applicability",
                            errors,
                            condition_option_names,
                        )
                        applicability_hook = option.get("applicabilityHook")
                        if applicability_hook is not None and applicability_hook not in hook_names:
                            errors.append(path_error(f"{option_path}.applicabilityHook", "must name an allow-listed normalization hook"))
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

            normalization = card.get("normalization")
            if normalization is not None:
                validate_card_normalization(
                    card_path,
                    normalization,
                    options if isinstance(options, list) else [],
                    hook_names,
                    data.get("migrationActions"),
                    errors,
                )

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
            target_type = target.get("type")
            if isinstance(target_type, str) and isinstance(cards, dict) and target_type not in cards:
                errors.append(path_error(f"{alias_path}.type", "must reference a card defined in cards"))

    global_option_names = {
        name for name in data.get("optionNames", []) if isinstance(name, str)
    }
    if isinstance(cards, dict):
        for card in cards.values():
            if not isinstance(card, dict):
                continue
            for option in card.get("options", []):
                if not isinstance(option, dict):
                    continue
                name = option.get("name")
                if isinstance(name, str):
                    global_option_names.add(name)
                storage = option.get("storage", [])
                if isinstance(storage, list):
                    global_option_names.update(item for item in storage if isinstance(item, str))
    validate_migration_actions(data.get("migrationActions"), hook_names, global_option_names, errors)

    validate_subpage_type_codes(data, cards, errors)
    validate_option_select(data, errors)
    validate_card_groups(data, errors)

    large = data.get("largeNumbers")
    if not isinstance(large, dict):
        errors.append(path_error("largeNumbers", "must be an object"))
    return errors


def validate_normalization_conditions(
    value: Any,
    path: str,
    errors: list[str],
    allowed_option_names: set[str] | None = None,
) -> None:
    if value is None:
        return
    if not isinstance(value, list):
        errors.append(path_error(path, "must be a list"))
        return
    for idx, condition in enumerate(value):
        condition_path = f"{path}[{idx}]"
        if not isinstance(condition, dict):
            errors.append(path_error(condition_path, "must be an object"))
            continue
        source = condition.get("source")
        name = condition.get("name")
        operator = condition.get("operator")
        if source not in {"field", "option"}:
            errors.append(path_error(f"{condition_path}.source", "must be field or option"))
        if not isinstance(name, str) or not name:
            errors.append(path_error(f"{condition_path}.name", "must be a non-empty string"))
        elif source == "field" and name not in SAVED_CONFIG_FIELDS:
            errors.append(path_error(f"{condition_path}.name", "must be a saved config field"))
        elif source == "option" and allowed_option_names is not None and name not in allowed_option_names:
            errors.append(path_error(f"{condition_path}.name", "must reference a declared option storage name"))
        if operator not in NORMALIZATION_CONDITION_OPERATORS:
            errors.append(path_error(f"{condition_path}.operator", "must be equals, in, or present"))
        condition_value = condition.get("value")
        if operator == "equals" and not isinstance(condition_value, str):
            errors.append(path_error(f"{condition_path}.value", "must be a string for equals"))
        if operator == "in" and (
            not isinstance(condition_value, list)
            or not condition_value
            or not all(isinstance(item, str) for item in condition_value)
        ):
            errors.append(path_error(f"{condition_path}.value", "must be a non-empty list of strings for in"))
        if operator == "present" and "value" in condition:
            errors.append(path_error(f"{condition_path}.value", "must be omitted for present"))
        if "negate" in condition and not isinstance(condition.get("negate"), bool):
            errors.append(path_error(f"{condition_path}.negate", "must be a boolean"))


def validate_card_normalization(
    card_path: str,
    normalization: Any,
    options: list[Any],
    hook_names: set[str],
    migration_actions: Any,
    errors: list[str],
) -> None:
    path = f"{card_path}.normalization"
    if not isinstance(normalization, dict):
        errors.append(path_error(path, "must be an object"))
        return
    fields = normalization.get("fields")
    if not isinstance(fields, dict) or set(fields) != set(SAVED_CONFIG_FIELDS):
        errors.append(path_error(f"{path}.fields", "must define every saved config field exactly once"))
    else:
        for field, rule in fields.items():
            rule_path = f"{path}.fields.{field}"
            if not isinstance(rule, dict):
                errors.append(path_error(rule_path, "must be an object"))
                continue
            policy = rule.get("policy")
            if policy not in NORMALIZATION_FIELD_POLICIES:
                errors.append(path_error(f"{rule_path}.policy", "must be keep, clear, default, allowed, alias, or hook"))
                continue
            if policy == "default" and not isinstance(rule.get("value"), str):
                errors.append(path_error(f"{rule_path}.value", "must be a string for default"))
            if policy == "allowed":
                values = rule.get("values")
                if not isinstance(values, list) or not values or not all(isinstance(item, str) for item in values):
                    errors.append(path_error(f"{rule_path}.values", "must be a non-empty list of strings for allowed"))
                if not isinstance(rule.get("fallback"), str):
                    errors.append(path_error(f"{rule_path}.fallback", "must be a string for allowed"))
                aliases = rule.get("aliases")
                if aliases is not None:
                    if not isinstance(aliases, dict) or not aliases or not all(
                        isinstance(key, str) and isinstance(value, str) for key, value in aliases.items()
                    ):
                        errors.append(path_error(f"{rule_path}.aliases", "must be a non-empty object of strings for allowed"))
                    elif isinstance(values, list) and any(target not in values for target in aliases.values()):
                        errors.append(path_error(f"{rule_path}.aliases", "must map to an allowed value"))
            if policy == "alias":
                aliases = rule.get("aliases")
                if not isinstance(aliases, dict) or not aliases or not all(
                    isinstance(key, str) and isinstance(value, str) for key, value in aliases.items()
                ):
                    errors.append(path_error(f"{rule_path}.aliases", "must be a non-empty object of strings for alias"))
            if policy == "hook":
                hook = rule.get("hook")
                if hook not in hook_names:
                    errors.append(path_error(f"{rule_path}.hook", "must name an allow-listed normalization hook"))

    unknown = normalization.get("unknownOptions")
    if unknown != "drop":
        errors.append(path_error(f"{path}.unknownOptions", "must be drop for the compatibility rollout"))

    required_storage_names: set[str] = set()
    for option in options:
        if not isinstance(option, dict):
            continue
        if not isinstance(option.get("omitDefault"), bool):
            errors.append(path_error(f"{path}.options.{option.get('name', '<unknown>')}", "must declare omitDefault"))
        names = option.get("storage", [option.get("name")])
        if isinstance(names, list):
            valid_names = {name for name in names if isinstance(name, str)}
            if "storageField" not in option and option.get("migration") != "drop":
                required_storage_names.update(valid_names)
    order = normalization.get("canonicalOptionOrder")
    if not isinstance(order, list) or not all(isinstance(name, str) for name in order):
        errors.append(path_error(f"{path}.canonicalOptionOrder", "must be a list of strings"))
    else:
        if len(order) != len(set(order)):
            errors.append(path_error(f"{path}.canonicalOptionOrder", "must not contain duplicates"))
        unknown_names = sorted(set(order) - required_storage_names)
        if unknown_names:
            errors.append(path_error(f"{path}.canonicalOptionOrder", f"contains unknown storage names: {', '.join(unknown_names)}"))
        missing_names = sorted(required_storage_names - set(order))
        if missing_names:
            errors.append(path_error(f"{path}.canonicalOptionOrder", f"omits storage names: {', '.join(missing_names)}"))

    hook = normalization.get("optionHook")
    if hook is not None and hook not in hook_names:
        errors.append(path_error(f"{path}.optionHook", "must name an allow-listed normalization hook"))

    actions = normalization.get("migrationActions", [])
    if not isinstance(actions, list) or not all(isinstance(name, str) for name in actions):
        errors.append(path_error(f"{path}.migrationActions", "must be a list of strings"))
    elif isinstance(migration_actions, dict):
        for name in actions:
            if name not in migration_actions:
                errors.append(path_error(f"{path}.migrationActions", f"references missing action {name!r}"))

    hook_data = normalization.get("hookData", {})
    if not isinstance(hook_data, dict):
        errors.append(path_error(f"{path}.hookData", "must be an object"))
    else:
        unknown_hook_data = sorted(set(hook_data) - hook_names)
        if unknown_hook_data:
            errors.append(path_error(f"{path}.hookData", f"contains hooks outside the allow-list: {', '.join(unknown_hook_data)}"))
        vacuum = hook_data.get("normalize_vacuum_fields")
        if vacuum is not None:
            if not isinstance(vacuum, dict):
                errors.append(path_error(f"{path}.hookData.normalize_vacuum_fields", "must be an object"))
            else:
                modes = vacuum.get("preserveUnitForModes")
                icons = vacuum.get("defaultIcons")
                if not isinstance(modes, list) or not all(isinstance(mode, str) and mode for mode in modes):
                    errors.append(path_error(f"{path}.hookData.normalize_vacuum_fields.preserveUnitForModes", "must be a list of non-empty strings"))
                if not isinstance(icons, dict) or not all(isinstance(mode, str) and isinstance(icon, str) and icon for mode, icon in icons.items()):
                    errors.append(path_error(f"{path}.hookData.normalize_vacuum_fields.defaultIcons", "must be an object of non-empty strings"))
                elif "default" not in icons:
                    errors.append(path_error(f"{path}.hookData.normalize_vacuum_fields.defaultIcons", "must include default"))


def validate_migration_actions(
    value: Any,
    hook_names: set[str],
    option_names: set[str],
    errors: list[str],
) -> None:
    if not isinstance(value, dict):
        errors.append(path_error("migrationActions", "must be an object"))
        return
    for name, action in value.items():
        path = f"migrationActions.{name}"
        if not isinstance(name, str) or not name or not isinstance(action, dict):
            errors.append(path_error("migrationActions", "keys must map to objects"))
            continue
        validate_normalization_conditions(action.get("when"), f"{path}.when", errors, option_names)
        updates = action.get("set", {})
        if not isinstance(updates, dict) or not updates:
            errors.append(path_error(f"{path}.set", "must be a non-empty object"))
        else:
            for field, field_value in updates.items():
                if field not in SAVED_CONFIG_FIELDS:
                    errors.append(path_error(f"{path}.set.{field}", "must be a saved config field"))
                if not isinstance(field_value, str):
                    errors.append(path_error(f"{path}.set.{field}", "must be a string"))
        hook = action.get("hook")
        if hook is not None and hook not in hook_names:
            errors.append(path_error(f"{path}.hook", "must name an allow-listed normalization hook"))


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

    retired = data.get("retiredSubpageTypeCodes")
    if not isinstance(retired, list) or not all(isinstance(code, str) and code for code in retired):
        errors.append(path_error("retiredSubpageTypeCodes", "must be a list of non-empty strings"))
    else:
        if len(retired) != len(set(retired)):
            errors.append(path_error("retiredSubpageTypeCodes", "must not contain duplicates"))
        reused = sorted(set(retired) & set(seen))
        if reused:
            errors.append(path_error("retiredSubpageTypeCodes", f"reuses active codes: {', '.join(reused)}"))


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
