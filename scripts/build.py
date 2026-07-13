#!/usr/bin/env python3
"""Unified build script for espcontrol.

Combines icon synchronization and www.js generation into a single tool.

Usage:
    python scripts/build.py               # run all generators
    python scripts/build.py --check       # exit 1 if any output is stale
    python scripts/build.py devices       # sync public device capabilities only
    python scripts/build.py icons         # sync icons only
    python scripts/build.py i18n          # sync firmware translations only
    python scripts/build.py www           # build www.js only
    python scripts/build.py www --temporary-output DIR  # isolated fresh bundles
    python scripts/build.py icons --check # check icons only
"""
import json
import re
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path

from device_profiles import load_device_profiles, public_device_capabilities, web_config
from check_timezones import AUTO_TIMEZONE_OPTION, load_timezone_select_options, timezone_option_id
from product_schema import (
    ProductSchemaError,
    assert_card_contract_valid,
    assert_entity_names_valid as assert_product_entity_names_valid,
)

ROOT = Path(__file__).resolve().parent.parent
MDI_VERSION = "7.4.47"
MDI_CSS_URL = f"https://cdn.jsdelivr.net/npm/@mdi/font@{MDI_VERSION}/css/materialdesignicons.css"

# ---------------------------------------------------------------------------
# Shared paths
# ---------------------------------------------------------------------------
ICONS_JSON = ROOT / "common" / "assets" / "icons.json"
ENTITY_NAMES_JSON = ROOT / "common" / "config" / "entity_names.json"
ENTITY_NAMES_YAML = ROOT / "common" / "config" / "entity_names.yaml"
ENTITY_NAMES_TS = ROOT / "src" / "webserver" / "generated" / "entity_catalog.ts"
WEB_ICONS_TS = ROOT / "src" / "webserver" / "generated" / "icons.ts"
STRINGS_DIR = ROOT / "common" / "config"
I18N_GENERATED_H = ROOT / "components" / "espcontrol" / "i18n_generated.h"
CARD_CONTRACT_JSON = ROOT / "common" / "config" / "card_contract.json"
CARD_CONTRACT_TS = ROOT / "src" / "webserver" / "generated" / "card_contract.ts"
CARD_CONTRACT_H = ROOT / "components" / "espcontrol" / "button_grid_contract_generated.h"
SAVED_CONFIG_SHADOW_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_shadow.ts"
SAVED_CONFIG_SHADOW_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_shadow_generated.h"
CARD_DOCS_DIR = ROOT / "docs" / "generated" / "cards"
DEVICE_CAPABILITIES_JSON = ROOT / "docs" / "public" / "device-profiles.json"
DEVICE_DOCS_DIR = ROOT / "docs" / "generated" / "screens"


class BuildError(RuntimeError):
    pass


def load_json(path):
    with open(path) as f:
        return json.load(f)


def load_entity_names_data():
    return load_json(ENTITY_NAMES_JSON)


def load_card_contract_data():
    return load_json(CARD_CONTRACT_JSON)


def replace_between_markers(text, start_tag, end_tag, new_content):
    """Replace content between marker lines, preserving the markers themselves."""
    pattern = re.compile(
        r"(^[^\n]*" + re.escape(start_tag) + r"[^\n]*\n)"
        r"(.*?)"
        r"(^[^\n]*" + re.escape(end_tag) + r"[^\n]*$)",
        re.MULTILINE | re.DOTALL,
    )
    m = pattern.search(text)
    if not m:
        raise ValueError(f"Markers not found: {start_tag} / {end_tag}")
    return text[: m.start(2)] + new_content + text[m.start(3) :]


# ===========================================================================
# Entity name sync
# ===========================================================================

def entity_name_entries(data):
    entries = data.get("entities")
    if not isinstance(entries, list):
        raise BuildError(f"{ENTITY_NAMES_JSON.relative_to(ROOT)} must contain an entities list")
    return entries


def validate_entity_names(data):
    errors = []
    keys = set()
    names_by_domain = {}
    for index, entry in enumerate(entity_name_entries(data)):
        key = entry.get("key")
        domain = entry.get("domain")
        name = entry.get("name")
        template = entry.get("template")
        if not isinstance(key, str) or not key:
            errors.append(f"entry {index + 1} has a missing key")
            continue
        if key in keys:
            errors.append(f"duplicate key {key!r}")
        keys.add(key)
        if not isinstance(domain, str) or not domain:
            errors.append(f"{key}: missing domain")
        if bool(name) == bool(template):
            errors.append(f"{key}: define exactly one of name or template")
        if template and "{slot}" not in template:
            errors.append(f"{key}: template must contain {{slot}}")
        value = name or template
        if isinstance(value, str):
            names_by_domain.setdefault(domain, {}).setdefault(value, []).append(key)
        object_ids = entry.get("objectIds", [])
        if object_ids and (
            not isinstance(object_ids, list)
            or not all(isinstance(v, str) and v for v in object_ids)
        ):
            errors.append(f"{key}: objectIds must be a list of strings")
        elif isinstance(object_ids, list) and len(object_ids) != len(set(object_ids)):
            errors.append(f"{key}: objectIds must not contain duplicate values")
        groups = entry.get("groups", [])
        if groups and (not isinstance(groups, list) or not all(isinstance(v, str) and v for v in groups)):
            errors.append(f"{key}: groups must be a list of strings")

    for domain, names in names_by_domain.items():
        for name, entry_keys in names.items():
            if len(entry_keys) > 1:
                errors.append(f"duplicate entity name for {domain} {name!r}: {', '.join(entry_keys)}")
    return errors


def assert_entity_names_valid(data):
    try:
        assert_product_entity_names_valid(data)
    except ProductSchemaError as exc:
        raise BuildError(str(exc)) from exc


def yaml_quote(value):
    return json.dumps(value)


def split_slot_template(template):
    before, after = template.split("{slot}", 1)
    return before, after


def gen_entity_names_yaml(data):
    lines = [
        "# =============================================================================\n",
        "# GENERATED ENTITY NAMES - do not edit by hand\n",
        "# Generated by scripts/build.py from common/config/entity_names.json.\n",
        "# =============================================================================\n",
        "\n",
        "substitutions:\n",
    ]
    for entry in entity_name_entries(data):
        key = entry["key"]
        if "name" in entry:
            lines.append(f"  entity_{key}: {yaml_quote(entry['name'])}\n")
        else:
            before, after = split_slot_template(entry["template"])
            lines.append(f"  entity_{key}_prefix: {yaml_quote(before)}\n")
            lines.append(f"  entity_{key}_suffix: {yaml_quote(after)}\n")
    return "".join(lines)


def gen_entity_names_js(data):
    entities = {}
    groups = {}
    for entry in entity_name_entries(data):
        key = entry["key"]
        entity = {"domain": entry["domain"]}
        if "name" in entry:
            entity["name"] = entry["name"]
        else:
            entity["template"] = entry["template"]
        if entry.get("objectIds"):
            entity["objectIds"] = entry["objectIds"]
        entities[key] = entity
        for group in entry.get("groups", []):
            groups.setdefault(group, []).append(key)

    payload = {
        "entities": entities,
        "groups": groups,
    }
    json_text = json.dumps(payload, indent=2)
    return (
        "// =============================================================================\n"
        "// GENERATED ENTITY CATALOG - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/entity_names.json.\n"
        "// =============================================================================\n"
        f"export const ENTITY_CATALOG = {json_text} as const;\n"
    )


def sync_entity_names(check_only=False):
    data = load_entity_names_data()
    assert_entity_names_valid(data)
    outputs = [
        (ENTITY_NAMES_YAML, gen_entity_names_yaml(data)),
        (ENTITY_NAMES_TS, gen_entity_names_js(data)),
    ]
    dirty = []
    for path, content in outputs:
        if not path.exists() or path.read_text() != content:
            dirty.append(path.relative_to(ROOT))

    if check_only:
        if dirty:
            print("Entity name outputs are out of sync. Run 'python scripts/build.py entities' to fix:")
            for rel in dirty:
                print(f"  {rel}")
        return dirty

    for path, content in outputs:
        if path.exists() and path.read_text() == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        print(f"  updated {path.relative_to(ROOT)}")
    return dirty


# ===========================================================================
# Firmware i18n generation
# ===========================================================================

def unescape_compact_string(value):
    out = []
    i = 0
    while i < len(value):
        ch = value[i]
        if ch != "\\" or i + 1 >= len(value):
            out.append(ch)
            i += 1
            continue
        nxt = value[i + 1]
        if nxt == "n":
            out.append("\n")
        elif nxt in {"\\", "="}:
            out.append(nxt)
        else:
            out.append(nxt)
        i += 2
    return "".join(out)


def load_compact_strings(path):
    strings = {}
    key_lines = {}
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise BuildError(f"Invalid strings entry in {path.relative_to(ROOT)}:{line_no}")
        key, value = line.split("=", 1)
        key = key.strip()
        if not key:
            raise BuildError(f"Empty strings key in {path.relative_to(ROOT)}:{line_no}")
        if key in key_lines:
            raise BuildError(
                f"Duplicate strings key {key!r} in {path.relative_to(ROOT)}:"
                f"{line_no} (first defined on line {key_lines[key]})"
            )
        key_lines[key] = line_no
        strings[key] = unescape_compact_string(value)
    return strings


def cpp_string(value):
    return json.dumps(value, ensure_ascii=False)


def gen_i18n_header():
    english_path = STRINGS_DIR / "strings.en.txt"
    if not english_path.exists():
        raise BuildError(f"Missing {english_path.relative_to(ROOT)}")
    english = load_compact_strings(english_path)
    language_files = sorted(STRINGS_DIR.glob("strings.*.txt"))
    languages = []
    for path in language_files:
        code = path.stem.split(".", 1)[1]
        if code == "en":
            continue
        translated = load_compact_strings(path)
        missing = [key for key in english if key not in translated]
        extra = [key for key in translated if key not in english]
        if missing or extra:
            raise BuildError(
                f"{path.relative_to(ROOT)} keys do not match strings.en.txt "
                f"(missing={missing[:5]}, extra={extra[:5]})"
            )
        languages.append((code, translated))

    lines = [
        "// =============================================================================",
        "// GENERATED FIRMWARE I18N - do not edit by hand",
        "// Generated by scripts/build.py from common/config/strings.*.txt.",
        "// =============================================================================",
        "#pragma once",
        "#include <cstring>",
        "#include <string>",
        "",
        "inline std::string &espcontrol_language_code() {",
        "  static std::string language = \"en\";",
        "  return language;",
        "}",
        "",
        "inline void set_espcontrol_language(const std::string &language) {",
        "  espcontrol_language_code() = language;",
        "}",
        "",
    ]

    for code, translated in languages:
        fn = re.sub(r"[^A-Za-z0-9_]", "_", code)
        lines.extend([
            f"inline const char *espcontrol_i18n_{fn}(const char *text) {{",
            "  if (!text) return \"\";",
        ])
        seen_sources = set()
        for key, source in english.items():
            if source in seen_sources:
                continue
            seen_sources.add(source)
            target = translated[key]
            if target == source:
                continue
            lines.append(f"  if (std::strcmp(text, {cpp_string(source)}) == 0) return {cpp_string(target)};")
        lines.extend([
            "  return text;",
            "}",
            "",
        ])

    lines.extend([
        "inline const char *espcontrol_i18n_key_en(const char *key) {",
        "  if (!key) return \"\";",
    ])
    for key, source in english.items():
        lines.append(f"  if (std::strcmp(key, {cpp_string(key)}) == 0) return {cpp_string(source)};")
    lines.extend([
        "  return key;",
        "}",
        "",
    ])

    for code, translated in languages:
        fn = re.sub(r"[^A-Za-z0-9_]", "_", code)
        lines.extend([
            f"inline const char *espcontrol_i18n_key_{fn}(const char *key) {{",
            "  if (!key) return \"\";",
        ])
        for key, target in translated.items():
            source = english[key]
            if target == source:
                continue
            lines.append(f"  if (std::strcmp(key, {cpp_string(key)}) == 0) return {cpp_string(target)};")
        lines.extend([
            "  return espcontrol_i18n_key_en(key);",
            "}",
            "",
        ])

    lines.extend([
        "inline const char *espcontrol_i18n(const char *text) {",
        "  if (!text) return \"\";",
    ])
    for code, _translated in languages:
        fn = re.sub(r"[^A-Za-z0-9_]", "_", code)
        lines.append(f"  if (espcontrol_language_code() == {cpp_string(code)}) return espcontrol_i18n_{fn}(text);")
    lines.extend([
        "  return text;",
        "}",
        "",
        "inline std::string espcontrol_i18n(const std::string &text) {",
        "  return std::string(espcontrol_i18n(text.c_str()));",
        "}",
        "",
        "inline const char *espcontrol_i18n_key(const char *key) {",
        "  if (!key) return \"\";",
    ])
    for code, _translated in languages:
        fn = re.sub(r"[^A-Za-z0-9_]", "_", code)
        lines.append(f"  if (espcontrol_language_code() == {cpp_string(code)}) return espcontrol_i18n_key_{fn}(key);")
    lines.extend([
        "  return espcontrol_i18n_key_en(key);",
        "}",
        "",
        "inline std::string espcontrol_i18n_key(const std::string &key) {",
        "  return std::string(espcontrol_i18n_key(key.c_str()));",
        "}",
        "",
    ])
    return "\n".join(lines)


def sync_i18n(check_only=False):
    generated = gen_i18n_header()
    dirty = []
    if not I18N_GENERATED_H.exists() or I18N_GENERATED_H.read_text(encoding="utf-8") != generated:
        dirty.append(I18N_GENERATED_H.relative_to(ROOT))

    if check_only:
        if dirty:
            print("Firmware i18n output is out of sync. Run 'python scripts/build.py i18n' to fix:")
            for rel in dirty:
                print(f"  {rel}")
        return dirty

    if dirty:
        I18N_GENERATED_H.parent.mkdir(parents=True, exist_ok=True)
        I18N_GENERATED_H.write_text(generated, encoding="utf-8")
        print(f"  updated {I18N_GENERATED_H.relative_to(ROOT)}")
    return dirty


# ===========================================================================
# Card config contract generation
# ===========================================================================

def js_string_list(values):
    return "[" + ", ".join(json.dumps(v) for v in values) + "]"


def contract_option_names(data):
    names = {}
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


def option_constant_name(option_name):
    return "CARD_CONTRACT_OPTION_NAME_" + re.sub(r"[^A-Za-z0-9]+", "_", option_name).strip("_").upper()


def gen_card_contract_ts(data):
    groups = data["cardGroups"]
    fan = groups["fan"]
    fan_default_icons = {card_type: cfg["defaultIcon"] for card_type, cfg in fan.items()}
    fan_default_icon_on = {card_type: cfg["defaultIconOn"] for card_type, cfg in fan.items() if cfg.get("defaultIconOn")}
    codes = data["subpageTypeCodes"]
    code_to_type = {code: card_type for card_type, code in codes.items()}
    large = data["largeNumbers"]
    cards = json.loads(json.dumps(data["cards"]))
    # Hook implementation data is only needed by generated shadow helpers. Keep
    # it out of the production browser bundle until a family is switched over.
    for card in cards.values():
        normalization = card.get("normalization")
        if normalization:
            normalization.pop("hookData", None)
    aliases = data.get("migrationAliases", {})
    option_names = contract_option_names(data)
    return (
        "// =============================================================================\n"
        "// GENERATED TYPED CARD CONFIG CONTRACT - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// =============================================================================\n"
        "import type { CardConfig, CardOptionSpec, CardTypeSpec, MigrationActionSpec, SavedConfigField } from \"../contracts/types\";\n"
        "\n"
        "type LargeNumbersRule = true | {\n"
        "  readonly precisions?: readonly string[];\n"
        "  readonly excludedPrecisions?: readonly string[];\n"
        "};\n"
        "\n"
        f"export const CARD_CONTRACT_VERSION = {int(data['contractVersion'])} as const;\n"
        f"export const CARD_CONTRACT_NORMALIZATION_HOOKS = {js_string_list(data['normalizationHooks'])} as const;\n"
        f"export const CARD_CONTRACT_MIGRATION_ACTIONS: Readonly<Record<string, MigrationActionSpec>> = {json.dumps(data['migrationActions'], indent=2)};\n"
        f"export const CARD_CONTRACT_RETIRED_SUBPAGE_TYPE_CODES = {js_string_list(data['retiredSubpageTypeCodes'])} as const;\n"
        f"export const CARD_CONFIG_FIELDS = {json.dumps(data['fields'])} as const satisfies readonly SavedConfigField[];\n"
        f"export const CARD_CONTRACT_CARDS: Readonly<Record<string, CardTypeSpec>> = {json.dumps(cards, indent=2)};\n"
        f"export const CARD_CONTRACT_MIGRATION_ALIASES: Readonly<Record<string, Partial<CardConfig>>> = {json.dumps(aliases, indent=2)};\n"
        f"export const CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES = {js_string_list(groups['brightnessSlider'])} as const;\n"
        f"export const CARD_CONTRACT_FAN_DEFAULT_ICONS: Readonly<Record<string, string>> = {json.dumps(fan_default_icons, indent=2)};\n"
        f"export const CARD_CONTRACT_FAN_DEFAULT_ICON_ON: Readonly<Record<string, string>> = {json.dumps(fan_default_icon_on, indent=2)};\n"
        f"export const CARD_CONTRACT_OPTION_SELECT_ACTION = {json.dumps(data['optionSelect']['canonicalAction'])};\n"
        f"export const CARD_CONTRACT_OPTION_SELECT_ACTIONS = {js_string_list(data['optionSelect']['actions'])} as const;\n"
        f"export const CARD_CONTRACT_SUBPAGE_TYPE_CODES: Readonly<Record<string, string>> = {json.dumps(codes, indent=2)};\n"
        f"export const CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE: Readonly<Record<string, string>> = {json.dumps(code_to_type, indent=2)};\n"
        f"export const CARD_CONTRACT_LARGE_NUMBERS: Readonly<Record<string, LargeNumbersRule>> = {json.dumps(large, indent=2)};\n"
        f"export const CARD_CONTRACT_OPTION_NAMES: Readonly<Record<string, string>> = {json.dumps(option_names, indent=2)};\n"
        "\n"
        "function cardContractListContains(list: readonly string[] | undefined, value: string): boolean {\n"
        "  return (list || []).indexOf(value) >= 0;\n"
        "}\n"
        "\n"
        "export function cardContractCard(type: string | null | undefined): CardTypeSpec | null {\n"
        "  return CARD_CONTRACT_CARDS[type || \"\"] || null;\n"
        "}\n"
        "\n"
        "export function cardContractCardKeys(): string[] {\n"
        "  return Object.keys(CARD_CONTRACT_CARDS);\n"
        "}\n"
        "\n"
        "export function cardContractCardLabel(type: string | null | undefined): string {\n"
        "  const card = cardContractCard(type);\n"
        "  return card ? card.label : (type || \"Switch\");\n"
        "}\n"
        "\n"
        "export function cardContractAllowInSubpage(type: string | null | undefined): boolean {\n"
        "  const card = cardContractCard(type);\n"
        "  return !!(card && card.allowInSubpage);\n"
        "}\n"
        "\n"
        "export function cardContractPickerKey(type: string | null | undefined): string {\n"
        "  const card = cardContractCard(type);\n"
        "  return card && card.pickerKey ? card.pickerKey : \"\";\n"
        "}\n"
        "\n"
        "export function cardContractHidden(type: string | null | undefined): boolean {\n"
        "  const card = cardContractCard(type);\n"
        "  return !!(card && card.hidden);\n"
        "}\n"
        "\n"
        "export function cardContractOptions(type: string | null | undefined): CardOptionSpec[] {\n"
        "  const card = cardContractCard(type);\n"
        "  return card && card.options ? JSON.parse(JSON.stringify(card.options)) as CardOptionSpec[] : [];\n"
        "}\n"
        "\n"
        "export function cardContractDefaultConfig(type: string | null | undefined): CardConfig {\n"
        "  const card = cardContractCard(type);\n"
        "  const defaults = card && card.default ? card.default : CARD_CONTRACT_CARDS[\"\"]!.default;\n"
        "  return Object.assign({}, defaults);\n"
        "}\n"
        "\n"
        "export function cardContractDomains(type: string | null | undefined): string[] {\n"
        "  const card = cardContractCard(type);\n"
        "  return card && card.domains ? card.domains.slice() : [];\n"
        "}\n"
        "\n"
        "export function cardContractMigrationAlias(type: string | null | undefined): Partial<CardConfig> | null {\n"
        "  const alias = CARD_CONTRACT_MIGRATION_ALIASES[type || \"\"];\n"
        "  return alias ? Object.assign({}, alias) : null;\n"
        "}\n"
        "\n"
        "export function cardContractIsBrightnessSliderType(type: string): boolean {\n"
        "  return cardContractListContains(CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES, type);\n"
        "}\n"
        "\n"
        "export function cardContractIsFanCardType(type: string | null | undefined): boolean {\n"
        "  return Object.prototype.hasOwnProperty.call(CARD_CONTRACT_FAN_DEFAULT_ICONS, type || \"\");\n"
        "}\n"
        "\n"
        "export function cardContractFanDefaultIcon(type: string): string {\n"
        "  return CARD_CONTRACT_FAN_DEFAULT_ICONS[type] || CARD_CONTRACT_FAN_DEFAULT_ICONS.fan_speed || \"Fan Speed 2\";\n"
        "}\n"
        "\n"
        "export function cardContractFanDefaultIconOn(type: string): string {\n"
        "  return CARD_CONTRACT_FAN_DEFAULT_ICON_ON[type] || \"Auto\";\n"
        "}\n"
        "\n"
        "export function cardContractIsOptionSelectType(type: string): boolean {\n"
        "  return type === \"option_select\";\n"
        "}\n"
        "\n"
        "export function cardContractIsOptionSelectAction(action: string): boolean {\n"
        "  return cardContractListContains(CARD_CONTRACT_OPTION_SELECT_ACTIONS, action);\n"
        "}\n"
        "\n"
        "export function cardContractSubpageTypeCode(type: string | null | undefined): string {\n"
        "  return CARD_CONTRACT_SUBPAGE_TYPE_CODES[type || \"\"] || (type || \"\");\n"
        "}\n"
        "\n"
        "export function cardContractSubpageTypeFromCode(code: string | null | undefined): string {\n"
        "  return CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE[code || \"\"] || (code || \"\");\n"
        "}\n"
        "\n"
        "export function cardContractLargeNumbersSupported(type: string | null | undefined, precision: string): boolean {\n"
        "  const rule = CARD_CONTRACT_LARGE_NUMBERS[type || \"\"];\n"
        "  if (rule === true) return true;\n"
        "  if (!rule) return false;\n"
        "  if (rule.excludedPrecisions) return !cardContractListContains(rule.excludedPrecisions, precision || \"\");\n"
        "  if (rule.precisions) return cardContractListContains(rule.precisions, precision || \"\");\n"
        "  return false;\n"
        "}\n"
        "\n"
        "export function cardContractOptionName(name: string | null | undefined): string {\n"
        "  return CARD_CONTRACT_OPTION_NAMES[name || \"\"] || name || \"\";\n"
        "}\n"
    )


def shadow_pilot_policies(data):
    pilot_types = ("action", "sensor", "media", "vacuum")
    return {
        card_type: data["cards"][card_type]["normalization"]
        for card_type in pilot_types
    }


def gen_saved_config_shadow_ts(data):
    policies = shadow_pilot_policies(data)
    migrations = {
        name: data["migrationActions"][name]
        for name in policies["vacuum"].get("migrationActions", [])
    }
    return (
        "// =============================================================================\n"
        "// GENERATED SAVED-CONFIG SHADOW HELPERS - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// Test-only shadow code: production normalization does not call this module.\n"
        "// =============================================================================\n"
        "import type { CardConfig, CardNormalizationSpec, MigrationActionSpec, NormalizationCondition } from \"../contracts/types\";\n"
        "\n"
        f"export const SAVED_CONFIG_SHADOW_PILOT_POLICIES: Readonly<Record<string, CardNormalizationSpec>> = {json.dumps(policies, indent=2)};\n"
        f"const VACUUM_MIGRATIONS: Readonly<Record<string, MigrationActionSpec>> = {json.dumps(migrations, indent=2)};\n"
        "\n"
        "function conditionMatches(config: CardConfig, condition: NormalizationCondition): boolean {\n"
        "  const actual = condition.source === \"field\" ? config[condition.name as keyof CardConfig] : \"\";\n"
        "  let matches = false;\n"
        "  if (condition.operator === \"present\") matches = !!actual;\n"
        "  else if (condition.operator === \"equals\") matches = actual === condition.value;\n"
        "  else if (condition.operator === \"in\") matches = Array.isArray(condition.value) && condition.value.indexOf(actual) >= 0;\n"
        "  return condition.negate ? !matches : matches;\n"
        "}\n"
        "\n"
        "function shaped(input: Partial<CardConfig>): CardConfig {\n"
        "  return {\n"
        "    entity: input.entity || \"\", label: input.label || \"\", icon: input.icon || \"Auto\",\n"
        "    icon_on: input.icon_on || \"Auto\", sensor: input.sensor || \"\", unit: input.unit || \"\",\n"
        "    type: input.type || \"\", precision: input.precision || \"\", options: input.options || \"\",\n"
        "  };\n"
        "}\n"
        "\n"
        "function optionPresent(options: string, name: string): boolean {\n"
        "  return options.split(\",\").indexOf(name) >= 0;\n"
        "}\n"
        "function decodeOptionValue(value: string): string {\n"
        "  return value.replace(/(%[0-9a-fA-F]{2})+/g, (run) => { try { return decodeURIComponent(run); } catch { return run; } });\n"
        "}\n"
        "function optionValue(options: string, name: string): string {\n"
        "  const prefix = name + \"=\";\n"
        "  for (const part of options.split(\",\")) if (part.indexOf(prefix) === 0) return decodeOptionValue(part.substring(prefix.length));\n"
        "  return \"\";\n"
        "}\n"
        "function encodeOptionValue(value: string): string {\n"
        "  return value.replace(/[%,;|:]/g, (character) => \"%\" + character.charCodeAt(0).toString(16).toUpperCase().padStart(2, \"0\"));\n"
        "}\n"
        "\n"
        "export function normalizeSavedConfigVacuumShadow(input: Partial<CardConfig>): CardConfig | null {\n"
        "  const config = shaped(input);\n"
        "  for (const action of Object.values(VACUUM_MIGRATIONS)) {\n"
        "    if (action.when.every((condition) => conditionMatches(config, condition))) Object.assign(config, action.set);\n"
        "  }\n"
        "  if (config.type !== \"vacuum\") return null;\n"
        "  const spec = SAVED_CONFIG_SHADOW_PILOT_POLICIES.vacuum!;\n"
        "  for (const field of Object.keys(spec.fields) as (keyof CardConfig)[]) {\n"
        "    const policy = spec.fields[field];\n"
        "    if (policy.policy === \"clear\") config[field] = \"\";\n"
        "    else if (policy.policy === \"default\") config[field] = policy.value;\n"
        "    else if (policy.policy === \"allowed\" && policy.values.indexOf(config[field]) < 0) config[field] = policy.fallback;\n"
        "    else if (policy.policy === \"alias\") config[field] = policy.aliases[config[field]] || config[field];\n"
        "  }\n"
        "  const hook = spec.hookData!.normalize_vacuum_fields as {\n"
        "    preserveUnitForModes: readonly string[]; defaultIcons: Readonly<Record<string, string>>;\n"
        "  };\n"
        "  if (hook.preserveUnitForModes.indexOf(config.sensor) < 0) config.unit = \"\";\n"
        "  if (!config.icon || config.icon === \"Auto\") config.icon = hook.defaultIcons[config.sensor] || hook.defaultIcons.default || \"Auto\";\n"
        "  return config;\n"
        "}\n"
        "\n"
        "export function normalizeSavedConfigSensorShadow(input: Partial<CardConfig>): CardConfig | null {\n"
        "  const config = shaped(input);\n"
        "  if (config.type === \"local_sensor\") { config.type = \"sensor\"; config.sensor = \"local\"; config.icon_on = \"Auto\"; config.options = \"\"; }\n"
        "  if (config.type !== \"sensor\") return null;\n"
        "  if (config.sensor === \"local\") {\n"
        "    config.icon_on = \"Auto\"; config.options = \"\";\n"
        "    if ([\"text\", \"1\", \"2\"].indexOf(config.precision) < 0) config.precision = \"\";\n"
        "    if (config.precision !== \"text\" && (!config.icon || config.icon === \"Auto\")) config.icon = \"Auto\";\n"
        "    return config;\n"
        "  }\n"
        "  const source = config.options; const out: string[] = [];\n"
        "  if (config.precision !== \"icon\" && config.precision !== \"text\") {\n"
        "    if (optionValue(source, \"large_numbers\") === \"off\") out.push(\"large_numbers=off\");\n"
        "    else if (optionPresent(source, \"large_numbers\")) out.push(\"large_numbers\");\n"
        "  }\n"
        "  if (config.precision === \"text\" && optionPresent(source, \"state_labels\")) {\n"
        "    out.push(\"state_labels\");\n"
        "    let stateInput = optionValue(source, \"state_input\"); let stateOutput = optionValue(source, \"state_output\");\n"
        "    if (!stateInput && optionValue(source, \"state_high_label\")) { stateInput = \"high\"; stateOutput = optionValue(source, \"state_high_label\"); }\n"
        "    else if (!stateInput && optionValue(source, \"state_low_label\")) { stateInput = \"low\"; stateOutput = optionValue(source, \"state_low_label\"); }\n"
        "    for (const [name, value] of [[\"state_input\", stateInput], [\"state_output\", stateOutput], [\"state_input_2\", optionValue(source, \"state_input_2\")], [\"state_output_2\", optionValue(source, \"state_output_2\")]]) {\n"
        "      if (value) out.push(name + \"=\" + encodeOptionValue(value));\n"
        "    }\n"
        "  }\n"
        "  config.options = out.join(\",\"); return config;\n"
        "}\n"
        "\n"
        "export function normalizeSavedConfigShadow(input: Partial<CardConfig>): CardConfig | null {\n"
        "  return normalizeSavedConfigVacuumShadow(input) || normalizeSavedConfigSensorShadow(input);\n"
        "}\n"
    )


def cpp_shadow_policy_kind(policy):
    return {
        "keep": "KEEP", "clear": "CLEAR", "default": "DEFAULT_VALUE",
        "allowed": "ALLOWED", "alias": "ALIAS", "hook": "HOOK",
    }[policy]


def gen_saved_config_shadow_h(data):
    policies = shadow_pilot_policies(data)
    vacuum = policies["vacuum"]
    hook = vacuum["hookData"]["normalize_vacuum_fields"]
    icons = hook["defaultIcons"]
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SHADOW HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// Test-only shadow code: production firmware does not include this header.\n",
        "// =============================================================================\n\n",
        "#include <cstddef>\n#include <string>\n\n",
        "enum class CardContractShadowPolicyKind { KEEP, CLEAR, DEFAULT_VALUE, ALLOWED, ALIAS, HOOK };\n",
        "struct CardContractShadowFieldPolicy { const char *field; CardContractShadowPolicyKind policy; const char *detail; };\n\n",
    ]
    for card_type, normalization in policies.items():
        name = re.sub(r"[^A-Za-z0-9]", "_", card_type).upper()
        lines.append(f"inline const CardContractShadowFieldPolicy SAVED_CONFIG_SHADOW_{name}_FIELDS[] = {{\n")
        for field, rule in normalization["fields"].items():
            detail = rule.get("hook", rule.get("value", rule.get("fallback", "")))
            lines.append(f"  {{{json.dumps(field)}, CardContractShadowPolicyKind::{cpp_shadow_policy_kind(rule['policy'])}, {json.dumps(detail)}}},\n")
        lines.append("};\n\n")
    modes = data["cards"]["vacuum"]["normalization"]["fields"]["sensor"]["values"]
    lines.extend([
        cpp_string_array("SAVED_CONFIG_SHADOW_VACUUM_MODES", modes),
        cpp_string_array("SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES", hook["preserveUnitForModes"]),
        "\ninline bool saved_config_shadow_string_in(const std::string &value, const char *const *values, size_t count) {\n",
        "  for (size_t index = 0; index < count; ++index) if (value == values[index]) return true;\n",
        "  return false;\n}\n\n",
        "inline const char *saved_config_shadow_vacuum_icon(const std::string &mode) {\n",
    ])
    for mode, icon in icons.items():
        if mode != "default":
            lines.append(f"  if (mode == {json.dumps(mode)}) return {json.dumps(icon)};\n")
    lines.extend([
        f"  return {json.dumps(icons['default'])};\n}}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_vacuum_shadow(Config &config) {\n",
        "  if (config.type == \"action\" && config.sensor == \"vacuum.start\") { config.type = \"vacuum\"; config.sensor = \"start_stop\"; }\n",
        "  if (config.type == \"action\" && config.sensor == \"vacuum.return_to_base\") { config.type = \"vacuum\"; config.sensor = \"dock\"; }\n",
        "  if (config.type != \"vacuum\") return false;\n",
        "  if (!saved_config_shadow_string_in(config.sensor, SAVED_CONFIG_SHADOW_VACUUM_MODES, sizeof(SAVED_CONFIG_SHADOW_VACUUM_MODES) / sizeof(SAVED_CONFIG_SHADOW_VACUUM_MODES[0]))) config.sensor = \"start_stop\";\n",
        "  config.type = \"vacuum\"; config.icon_on = \"Auto\"; config.precision.clear(); config.options.clear();\n",
        "  if (!saved_config_shadow_string_in(config.sensor, SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES, sizeof(SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES) / sizeof(SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES[0]))) config.unit.clear();\n",
        "  if (config.icon.empty() || config.icon == \"Auto\") config.icon = saved_config_shadow_vacuum_icon(config.sensor);\n",
        "  return true;\n}\n",
        "\ninline void saved_config_shadow_append_option(std::string &out, const std::string &name, const std::string &value = \"\") {\n",
        "  if (!out.empty()) out += ','; out += name; if (!value.empty()) out += \"=\" + encode_compact_field(value);\n",
        "}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_sensor_shadow(Config &config) {\n",
        "  if (config.type == \"local_sensor\") { config.type = \"sensor\"; config.sensor = \"local\"; config.icon_on = \"Auto\"; config.options.clear(); }\n",
        "  if (config.type != \"sensor\") return false;\n",
        "  if (config.sensor == \"local\") { config.icon_on = \"Auto\"; config.options.clear(); if (config.precision != \"text\" && config.precision != \"1\" && config.precision != \"2\") config.precision.clear(); if (config.precision != \"text\" && (config.icon.empty() || config.icon == \"Auto\")) config.icon = \"Auto\"; return true; }\n",
        "  const std::string source = config.options; std::string out;\n",
        "  if (config.precision != \"icon\" && config.precision != \"text\") append_large_numbers_option(out, source);\n",
        "  if (config.precision == \"text\" && cfg_option_token_present(source, \"state_labels\")) {\n",
        "    saved_config_shadow_append_option(out, \"state_labels\"); std::string input = cfg_option_value(source, \"state_input\"); std::string output = cfg_option_value(source, \"state_output\");\n",
        "    if (input.empty() && !cfg_option_value(source, \"state_high_label\").empty()) { input = \"high\"; output = cfg_option_value(source, \"state_high_label\"); }\n",
        "    else if (input.empty() && !cfg_option_value(source, \"state_low_label\").empty()) { input = \"low\"; output = cfg_option_value(source, \"state_low_label\"); }\n",
        "    if (!input.empty()) saved_config_shadow_append_option(out, \"state_input\", input); if (!output.empty()) saved_config_shadow_append_option(out, \"state_output\", output);\n",
        "    const std::string input_2 = cfg_option_value(source, \"state_input_2\"); const std::string output_2 = cfg_option_value(source, \"state_output_2\");\n",
        "    if (!input_2.empty()) saved_config_shadow_append_option(out, \"state_input_2\", input_2); if (!output_2.empty()) saved_config_shadow_append_option(out, \"state_output_2\", output_2);\n",
        "  }\n",
        "  config.options = out; return true;\n}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_shadow(Config &config) {\n",
        "  if (normalize_saved_config_vacuum_shadow(config)) return true; return normalize_saved_config_sensor_shadow(config);\n",
        "}\n",
    ])
    return "".join(lines)


def cpp_string_array(name, values):
    quoted = ", ".join(json.dumps(v) for v in values)
    return f"inline const char *const {name}[] = {{{quoted}}};\n"


def contract_card_option(cards, card_type, option_name):
    for option in cards[card_type].get("options", []):
        if option.get("name") == option_name:
            return option
    raise KeyError(f"Missing {card_type}.{option_name} option metadata")


def contract_card_option_values(cards, card_type, option_name):
    return contract_card_option(cards, card_type, option_name).get("values", [])


def contract_card_option_default(cards, card_type, option_name):
    return contract_card_option(cards, card_type, option_name).get("defaultValue", "")


def contract_card_option_int(cards, card_type, option_name, key):
    return int(contract_card_option(cards, card_type, option_name).get(key, 0))


def gen_card_contract_h(data):
    groups = data["cardGroups"]
    fan = groups["fan"]
    codes = data["subpageTypeCodes"]
    option_actions = data["optionSelect"]["actions"]
    cards = data["cards"]
    alarm_behavior = cards["alarm"]["behavior"]["alarm"]
    cover_behavior = cards["cover"]["behavior"]["cover"]
    lock_behavior = cards["lock"]["behavior"]["lock"]
    media_behavior = cards["media"]["behavior"]["media"]
    climate_behavior = cards["climate"]["behavior"]["climate"]
    large_numbers = data["largeNumbers"]
    option_names = contract_option_names(data)
    lines = [
        "#pragma once\n",
        "\n",
        "// =============================================================================\n",
        "// GENERATED CARD CONFIG CONTRACT - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        "\n",
        f"constexpr int CARD_CONTRACT_VERSION = {int(data['contractVersion'])};\n",
        f'constexpr const char *CARD_CONTRACT_OPTION_SELECT_ACTION = {json.dumps(data["optionSelect"]["canonicalAction"])};\n',
        cpp_string_array("CARD_CONTRACT_OPTION_SELECT_ACTIONS", option_actions),
        cpp_string_array("CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES", groups["brightnessSlider"]),
        cpp_string_array("CARD_CONTRACT_COVER_MODES", contract_card_option_values(cards, "cover", "cover_mode")),
        cpp_string_array("CARD_CONTRACT_COVER_CONTROL_TABS", contract_card_option_values(cards, "cover", "cover_tabs")),
        cpp_string_array("CARD_CONTRACT_GARAGE_MODES", contract_card_option_values(cards, "garage", "garage_mode")),
        cpp_string_array("CARD_CONTRACT_GARAGE_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "garage", "label_display")),
        cpp_string_array("CARD_CONTRACT_GATE_MODES", contract_card_option_values(cards, "gate", "gate_mode")),
        cpp_string_array("CARD_CONTRACT_GATE_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "gate", "label_display")),
        cpp_string_array("CARD_CONTRACT_INTERNAL_MODES", contract_card_option_values(cards, "internal", "internal_mode")),
        cpp_string_array("CARD_CONTRACT_LOCK_MODES", contract_card_option_values(cards, "lock", "lock_mode")),
        cpp_string_array("CARD_CONTRACT_MEDIA_MODES", contract_card_option_values(cards, "media", "media_mode")),
        cpp_string_array("CARD_CONTRACT_MEDIA_DISPLAY_MODES", contract_card_option_values(cards, "media", "media_display")),
        cpp_string_array("CARD_CONTRACT_MEDIA_NOW_PLAYING_CONTROLS", contract_card_option_values(cards, "media", "media_now_playing_controls")),
        cpp_string_array("CARD_CONTRACT_MEDIA_LEGACY_MODES", media_behavior["legacyModes"].keys()),
        cpp_string_array("CARD_CONTRACT_MEDIA_STATE_DISPLAY_MODES", media_behavior["stateDisplayModes"]),
        cpp_string_array("CARD_CONTRACT_ALARM_ACTION_MODES", [item["value"] for item in alarm_behavior["actions"]]),
        cpp_string_array("CARD_CONTRACT_ALARM_DEFAULT_ACTIONS", alarm_behavior["defaultActions"]),
        cpp_string_array("CARD_CONTRACT_ALARM_ICON_DISPLAY_MODES", contract_card_option_values(cards, "alarm", "icon_display")),
        cpp_string_array("CARD_CONTRACT_ALARM_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "alarm", "label_display")),
        cpp_string_array("CARD_CONTRACT_IMAGE_MODAL_MODES", contract_card_option_values(cards, "image", "image_modal_mode")),
        cpp_string_array("CARD_CONTRACT_LIGHT_CONTROL_TABS", contract_card_option_values(cards, "light_control", "light_tabs")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "climate", "label_display")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_MODES", contract_card_option_values(cards, "climate", "number_display")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_TEMPERATURE_STEPS", contract_card_option_values(cards, "climate", "temperature_step")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_PRECISION_VALUES", climate_behavior["precisionValues"]),
        cpp_string_array("CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS", large_numbers["weather"]["precisions"]),
        "".join(
            f"constexpr const char *{option_constant_name(name)} = {json.dumps(value)};\n"
            for name, value in option_names.items()
        ),
        f'constexpr const char *CARD_CONTRACT_GARAGE_LABEL_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "garage", "label_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_GATE_LABEL_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "gate", "label_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_COVER_CONTROL_TABS_DEFAULT = {json.dumps(contract_card_option_default(cards, "cover", "cover_tabs"))};\n',
        f'constexpr const char *CARD_CONTRACT_MEDIA_DEFAULT_MODE = {json.dumps(media_behavior["defaultMode"])};\n',
        f'constexpr int CARD_CONTRACT_MEDIA_VOLUME_MAX_MIN = {contract_card_option_int(cards, "media", "volume_max", "min")};\n',
        f'constexpr int CARD_CONTRACT_MEDIA_VOLUME_MAX_MAX = {contract_card_option_int(cards, "media", "volume_max", "max")};\n',
        f'constexpr int CARD_CONTRACT_MEDIA_VOLUME_MAX_DEFAULT = {int(contract_card_option_default(cards, "media", "volume_max"))};\n',
        f'constexpr size_t CARD_CONTRACT_ALARM_MAX_VISIBLE_ACTIONS = {int(alarm_behavior.get("maxVisibleActions", 3))};\n',
        f'constexpr const char *CARD_CONTRACT_ALARM_ICON_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "alarm", "icon_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_ALARM_LABEL_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "alarm", "label_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_IMAGE_MODAL_MODE_DEFAULT = {json.dumps(contract_card_option_default(cards, "image", "image_modal_mode"))};\n',
        f'constexpr const char *CARD_CONTRACT_LIGHT_CONTROL_TABS_DEFAULT = {json.dumps(contract_card_option_default(cards, "light_control", "light_tabs"))};\n',
        f'constexpr const char *CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_DEFAULT = {json.dumps(climate_behavior["defaultLabelDisplay"])};\n',
        f'constexpr const char *CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_DEFAULT = {json.dumps(climate_behavior["defaultNumberDisplay"])};\n',
        f'constexpr const char *CARD_CONTRACT_CLIMATE_TEMPERATURE_STEP_DEFAULT = {json.dumps(climate_behavior["defaultTemperatureStep"])};\n',
        "\n",
        "inline bool card_contract_string_in(const std::string &value, const char *const *items, size_t count) {\n",
        "  for (size_t i = 0; i < count; i++) {\n",
        "    if (value == items[i]) return true;\n",
        "  }\n",
        "  return false;\n",
        "}\n",
        "\n",
        "inline bool card_contract_is_brightness_slider_type(const std::string &type) {\n",
        "  return card_contract_string_in(type, CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES,\n",
        "    sizeof(CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES) / sizeof(CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_is_option_select_action(const std::string &action) {\n",
        "  return card_contract_string_in(action, CARD_CONTRACT_OPTION_SELECT_ACTIONS,\n",
        "    sizeof(CARD_CONTRACT_OPTION_SELECT_ACTIONS) / sizeof(CARD_CONTRACT_OPTION_SELECT_ACTIONS[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_cover_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_COVER_MODES,\n",
        "    sizeof(CARD_CONTRACT_COVER_MODES) / sizeof(CARD_CONTRACT_COVER_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_cover_control_tab_valid(const std::string &tab) {\n",
        "  return card_contract_string_in(tab, CARD_CONTRACT_COVER_CONTROL_TABS,\n",
        "    sizeof(CARD_CONTRACT_COVER_CONTROL_TABS) / sizeof(CARD_CONTRACT_COVER_CONTROL_TABS[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_garage_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_GARAGE_MODES,\n",
        "    sizeof(CARD_CONTRACT_GARAGE_MODES) / sizeof(CARD_CONTRACT_GARAGE_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_garage_label_display_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_GARAGE_LABEL_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_GARAGE_LABEL_DISPLAY_MODES) / sizeof(CARD_CONTRACT_GARAGE_LABEL_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_gate_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_GATE_MODES,\n",
        "    sizeof(CARD_CONTRACT_GATE_MODES) / sizeof(CARD_CONTRACT_GATE_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_gate_label_display_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_GATE_LABEL_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_GATE_LABEL_DISPLAY_MODES) / sizeof(CARD_CONTRACT_GATE_LABEL_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_internal_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_INTERNAL_MODES,\n",
        "    sizeof(CARD_CONTRACT_INTERNAL_MODES) / sizeof(CARD_CONTRACT_INTERNAL_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_lock_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_LOCK_MODES,\n",
        "    sizeof(CARD_CONTRACT_LOCK_MODES) / sizeof(CARD_CONTRACT_LOCK_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_media_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_MEDIA_MODES,\n",
        "    sizeof(CARD_CONTRACT_MEDIA_MODES) / sizeof(CARD_CONTRACT_MEDIA_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_media_legacy_mode(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_MEDIA_LEGACY_MODES,\n",
        "    sizeof(CARD_CONTRACT_MEDIA_LEGACY_MODES) / sizeof(CARD_CONTRACT_MEDIA_LEGACY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_media_state_display_mode(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_MEDIA_STATE_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_MEDIA_STATE_DISPLAY_MODES) / sizeof(CARD_CONTRACT_MEDIA_STATE_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_alarm_action_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_ALARM_ACTION_MODES,\n",
        "    sizeof(CARD_CONTRACT_ALARM_ACTION_MODES) / sizeof(CARD_CONTRACT_ALARM_ACTION_MODES[0]));\n",
        "}\n",
        "\n",
        "inline size_t card_contract_alarm_default_action_count() {\n",
        "  return sizeof(CARD_CONTRACT_ALARM_DEFAULT_ACTIONS) / sizeof(CARD_CONTRACT_ALARM_DEFAULT_ACTIONS[0]);\n",
        "}\n",
        "\n",
        "inline const char *card_contract_alarm_default_action_at(size_t index) {\n",
        "  return index < card_contract_alarm_default_action_count()\n",
        "    ? CARD_CONTRACT_ALARM_DEFAULT_ACTIONS[index]\n",
        "    : \"\";\n",
        "}\n",
        "\n",
        "inline bool card_contract_alarm_icon_display_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_ALARM_ICON_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_ALARM_ICON_DISPLAY_MODES) / sizeof(CARD_CONTRACT_ALARM_ICON_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_alarm_label_display_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_ALARM_LABEL_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_ALARM_LABEL_DISPLAY_MODES) / sizeof(CARD_CONTRACT_ALARM_LABEL_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_image_modal_mode_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_IMAGE_MODAL_MODES,\n",
        "    sizeof(CARD_CONTRACT_IMAGE_MODAL_MODES) / sizeof(CARD_CONTRACT_IMAGE_MODAL_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_light_control_tab_valid(const std::string &tab) {\n",
        "  return card_contract_string_in(tab, CARD_CONTRACT_LIGHT_CONTROL_TABS,\n",
        "    sizeof(CARD_CONTRACT_LIGHT_CONTROL_TABS) / sizeof(CARD_CONTRACT_LIGHT_CONTROL_TABS[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_climate_label_display_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_MODES) / sizeof(CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_climate_number_display_valid(const std::string &mode) {\n",
        "  return card_contract_string_in(mode, CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_MODES,\n",
        "    sizeof(CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_MODES) / sizeof(CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_MODES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_climate_temperature_step_valid(const std::string &step) {\n",
        "  return card_contract_string_in(step, CARD_CONTRACT_CLIMATE_TEMPERATURE_STEPS,\n",
        "    sizeof(CARD_CONTRACT_CLIMATE_TEMPERATURE_STEPS) / sizeof(CARD_CONTRACT_CLIMATE_TEMPERATURE_STEPS[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_climate_precision_valid(const std::string &precision) {\n",
        "  return card_contract_string_in(precision, CARD_CONTRACT_CLIMATE_PRECISION_VALUES,\n",
        "    sizeof(CARD_CONTRACT_CLIMATE_PRECISION_VALUES) / sizeof(CARD_CONTRACT_CLIMATE_PRECISION_VALUES[0]));\n",
        "}\n",
        "\n",
        "inline bool card_contract_weather_forecast_precision(const std::string &precision) {\n",
        "  return card_contract_string_in(precision, CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS,\n",
        "    sizeof(CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS) / sizeof(CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS[0]));\n",
        "}\n",
        "\n",
        "inline const char *card_contract_cover_command_service(const std::string &mode) {\n",
    ]
    for mode, service in cover_behavior["commandServices"].items():
        lines.append(f'  if (mode == {json.dumps(mode)}) return {json.dumps(service)};\n')
    lines.extend([
        "  return nullptr;\n",
        "}\n",
        "\n",
        "inline const char *card_contract_lock_command_service(const std::string &mode) {\n",
    ])
    for mode, service in lock_behavior["commandServices"].items():
        lines.append(f'  if (mode == {json.dumps(mode)}) return {json.dumps(service)};\n')
    lines.extend([
        "  return nullptr;\n",
        "}\n",
        "\n",
        "inline const char *card_contract_lock_toggle_service(const std::string &state) {\n",
    ])
    lock_toggle_services = lock_behavior["toggleServices"]
    for state, service in lock_toggle_services.items():
        if state == "default":
            continue
        lines.append(f'  if (state == {json.dumps(state)}) return {json.dumps(service)};\n')
    lines.extend([
        f'  return {json.dumps(lock_toggle_services["default"])};\n',
        "}\n",
        "\n",
        "inline const char *card_contract_media_playback_service(const std::string &mode) {\n",
    ])
    media_playback_services = media_behavior["playbackServices"]
    for mode, service in media_playback_services.items():
        lines.append(f'  if (mode == {json.dumps(mode)}) return {json.dumps(service)};\n')
    lines.extend([
        f'  return {json.dumps(media_playback_services[media_behavior["defaultMode"]])};\n',
        "}\n",
        "\n",
        "inline const char *card_contract_alarm_action_service(const std::string &mode) {\n",
    ])
    for action in alarm_behavior["actions"]:
        lines.append(f'  if (mode == {json.dumps(action["value"])}) return {json.dumps(action["service"])};\n')
    lines.extend([
        "  return nullptr;\n",
        "}\n",
        "\n",
        "inline const char *card_contract_alarm_action_icon_name(const std::string &mode) {\n",
    ])
    for action in alarm_behavior["actions"]:
        lines.append(f'  if (mode == {json.dumps(action["value"])}) return {json.dumps(action["icon"])};\n')
    lines.extend([
        "  return \"Alarm\";\n",
        "}\n",
        "\n",
        "inline bool card_contract_alarm_action_legacy_icon_name(const std::string &mode, const std::string &icon) {\n",
    ])
    for action in alarm_behavior["actions"]:
        lines.append(f'  if (mode == {json.dumps(action["value"])}) return icon == {json.dumps(action["legacyIcon"])};\n')
    lines.extend([
        "  return false;\n",
        "}\n",
        "\n",
        "inline const char *card_contract_card_label(const std::string &type) {\n",
    ])
    for card_type, card in cards.items():
        lines.append(f'  if (type == {json.dumps(card_type)}) return {json.dumps(card["label"])};\n')
    lines.extend([
        "  return type.empty() ? \"Switch\" : type.c_str();\n",
        "}\n",
        "\n",
        "inline bool card_contract_allow_in_subpage(const std::string &type) {\n",
    ])
    for card_type, card in cards.items():
        lines.append(f'  if (type == {json.dumps(card_type)}) return {"true" if card["allowInSubpage"] else "false"};\n')
    lines.extend([
        "  return false;\n",
        "}\n",
        "\n",
        "inline const char *card_contract_default_icon_name(const std::string &type) {\n",
    ])
    for card_type, card in cards.items():
        lines.append(f'  if (type == {json.dumps(card_type)}) return {json.dumps(card["default"]["icon"])};\n')
    lines.extend([
        "  return \"Auto\";\n",
        "}\n",
        "\n",
        "inline const char *card_contract_default_icon_on_name(const std::string &type) {\n",
    ])
    for card_type, card in cards.items():
        lines.append(f'  if (type == {json.dumps(card_type)}) return {json.dumps(card["default"]["icon_on"])};\n')
    lines.extend([
        "  return \"Auto\";\n",
        "}\n",
        "\n",
        "inline bool card_contract_is_fan_card_type(const std::string &type) {\n",
    ]
    )
    fan_conditions = " ||\n         ".join(f'type == "{card_type}"' for card_type in fan.keys())
    lines.append(f"  return {fan_conditions};\n")
    lines.extend([
        "}\n",
        "\n",
        "inline const char *card_contract_fan_default_icon_name(const std::string &type) {\n",
    ])
    for card_type, config in fan.items():
        lines.append(f'  if (type == "{card_type}") return {json.dumps(config["defaultIcon"])};\n')
    lines.extend([
        "  return \"Fan Speed 2\";\n",
        "}\n",
        "\n",
        "inline const char *card_contract_fan_default_icon_on_name(const std::string &type) {\n",
    ])
    for card_type, config in fan.items():
        if config.get("defaultIconOn"):
            lines.append(f'  if (type == "{card_type}") return {json.dumps(config["defaultIconOn"])};\n')
    lines.extend([
        "  return \"Auto\";\n",
        "}\n",
        "\n",
        "inline bool card_contract_large_numbers_supported(const std::string &type, const std::string &precision) {\n",
        "  if (type == \"sensor\") return precision != \"icon\" && precision != \"text\";\n",
        "  if (type == \"weather\") return precision == \"today\" || precision == \"tomorrow\";\n",
        "  return type == \"\" || type == \"action\" || type == \"calendar\" || type == \"clock\" ||\n",
        "         type == \"climate\" || type == \"media\" || type == \"subpage\" ||\n",
        "         type == \"timezone\";\n",
        "}\n",
        "\n",
        "inline const char *card_contract_subpage_type_code(const std::string &type) {\n",
    ])
    for card_type, code in codes.items():
        lines.append(f'  if (type == "{card_type}") return "{code}";\n')
    lines.extend([
        "  return type.c_str();\n",
        "}\n",
        "\n",
        "inline std::string card_contract_subpage_type_from_code(const std::string &code) {\n",
    ])
    for card_type, code in codes.items():
        lines.append(f'  if (code == "{code}") return "{card_type}";\n')
    lines.extend([
        "  return code;\n",
        "}\n",
    ])
    return "".join(lines)


def generated_card_markdown_header(kind):
    return (
        "<!-- =============================================================================\n"
        f"GENERATED {kind} - do not edit by hand\n"
        "Generated by scripts/build.py from common/config/card_contract.json.\n"
        "============================================================================= -->\n\n"
    )


def markdown_table_cell(value):
    return str(value).replace("|", "\\|").replace("\n", "<br>")


def card_doc_type_name(card_type):
    return card_type or "switch"


def summarize_card_options(card):
    options = []
    for option in card.get("options", []):
        if option.get("docsHidden"):
            continue
        label = option.get("label") or option["name"]
        values = option.get("values") or []
        if values:
            display_values = [value or "default" for value in values]
            options.append(f"{label}: {', '.join(display_values)}")
        elif option.get("kind") == "number":
            min_value = option.get("min")
            max_value = option.get("max")
            if min_value is not None and max_value is not None:
                options.append(f"{label}: {min_value}-{max_value}")
            else:
                options.append(label)
        else:
            options.append(label)
    return "; ".join(options) if options else "None"


def summarize_card_status(card):
    if card.get("hidden"):
        return "Hidden"
    return "Visible"


def summarize_picker_group(card_type, card, cards):
    picker_key = card.get("pickerKey")
    if not picker_key:
        return "Own picker item"
    picker_label = cards.get(picker_key, {}).get("label") or card_doc_type_name(picker_key)
    return f"{picker_label} ({card_doc_type_name(picker_key)})"


def gen_card_capability_docs(data):
    cards = data["cards"]
    lines = [
        generated_card_markdown_header("CARD CAPABILITIES"),
        "# Card Capability Reference\n\n",
        "This generated reference lists stable setup facts from the shared card contract. "
        "Explanations, screenshots, and card-specific guidance stay in the hand-written card docs.\n\n",
        "| Card | Type | Entity domains | Subpages | Picker grouping | Main modes and options | Status |\n",
        "|---|---|---|---|---|---|---|\n",
    ]
    for card_type, card in cards.items():
        domains = ", ".join(card.get("domains") or []) or "None"
        row = [
            card["label"],
            card_doc_type_name(card_type),
            domains,
            "Yes" if card.get("allowInSubpage") else "No",
            summarize_picker_group(card_type, card, cards),
            summarize_card_options(card),
            summarize_card_status(card),
        ]
        lines.append("| " + " | ".join(markdown_table_cell(value) for value in row) + " |\n")
    return "".join(lines)


def sync_card_contract(check_only=False):
    data = load_card_contract_data()
    assert_card_contract_valid(data)
    outputs = [
        (CARD_CONTRACT_TS, gen_card_contract_ts(data)),
        (CARD_CONTRACT_H, gen_card_contract_h(data)),
        (SAVED_CONFIG_SHADOW_TS, gen_saved_config_shadow_ts(data)),
        (SAVED_CONFIG_SHADOW_H, gen_saved_config_shadow_h(data)),
        (CARD_DOCS_DIR / "capabilities.md", gen_card_capability_docs(data)),
    ]
    dirty = []
    for path, content in outputs:
        if not path.exists() or path.read_text() != content:
            dirty.append(path.relative_to(ROOT))

    if check_only:
        if dirty:
            print("Card contract outputs are out of sync. Run 'python scripts/build.py contract' to fix:")
            for rel in dirty:
                print(f"  {rel}")
        return dirty

    for path, content in outputs:
        if path.exists() and path.read_text() == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        print(f"  updated {path.relative_to(ROOT)}")
    return dirty


# ===========================================================================
# Public device capability generation
# ===========================================================================

def gen_device_capabilities_json():
    return json.dumps(public_device_capabilities(), indent=2) + "\n"


def generated_markdown_header(kind):
    return (
        "<!-- =============================================================================\n"
        f"GENERATED {kind} - do not edit by hand\n"
        "Generated by scripts/build.py from devices/manifest.json.\n"
        "============================================================================= -->\n\n"
    )


def screen_doc_stem(capability):
    return capability["docsPath"].rstrip("/").split("/")[-1]


def gen_device_grid_snippet(capability):
    rows = capability["grid"]["rows"]
    cols = capability["grid"]["cols"]
    slots = capability["slots"]
    relays = capability.get("relays", 0)
    relay_text = "No built-in relays" if relays == 0 else f"{relays} built-in relay" + ("" if relays == 1 else "s")
    ethernet = "Yes, manual ESPHome install only" if capability.get("ethernetManualInstall") else "No"
    if capability.get("subpages", True):
        layout_text = (
            f"The home screen uses a **{rows}-row x {cols}-column** grid, giving you "
            f"**{slots} card slots**. Any home-screen card can be turned into a "
            f"[Subpage](/features/subpages) folder containing up to {slots - 1} more cards.\n\n"
        )
    else:
        layout_text = (
            f"The home screen uses a **{rows}-row x {cols}-column** grid, giving you "
            f"**{slots} card slots**. Touch subpages are not available on this device.\n\n"
        )
    return (
        generated_markdown_header("SCREEN GRID CAPABILITIES") +
        layout_text +
        "Flexible card sizes are supported: Single, Tall, Wide, and Large.\n\n"
        "| Capability | Value |\n"
        "|---|---|\n"
        f"| Screen | {capability['screenSize']}, {capability['resolution']}, {capability['orientation']} |\n"
        f"| Processor | {capability['chipFamily']} |\n"
        f"| Built-in relays | {relay_text} |\n"
        f"| Rotation support | {'Yes' if capability.get('rotation') else 'No'} |\n"
        f"| Browser install slug | `{capability['installSlug']}` |\n"
        f"| Ethernet option | {ethernet} |\n"
    )


def gen_device_install_snippet(capability):
    return (
        generated_markdown_header("SCREEN INSTALL BUTTON") +
        f'<EspInstallButton slug="{capability["installSlug"]}" />\n'
    )


def device_docs_outputs(capabilities):
    outputs = {}
    for capability in capabilities["devices"]:
        stem = screen_doc_stem(capability)
        outputs[DEVICE_DOCS_DIR / f"{stem}-grid.md"] = gen_device_grid_snippet(capability)
        outputs[DEVICE_DOCS_DIR / f"{stem}-install.md"] = gen_device_install_snippet(capability)
    return outputs


def sync_device_capabilities(check_only=False):
    capabilities = public_device_capabilities()
    outputs = {
        DEVICE_CAPABILITIES_JSON: gen_device_capabilities_json(),
        **device_docs_outputs(capabilities),
    }
    dirty = []
    for path, generated in outputs.items():
        if not path.exists() or path.read_text() != generated:
            dirty.append(path.relative_to(ROOT))

    if check_only:
        if dirty:
            print("Device capability outputs are out of sync. Run 'python scripts/build.py devices' to fix:")
            for rel in dirty:
                print(f"  {rel}")
        return dirty

    for path, generated in outputs.items():
        if path.exists() and path.read_text() == generated:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(generated)
        print(f"  updated {path.relative_to(ROOT)}")
    return dirty


# ===========================================================================
# Icon sync
# ===========================================================================

def icon_items(data):
    return [data["fallback"], *data.get("structural", []), *data["icons"]]


def load_mdi_codepoints():
    """Load the codepoint map from the same MDI CSS version used by the web UI."""
    try:
        with urllib.request.urlopen(MDI_CSS_URL, timeout=20) as response:
            css = response.read().decode("utf-8")
    except Exception as exc:
        raise BuildError(f"Unable to fetch pinned MDI CSS from {MDI_CSS_URL}: {exc}") from exc

    return {
        match.group(1): match.group(2).upper()
        for match in re.finditer(
            r'\.mdi-([a-z0-9-]+)::before \{\s*content: "\\([0-9A-Fa-f]+)";',
            css,
        )
    }


def check_duplicate_icon_fields(data):
    errors = []
    seen = {}
    for item in icon_items(data):
        seen.setdefault(item["name"], []).append(item["mdi"])
    for name, mdi_names in seen.items():
        if len(mdi_names) > 1:
            errors.append(f"duplicate name {name!r}: {', '.join(mdi_names)}")
    return errors


def normalize_icon_codepoint(value):
    return value.upper().lstrip("0") or "0"


def check_firmware_icon_literals(data):
    """Catch raw firmware icon literals that are missing from the device font subset."""
    known_codepoints = {
        normalize_icon_codepoint(item["codepoint"])
        for item in icon_items(data)
    }
    errors = []
    for path in sorted((ROOT / "components" / "espcontrol").glob("*.h")):
        if path.name == "icons.h":
            continue
        text = path.read_text()
        for match in re.finditer(r"\\U([0-9A-Fa-f]{8})", text):
            codepoint = normalize_icon_codepoint(match.group(1))
            if codepoint in known_codepoints:
                continue
            line_no = text.count("\n", 0, match.start()) + 1
            errors.append(
                f"{path.relative_to(ROOT)}:{line_no} uses U+{codepoint:0>8s} "
                f"but it is not in {ICONS_JSON.relative_to(ROOT)}"
            )
    return errors


def check_mdi_versions():
    """Make sure the browser CSS and device font URLs stay on the same MDI version."""
    files = [
        ROOT / "src" / "webserver" / "application" / "app.ts",
        ROOT / "common" / "assets" / "icons.yaml",
        *sorted(ROOT.glob("devices/*/device/device.yaml")),
        *sorted(ROOT.glob("devices/*/dev.yaml")),
        *sorted(ROOT.glob("builds/*.yaml")),
    ]
    version_re = re.compile(
        r"(?:@mdi/font@|MaterialDesign-Webfont/raw/v|materialdesignicons\.com/cdn/|materialdesignicons-webfont-)"
        r"([0-9]+(?:\.[0-9]+)+)"
    )
    errors = []
    for path in files:
        versions = set(version_re.findall(path.read_text()))
        if versions and versions != {MDI_VERSION}:
            rel = path.relative_to(ROOT)
            errors.append(f"{rel} references MDI version(s) {', '.join(sorted(versions))}, expected {MDI_VERSION}")
    return errors


def validate_icon_data(data):
    """Verify icons.json matches the pinned Material Design Icons release."""
    errors = []
    errors.extend(check_duplicate_icon_fields(data))
    errors.extend(check_mdi_versions())
    errors.extend(check_firmware_icon_literals(data))

    mdi_codepoints = load_mdi_codepoints()
    for item in icon_items(data):
        mdi = item["mdi"]
        expected = mdi_codepoints.get(mdi)
        actual = item["codepoint"].upper()
        if expected is None:
            errors.append(f"{item['name']} references missing mdi-{mdi}")
        elif actual != expected:
            errors.append(f"{item['name']} / mdi-{mdi}: icons.json={actual}, MDI {MDI_VERSION}={expected}")

    return errors


def assert_icon_data_valid(data):
    errors = validate_icon_data(data)
    if not errors:
        return

    print(f"Icon data does not match Material Design Icons {MDI_VERSION}:")
    for error in errors:
        print(f"  {error}")
    raise BuildError("Icon validation failed.")


# ===========================================================================
# Icon sync (formerly sync_icons.py)
# ===========================================================================

def gen_icon_glyphs(data):
    """Font glyph codepoint list for LVGL font subsetting."""
    fb = data["fallback"]
    seen_codepoints = {fb["codepoint"]}
    lines = [f'- "\\U{fb["codepoint"]:>08s}"  # mdi-{fb["mdi"]} (Auto fallback)\n']
    for icon in data.get("structural", []):
        if icon["codepoint"] in seen_codepoints:
            continue
        seen_codepoints.add(icon["codepoint"])
        comment = icon.get("comment", "")
        suffix = f" ({comment})" if comment else ""
        lines.append(f'- "\\U{icon["codepoint"]:>08s}"  # mdi-{icon["mdi"]}{suffix}\n')
    for icon in data["icons"]:
        if icon["codepoint"] in seen_codepoints:
            continue
        seen_codepoints.add(icon["codepoint"])
        cp = icon["codepoint"]
        lines.append(f'- "\\U{cp:>08s}"  # mdi-{icon["mdi"]}\n')
    return "".join(lines)


def gen_icons_h_entries(data):
    """C++ IconEntry array initializers for icons.h."""
    max_name_len = max(len(i["name"]) for i in data["icons"])
    lines = []
    for icon in data["icons"]:
        padded = f'"{icon["name"]}",'
        padded = padded.ljust(max_name_len + 3)
        lines.append(f'    {{{padded} "\\U{icon["codepoint"]:>08s}"}},\n')
    return "".join(lines)


def gen_icons_h_domain_icons(data):
    """C++ early-return chain for domain default icons in icons.h."""
    icon_by_name = {i["name"]: i for i in data["icons"]}
    entries = list(data["domain_defaults"].items())
    target_col = 46
    lines = []
    for domain, icon_name in entries:
        icon = icon_by_name[icon_name]
        cp = icon["codepoint"]
        prefix = f'  if (domain == "{domain}")'
        pad = max(target_col - len(prefix), 1)
        lines.append(
            f'{prefix}{" " * pad}'
            f'return "\\U{cp:>08s}";  // {icon_name}\n'
        )
    return "".join(lines)


def gen_web_icon_module(data):
    """Typed icon names and exception map for the web bundle."""
    fb = data["fallback"]
    exceptions = [f'    Auto: "{fb["mdi"]}",\n']
    names = []

    for icon in data["icons"]:
        name = icon["name"]
        mdi = icon["mdi"]
        names.append(name)
        expected = re.sub(r"[^a-z0-9 ]", "", name.lower()).replace(" ", "-")
        if expected != mdi:
            key = name if re.match(r"^[A-Za-z_$][A-Za-z0-9_$]*$", name) else f'"{name}"'
            exceptions.append(f'    {key}: "{mdi}",\n')

    lines = ["export const GENERATED_ICON_EXCEPTIONS: Readonly<Record<string, string>> = {\n"]
    lines.extend(exceptions)
    lines.append("};\n")
    lines.append("export const GENERATED_ICON_NAMES: readonly string[] = [\n")
    for i in range(0, len(names), 6):
        chunk = names[i : i + 6]
        formatted = ", ".join(f'"{n}"' for n in chunk)
        lines.append(f"    {formatted},\n")
    lines.append("  ];\n")
    return "".join(lines)


def gen_web_domain_icon_module(data):
    """Typed domain-to-icon map for the web bundle."""
    icon_by_name = {i["name"]: i for i in data["icons"]}
    lines = ["export const GENERATED_DOMAIN_ICONS: Readonly<Record<string, string>> = {\n"]
    for domain, icon_name in data["domain_defaults"].items():
        mdi = icon_by_name[icon_name]["mdi"]
        lines.append(f'  {domain}: "{mdi}",\n')
    lines.append("};\n")
    return "".join(lines)


def sync_icons(check_only=False):
    """Sync icon data from icons.json into all downstream files."""
    data = load_json(ICONS_JSON)
    assert_icon_data_valid(data)
    dirty = []

    icons_h = ROOT / "components" / "espcontrol" / "icons.h"
    icon_glyphs = ROOT / "common" / "assets" / "icon_glyphs.yaml"
    web_icons = WEB_ICONS_TS

    patches = [
        (icon_glyphs, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_icon_glyphs),
        (icons_h, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_icons_h_entries),
        (icons_h, "GENERATED:DOMAIN_ICONS START", "GENERATED:DOMAIN_ICONS END", gen_icons_h_domain_icons),
        (web_icons, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_web_icon_module),
        (web_icons, "GENERATED:DOMAIN_ICONS START", "GENERATED:DOMAIN_ICONS END", gen_web_domain_icon_module),
    ]

    file_contents = {}
    for path, start_tag, end_tag, generator in patches:
        if path not in file_contents:
            file_contents[path] = path.read_text()
        old = file_contents[path]
        new_content = generator(data)
        updated = replace_between_markers(old, start_tag, end_tag, new_content)
        if updated != old:
            file_contents[path] = updated
            dirty.append((path.relative_to(ROOT), start_tag))

    if check_only:
        if dirty:
            print("Icon data is out of sync. Run 'python scripts/build.py icons' to fix:")
            for rel, tag in dirty:
                print(f"  {rel} ({tag})")
        return dirty

    for path, content in file_contents.items():
        original = path.read_text()
        if content != original:
            path.write_text(content)
            print(f"  updated {path.relative_to(ROOT)}")
    return dirty


# ===========================================================================
# www.js build (formerly build_www.py)
# ===========================================================================

WWW_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"
TIME_YAML = ROOT / "common" / "addon" / "time.yaml"


def build_web_devices():
    timezone_options = load_timezone_options()
    devices = {
        slug: web_config(profile)
        for slug, profile in load_device_profiles().items()
    }
    for cfg in devices.values():
        cfg["timezoneOptions"] = timezone_options
    return devices


def load_timezone_options():
    options = []
    for _line_no, option in load_timezone_select_options():
        if option == AUTO_TIMEZONE_OPTION or timezone_option_id(option) is not None:
            options.append(option)
    if not options:
        raise BuildError(f"No timezone options found in {TIME_YAML.relative_to(ROOT)}")
    return options


def build_www(check_only=False, output_dir=None, test_hooks=False):
    """Build per-device www.js from the single source template."""
    devices = build_web_devices()
    temporary_root = None
    if output_dir is None:
        temporary_root = tempfile.TemporaryDirectory(prefix="espcontrol-www-")
        build_root = Path(temporary_root.name)
    else:
        build_root = Path(output_dir).resolve()
        build_root.mkdir(parents=True, exist_ok=True)

    result = subprocess.run(
        ["node", str(ROOT / "scripts" / "build_web_bundle.js")],
        input=json.dumps({"outputDir": str(build_root), "devices": devices, "testHooks": test_hooks}),
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        if temporary_root:
            temporary_root.cleanup()
        raise BuildError(result.stderr.strip() or "esbuild failed while building web bundles")

    if output_dir is not None:
        print(f"Built {len(devices)} www.js bundle(s) in {build_root}")
        return []

    dirty = []

    for slug in devices:
        output_path = WWW_OUTPUT_DIR / slug / "www.js"
        generated = (build_root / slug / "www.js").read_text()

        if output_path.exists():
            current = output_path.read_text()
            if current == generated:
                continue

        dirty.append(slug)

        if not check_only:
            output_path.parent.mkdir(parents=True, exist_ok=True)
            output_path.write_text(generated)
            print(f"  updated docs/public/webserver/{slug}/www.js")

    if check_only and dirty:
        print("www.js outputs are out of date. Run 'python scripts/build.py www' to fix:")
        for slug in dirty:
            print(f"  docs/public/webserver/{slug}/www.js")
    if temporary_root:
        temporary_root.cleanup()
    return dirty


# ===========================================================================
# Main
# ===========================================================================

def main():
    args = sys.argv[1:]
    check_only = "--check" in args
    test_hooks = "--test-hooks" in args
    args = [arg for arg in args if arg != "--test-hooks"]
    temporary_output = None
    if "--temporary-output" in args:
        index = args.index("--temporary-output")
        if index + 1 >= len(args):
            raise BuildError("--temporary-output requires a directory")
        temporary_output = args[index + 1]
        del args[index:index + 2]
    commands = [a for a in args if a != "--check"]

    if not commands:
        commands = ["all"]

    exit_code = 0

    try:
        for cmd in commands:
            if cmd == "all":
                entity_dirty = sync_entity_names(check_only=check_only)
                i18n_dirty = sync_i18n(check_only=check_only)
                contract_dirty = sync_card_contract(check_only=check_only)
                device_dirty = sync_device_capabilities(check_only=check_only)
                icon_dirty = sync_icons(check_only=check_only)
                www_dirty = build_www(check_only=check_only)
                if check_only and (entity_dirty or i18n_dirty or contract_dirty or device_dirty or icon_dirty or www_dirty):
                    exit_code = 1
                elif not entity_dirty and not i18n_dirty and not contract_dirty and not device_dirty and not icon_dirty and not www_dirty:
                    print("All outputs are up to date.")
                else:
                    total = (
                        len(entity_dirty) + len(i18n_dirty) + len(contract_dirty) + len(device_dirty) +
                        len(icon_dirty) + len(www_dirty)
                    )
                    print(f"Updated {total} target(s).")
            elif cmd == "entities":
                dirty = sync_entity_names(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("Entity name outputs are in sync.")
                else:
                    print(f"Synced {len(dirty)} entity name output(s).")
            elif cmd == "icons":
                dirty = sync_icons(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("Icon data is in sync.")
                else:
                    print(f"Synced {len(dirty)} section(s).")
            elif cmd == "i18n":
                dirty = sync_i18n(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("Firmware i18n output is in sync.")
                else:
                    print(f"Synced {len(dirty)} firmware i18n output(s).")
            elif cmd == "contract":
                dirty = sync_card_contract(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("Card contract outputs are in sync.")
                else:
                    print(f"Synced {len(dirty)} card contract output(s).")
            elif cmd == "devices":
                dirty = sync_device_capabilities(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("Device capability output is in sync.")
                else:
                    print(f"Synced {len(dirty)} device capability output(s).")
            elif cmd == "www":
                dirty = build_www(check_only=check_only, output_dir=temporary_output, test_hooks=test_hooks)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("All www.js outputs are up to date.")
                else:
                    print(f"Built {len(dirty)} file(s).")
            else:
                print(f"Unknown command: {cmd}")
                print("Usage: python scripts/build.py [all|entities|contract|devices|icons|i18n|www] [--check]")
                exit_code = 1
    except (BuildError, ProductSchemaError) as exc:
        print(exc)
        return 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
