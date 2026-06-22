#!/usr/bin/env python3
"""Unified build script for espcontrol.

Combines icon synchronization and www.js generation into a single tool.

Usage:
    python scripts/build.py               # run all generators
    python scripts/build.py --check       # exit 1 if any output is stale
    python scripts/build.py devices       # sync public device capabilities only
    python scripts/build.py icons         # sync icons only
    python scripts/build.py i18n          # sync firmware translations only
    python scripts/build.py model         # build generated web model only
    python scripts/build.py www           # build www.js only
    python scripts/build.py icons --check # check icons only
"""
import json
import re
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

from device_profiles import load_device_profiles, public_device_capabilities, web_config
from product_schema import ProductSchemaError, assert_card_contract_valid

ROOT = Path(__file__).resolve().parent.parent
MDI_VERSION = "7.4.47"
MDI_CSS_URL = f"https://cdn.jsdelivr.net/npm/@mdi/font@{MDI_VERSION}/css/materialdesignicons.css"

# ---------------------------------------------------------------------------
# Shared paths
# ---------------------------------------------------------------------------
ICONS_JSON = ROOT / "common" / "assets" / "icons.json"
ENTITY_NAMES_JSON = ROOT / "common" / "config" / "entity_names.json"
ENTITY_NAMES_YAML = ROOT / "common" / "config" / "entity_names.yaml"
ENTITY_NAMES_JS = ROOT / "src" / "webserver" / "modules" / "entity_catalog.js"
STRINGS_DIR = ROOT / "common" / "config"
I18N_GENERATED_H = ROOT / "components" / "espcontrol" / "i18n_generated.h"
CARD_CONTRACT_JSON = ROOT / "common" / "config" / "card_contract.json"
CARD_CONTRACT_JS = ROOT / "src" / "webserver" / "modules" / "card_contract_generated.js"
CARD_CONTRACT_H = ROOT / "components" / "espcontrol" / "button_grid_contract_generated.h"
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
        if object_ids and (not isinstance(object_ids, list) or not all(isinstance(v, str) and v for v in object_ids)):
            errors.append(f"{key}: objectIds must be a list of strings")
        groups = entry.get("groups", [])
        if groups and (not isinstance(groups, list) or not all(isinstance(v, str) and v for v in groups)):
            errors.append(f"{key}: groups must be a list of strings")

    for domain, names in names_by_domain.items():
        for name, entry_keys in names.items():
            if len(entry_keys) > 1:
                errors.append(f"duplicate entity name for {domain} {name!r}: {', '.join(entry_keys)}")
    return errors


def assert_entity_names_valid(data):
    errors = validate_entity_names(data)
    if not errors:
        return
    print("Entity name registry is invalid:")
    for error in errors:
        print(f"  {error}")
    raise BuildError("Entity name validation failed.")


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
        f"var ENTITY_CATALOG = {json_text};\n"
    )


def sync_entity_names(check_only=False):
    data = load_entity_names_data()
    assert_entity_names_valid(data)
    outputs = [
        (ENTITY_NAMES_YAML, gen_entity_names_yaml(data)),
        (ENTITY_NAMES_JS, gen_entity_names_js(data)),
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
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise BuildError(f"Invalid strings entry in {path.relative_to(ROOT)}:{line_no}")
        key, value = line.split("=", 1)
        key = key.strip()
        if not key:
            raise BuildError(f"Empty strings key in {path.relative_to(ROOT)}:{line_no}")
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
        for key, source in english.items():
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


def gen_card_contract_js(data):
    groups = data["cardGroups"]
    fan = groups["fan"]
    fan_default_icons = {card_type: cfg["defaultIcon"] for card_type, cfg in fan.items()}
    fan_default_icon_on = {card_type: cfg["defaultIconOn"] for card_type, cfg in fan.items() if cfg.get("defaultIconOn")}
    codes = data["subpageTypeCodes"]
    code_to_type = {code: card_type for card_type, code in codes.items()}
    large = data["largeNumbers"]
    cards = data["cards"]
    aliases = data.get("migrationAliases", {})
    return (
        "// =============================================================================\n"
        "// GENERATED CARD CONFIG CONTRACT - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// =============================================================================\n"
        f"var CARD_CONFIG_FIELDS = {json.dumps(data['fields'])};\n"
        f"var CARD_CONTRACT_CARDS = {json.dumps(cards, indent=2)};\n"
        f"var CARD_CONTRACT_MIGRATION_ALIASES = {json.dumps(aliases, indent=2)};\n"
        f"var CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES = {js_string_list(groups['brightnessSlider'])};\n"
        f"var CARD_CONTRACT_FAN_DEFAULT_ICONS = {json.dumps(fan_default_icons, indent=2)};\n"
        f"var CARD_CONTRACT_FAN_DEFAULT_ICON_ON = {json.dumps(fan_default_icon_on, indent=2)};\n"
        f"var CARD_CONTRACT_OPTION_SELECT_ACTION = {json.dumps(data['optionSelect']['canonicalAction'])};\n"
        f"var CARD_CONTRACT_OPTION_SELECT_ACTIONS = {js_string_list(data['optionSelect']['actions'])};\n"
        f"var CARD_CONTRACT_SUBPAGE_TYPE_CODES = {json.dumps(codes, indent=2)};\n"
        f"var CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE = {json.dumps(code_to_type, indent=2)};\n"
        f"var CARD_CONTRACT_LARGE_NUMBERS = {json.dumps(large, indent=2)};\n"
        "\n"
        "function cardContractListContains(list, value) {\n"
        "  return (list || []).indexOf(value) >= 0;\n"
        "}\n"
        "\n"
        "function cardContractCard(type) {\n"
        "  return CARD_CONTRACT_CARDS[type || \"\"] || null;\n"
        "}\n"
        "\n"
        "function cardContractCardKeys() {\n"
        "  return Object.keys(CARD_CONTRACT_CARDS);\n"
        "}\n"
        "\n"
        "function cardContractCardLabel(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  return card ? card.label : (type || \"Switch\");\n"
        "}\n"
        "\n"
        "function cardContractAllowInSubpage(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  return !!(card && card.allowInSubpage);\n"
        "}\n"
        "\n"
        "function cardContractPickerKey(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  return card && card.pickerKey ? card.pickerKey : \"\";\n"
        "}\n"
        "\n"
        "function cardContractHidden(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  return !!(card && card.hidden);\n"
        "}\n"
        "\n"
        "function cardContractOptions(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  return card && card.options ? JSON.parse(JSON.stringify(card.options)) : [];\n"
        "}\n"
        "\n"
        "function cardContractDefaultConfig(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  var defaults = card && card.default ? card.default : CARD_CONTRACT_CARDS[\"\"].default;\n"
        "  return Object.assign({}, defaults);\n"
        "}\n"
        "\n"
        "function cardContractDomains(type) {\n"
        "  var card = cardContractCard(type);\n"
        "  return card && card.domains ? card.domains.slice() : [];\n"
        "}\n"
        "\n"
        "function cardContractMigrationAlias(type) {\n"
        "  var alias = CARD_CONTRACT_MIGRATION_ALIASES[type || \"\"];\n"
        "  return alias ? Object.assign({}, alias) : null;\n"
        "}\n"
        "\n"
        "function cardContractIsBrightnessSliderType(type) {\n"
        "  return cardContractListContains(CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES, type);\n"
        "}\n"
        "\n"
        "function cardContractIsFanCardType(type) {\n"
        "  return Object.prototype.hasOwnProperty.call(CARD_CONTRACT_FAN_DEFAULT_ICONS, type || \"\");\n"
        "}\n"
        "\n"
        "function cardContractFanDefaultIcon(type) {\n"
        "  return CARD_CONTRACT_FAN_DEFAULT_ICONS[type] || CARD_CONTRACT_FAN_DEFAULT_ICONS.fan_speed || \"Fan Speed 2\";\n"
        "}\n"
        "\n"
        "function cardContractFanDefaultIconOn(type) {\n"
        "  return CARD_CONTRACT_FAN_DEFAULT_ICON_ON[type] || \"Auto\";\n"
        "}\n"
        "\n"
        "function cardContractIsOptionSelectType(type) {\n"
        "  return type === \"option_select\";\n"
        "}\n"
        "\n"
        "function cardContractIsOptionSelectAction(action) {\n"
        "  return cardContractListContains(CARD_CONTRACT_OPTION_SELECT_ACTIONS, action);\n"
        "}\n"
        "\n"
        "function cardContractSubpageTypeCode(type) {\n"
        "  return CARD_CONTRACT_SUBPAGE_TYPE_CODES[type || \"\"] || (type || \"\");\n"
        "}\n"
        "\n"
        "function cardContractSubpageTypeFromCode(code) {\n"
        "  return CARD_CONTRACT_SUBPAGE_TYPES_BY_CODE[code || \"\"] || (code || \"\");\n"
        "}\n"
        "\n"
        "function cardContractLargeNumbersSupported(type, precision) {\n"
        "  var rule = CARD_CONTRACT_LARGE_NUMBERS[type || \"\"];\n"
        "  if (rule === true) return true;\n"
        "  if (!rule) return false;\n"
        "  if (rule.excludedPrecisions) return !cardContractListContains(rule.excludedPrecisions, precision || \"\");\n"
        "  if (rule.precisions) return cardContractListContains(rule.precisions, precision || \"\");\n"
        "  return false;\n"
        "}\n"
    )


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
    lines = [
        "#pragma once\n",
        "\n",
        "// =============================================================================\n",
        "// GENERATED CARD CONFIG CONTRACT - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        "\n",
        f'constexpr const char *CARD_CONTRACT_OPTION_SELECT_ACTION = {json.dumps(data["optionSelect"]["canonicalAction"])};\n',
        cpp_string_array("CARD_CONTRACT_OPTION_SELECT_ACTIONS", option_actions),
        cpp_string_array("CARD_CONTRACT_BRIGHTNESS_SLIDER_TYPES", groups["brightnessSlider"]),
        cpp_string_array("CARD_CONTRACT_COVER_MODES", contract_card_option_values(cards, "cover", "cover_mode")),
        cpp_string_array("CARD_CONTRACT_GARAGE_MODES", contract_card_option_values(cards, "garage", "garage_mode")),
        cpp_string_array("CARD_CONTRACT_GARAGE_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "garage", "label_display")),
        cpp_string_array("CARD_CONTRACT_INTERNAL_MODES", contract_card_option_values(cards, "internal", "internal_mode")),
        cpp_string_array("CARD_CONTRACT_LOCK_MODES", contract_card_option_values(cards, "lock", "lock_mode")),
        cpp_string_array("CARD_CONTRACT_MEDIA_MODES", contract_card_option_values(cards, "media", "media_mode")),
        cpp_string_array("CARD_CONTRACT_MEDIA_DISPLAY_MODES", contract_card_option_values(cards, "media", "media_display")),
        cpp_string_array("CARD_CONTRACT_MEDIA_NOW_PLAYING_CONTROLS", contract_card_option_values(cards, "media", "media_now_playing_controls")),
        cpp_string_array("CARD_CONTRACT_MEDIA_LEGACY_MODES", media_behavior["legacyModes"].keys()),
        cpp_string_array("CARD_CONTRACT_MEDIA_STATE_DISPLAY_MODES", media_behavior["stateDisplayModes"]),
        cpp_string_array("CARD_CONTRACT_ALARM_ACTION_MODES", [item["value"] for item in alarm_behavior["actions"]]),
        cpp_string_array("CARD_CONTRACT_ALARM_ICON_DISPLAY_MODES", contract_card_option_values(cards, "alarm", "icon_display")),
        cpp_string_array("CARD_CONTRACT_ALARM_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "alarm", "label_display")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_MODES", contract_card_option_values(cards, "climate", "label_display")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_MODES", contract_card_option_values(cards, "climate", "number_display")),
        cpp_string_array("CARD_CONTRACT_CLIMATE_PRECISION_VALUES", climate_behavior["precisionValues"]),
        cpp_string_array("CARD_CONTRACT_WEATHER_FORECAST_PRECISIONS", large_numbers["weather"]["precisions"]),
        f'constexpr const char *CARD_CONTRACT_GARAGE_LABEL_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "garage", "label_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_MEDIA_DEFAULT_MODE = {json.dumps(media_behavior["defaultMode"])};\n',
        f'constexpr const char *CARD_CONTRACT_ALARM_ICON_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "alarm", "icon_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_ALARM_LABEL_DISPLAY_DEFAULT = {json.dumps(contract_card_option_default(cards, "alarm", "label_display"))};\n',
        f'constexpr const char *CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_DEFAULT = {json.dumps(climate_behavior["defaultLabelDisplay"])};\n',
        f'constexpr const char *CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_DEFAULT = {json.dumps(climate_behavior["defaultNumberDisplay"])};\n',
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
        (CARD_CONTRACT_JS, gen_card_contract_js(data)),
        (CARD_CONTRACT_H, gen_card_contract_h(data)),
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
        ROOT / "src" / "webserver" / "entry.js",
        ROOT / "common" / "assets" / "icons.yaml",
        *sorted(ROOT.glob("devices/*/device/fonts.yaml")),
    ]
    version_re = re.compile(
        r"(?:@mdi/font@|MaterialDesign-Webfont/raw/v|materialdesignicons\.com/cdn/)"
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


def gen_www_js_icon_map(data):
    """JS ICON_EXCEPTIONS + ICON_NAMES for the web entry bundle."""
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

    lines = ["  var ICON_EXCEPTIONS = {\n"]
    lines.extend(exceptions)
    lines.append("  };\n")
    lines.append("  var ICON_NAMES = [\n")
    for i in range(0, len(names), 6):
        chunk = names[i : i + 6]
        formatted = ", ".join(f'"{n}"' for n in chunk)
        lines.append(f"    {formatted},\n")
    lines.append("  ];\n")
    return "".join(lines)


def gen_www_js_domain_icons(data):
    """JS DOMAIN_ICONS object entries."""
    icon_by_name = {i["name"]: i for i in data["icons"]}
    lines = []
    for domain, icon_name in data["domain_defaults"].items():
        mdi = icon_by_name[icon_name]["mdi"]
        lines.append(f'    {domain}: "{mdi}",\n')
    return "".join(lines)


def sync_icons(check_only=False):
    """Sync icon data from icons.json into all downstream files."""
    data = load_json(ICONS_JSON)
    assert_icon_data_valid(data)
    dirty = []

    icons_h = ROOT / "components" / "espcontrol" / "icons.h"
    icon_glyphs = ROOT / "common" / "assets" / "icon_glyphs.yaml"
    web_entry = ROOT / "src" / "webserver" / "entry.js"

    patches = [
        (icon_glyphs, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_icon_glyphs),
        (icons_h, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_icons_h_entries),
        (icons_h, "GENERATED:DOMAIN_ICONS START", "GENERATED:DOMAIN_ICONS END", gen_icons_h_domain_icons),
        (web_entry, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_www_js_icon_map),
        (web_entry, "GENERATED:DOMAIN_ICONS START", "GENERATED:DOMAIN_ICONS END", gen_www_js_domain_icons),
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

WWW_SOURCE = ROOT / "src" / "webserver" / "entry.js"
MODULES_DIR = ROOT / "src" / "webserver" / "modules"
TYPES_DIR = ROOT / "src" / "webserver" / "types"
WWW_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"
WEB_MODULE_ORDER_PATH = ROOT / "scripts" / "web_modules.json"
MODEL_ENTRY = ROOT / "src" / "webserver" / "model" / "index.ts"
MODEL_GENERATED_JS = MODULES_DIR / "model_generated.js"
TIME_YAML = ROOT / "common" / "addon" / "time.yaml"

CONFIG_START = "__DEVICE_CONFIG_START__"
CONFIG_END = "__DEVICE_CONFIG_END__"
MODULES_START = "__WEB_MODULES_START__"
MODULES_END = "__WEB_MODULES_END__"
TYPES_START = "__BUTTON_TYPES_START__"
TYPES_END = "__BUTTON_TYPES_END__"

def load_web_module_order():
    order = load_json(WEB_MODULE_ORDER_PATH)
    if not isinstance(order, list) or not all(isinstance(name, str) and name for name in order):
        raise BuildError(f"Invalid web module order: {WEB_MODULE_ORDER_PATH.relative_to(ROOT)}")
    return order


def build_config_block(slug, cfg):
    cfg_lines = json.dumps(cfg, indent=2).splitlines()
    cfg_body = "\n".join("  " + line for line in cfg_lines[1:])
    return (
        f'  var DEVICE_ID = "{slug}";\n'
        f"  var CFG = {cfg_lines[0]}\n"
        f"{cfg_body};\n"
    )


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
    for match in re.finditer(r'^\s+- "([^"]+)"$', TIME_YAML.read_text(), re.M):
        option = match.group(1)
        if option == "Auto (Home Assistant)" or (
            " (GMT" in option and ("/" in option or option.startswith("UTC "))
        ):
            options.append(option)
    if not options:
        raise BuildError(f"No timezone options found in {TIME_YAML.relative_to(ROOT)}")
    return options


def load_button_types():
    if not TYPES_DIR.is_dir():
        return ""
    files = sorted(TYPES_DIR.glob("*.js"))
    if not files:
        return ""
    chunks = []
    for f in files:
        chunks.append(f"  // --- type: {f.stem} ---")
        for line in f.read_text().rstrip().splitlines():
            chunks.append(f"  {line}" if line.strip() else "")
    return "\n".join(chunks) + "\n"


def load_web_modules():
    chunks = []
    for name in load_web_module_order():
        path = MODULES_DIR / f"{name}.js"
        if not path.exists():
            raise BuildError(f"Missing web module: {path.relative_to(ROOT)}")
        chunks.append(f"  // --- module: {name} ---")
        for line in path.read_text().rstrip().splitlines():
            chunks.append(f"  {line}" if line.strip() else "")
    return "\n".join(chunks) + "\n"


def replace_marked_block(source_text, start_tag, end_tag, new_content):
    pattern = re.compile(
        r"(^[^\n]*" + re.escape(start_tag) + r"[^\n]*\n)"
        r"(.*?)"
        r"(^[^\n]*" + re.escape(end_tag) + r"[^\n]*$)",
        re.MULTILINE | re.DOTALL,
    )
    m = pattern.search(source_text)
    if not m:
        return None
    return source_text[: m.start(2)] + new_content + source_text[m.start(3) :]


def replace_types(source_text):
    replaced = replace_marked_block(source_text, TYPES_START, TYPES_END, load_button_types())
    if replaced is None:
        return source_text
    return replaced


def replace_modules(source_text):
    replaced = replace_marked_block(source_text, MODULES_START, MODULES_END, load_web_modules())
    if replaced is None:
        raise ValueError(f"Module markers not found: {MODULES_START} / {MODULES_END}")
    return replaced


def replace_config(source_text, slug, cfg):
    pattern = re.compile(
        r"(^[^\n]*" + re.escape(CONFIG_START) + r"[^\n]*\n)"
        r"(.*?)"
        r"(^[^\n]*" + re.escape(CONFIG_END) + r"[^\n]*$)",
        re.MULTILINE | re.DOTALL,
    )
    m = pattern.search(source_text)
    if not m:
        raise ValueError(f"Config markers not found: {CONFIG_START} / {CONFIG_END}")
    return source_text[: m.start(2)] + build_config_block(slug, cfg) + source_text[m.start(3) :]


def esbuild_cmd():
    """Return an esbuild command path, preferring the repo-installed binary."""
    local = ROOT / "node_modules" / ".bin" / ("esbuild.cmd" if sys.platform == "win32" else "esbuild")
    if local.exists():
        return str(local)
    found = shutil.which("esbuild")
    if found:
        return found
    raise RuntimeError("esbuild not found. Run 'npm ci' before building www.js outputs.")


def minify_js(source_text):
    """Minify generated web UI JavaScript with esbuild."""
    result = subprocess.run(
        [esbuild_cmd(), "--loader=js", "--minify"],
        input=source_text,
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "esbuild failed")
    return result.stdout


def build_model_generated_js():
    """Bundle TypeScript model helpers into the web module namespace."""
    result = subprocess.run(
        [
            esbuild_cmd(),
            str(MODEL_ENTRY),
            "--bundle",
            "--format=iife",
            "--global-name=EspControlModel",
            "--target=es2020",
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        raise BuildError(result.stderr.strip() or "esbuild failed while building web model")
    return (
        "// =============================================================================\n"
        "// GENERATED WEB MODEL - do not edit by hand\n"
        "// Generated by scripts/build.py from src/webserver/model/index.ts.\n"
        "// =============================================================================\n"
        f"{result.stdout}"
    )


def sync_web_model(check_only=False):
    generated = build_model_generated_js()
    dirty = []
    if not MODEL_GENERATED_JS.exists() or MODEL_GENERATED_JS.read_text() != generated:
        dirty.append(MODEL_GENERATED_JS.relative_to(ROOT))

    if check_only:
        if dirty:
            print("Web model output is out of sync. Run 'python scripts/build.py model' to fix:")
            for rel in dirty:
                print(f"  {rel}")
        return dirty

    if dirty:
        MODEL_GENERATED_JS.parent.mkdir(parents=True, exist_ok=True)
        MODEL_GENERATED_JS.write_text(generated)
        print(f"  updated {MODEL_GENERATED_JS.relative_to(ROOT)}")
    return dirty


def build_www(check_only=False):
    """Build per-device www.js from the single source template."""
    devices = build_web_devices()
    source_text = WWW_SOURCE.read_text()
    source_text = replace_types(source_text)
    source_text = replace_modules(source_text)
    dirty = []

    for slug, cfg in devices.items():
        output_path = WWW_OUTPUT_DIR / slug / "www.js"
        generated = minify_js(replace_config(source_text, slug, cfg))

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
    return dirty


# ===========================================================================
# Main
# ===========================================================================

def main():
    args = sys.argv[1:]
    check_only = "--check" in args
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
                model_dirty = sync_web_model(check_only=check_only)
                www_dirty = build_www(check_only=check_only)
                if check_only and (entity_dirty or i18n_dirty or contract_dirty or device_dirty or icon_dirty or model_dirty or www_dirty):
                    exit_code = 1
                elif not entity_dirty and not i18n_dirty and not contract_dirty and not device_dirty and not icon_dirty and not model_dirty and not www_dirty:
                    print("All outputs are up to date.")
                else:
                    total = (
                        len(entity_dirty) + len(i18n_dirty) + len(contract_dirty) + len(device_dirty) +
                        len(icon_dirty) + len(model_dirty) + len(www_dirty)
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
            elif cmd == "model":
                dirty = sync_web_model(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("Web model output is in sync.")
                else:
                    print(f"Synced {len(dirty)} web model output(s).")
            elif cmd == "www":
                dirty = build_www(check_only=check_only)
                if check_only and dirty:
                    exit_code = 1
                elif not dirty:
                    print("All www.js outputs are up to date.")
                else:
                    print(f"Built {len(dirty)} file(s).")
            else:
                print(f"Unknown command: {cmd}")
                print("Usage: python scripts/build.py [all|entities|contract|devices|icons|i18n|model|www] [--check]")
                exit_code = 1
    except (BuildError, ProductSchemaError) as exc:
        print(exc)
        return 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
