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
    python scripts/build.py --self-test    # verify transactional publishing
"""
import json
import os
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
SAVED_CONFIG_VACUUM_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_vacuum.ts"
SAVED_CONFIG_VACUUM_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_vacuum_generated.h"
SAVED_CONFIG_SENSOR_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_sensor.ts"
SAVED_CONFIG_SENSOR_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_sensor_generated.h"
SAVED_CONFIG_ACTION_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_action.ts"
SAVED_CONFIG_ACTION_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_action_generated.h"
SAVED_CONFIG_MEDIA_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_media.ts"
SAVED_CONFIG_MEDIA_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_media_generated.h"
SAVED_CONFIG_STATIC_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_static.ts"
SAVED_CONFIG_STATIC_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_static_generated.h"
SAVED_CONFIG_FAN_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_fan.ts"
SAVED_CONFIG_FAN_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_fan_generated.h"
SAVED_CONFIG_DATE_TIME_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_date_time.ts"
SAVED_CONFIG_DATE_TIME_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_date_time_generated.h"
SAVED_CONFIG_MOWER_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_mower.ts"
SAVED_CONFIG_MOWER_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_mower_generated.h"
SAVED_CONFIG_OCCUPANCY_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_occupancy.ts"
SAVED_CONFIG_OCCUPANCY_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_occupancy_generated.h"
SAVED_CONFIG_ACCESS_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_access.ts"
SAVED_CONFIG_ACCESS_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_access_generated.h"
SAVED_CONFIG_SECURITY_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_security.ts"
SAVED_CONFIG_SECURITY_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_security_generated.h"
SAVED_CONFIG_WEATHER_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_weather.ts"
SAVED_CONFIG_WEATHER_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_weather_generated.h"
SAVED_CONFIG_IMAGE_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_image.ts"
SAVED_CONFIG_IMAGE_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_image_generated.h"
SAVED_CONFIG_CLIMATE_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_climate.ts"
SAVED_CONFIG_CLIMATE_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_climate_generated.h"
SAVED_CONFIG_LIGHT_CONTROL_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_light_control.ts"
SAVED_CONFIG_LIGHT_CONTROL_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_light_control_generated.h"
SAVED_CONFIG_WEBHOOK_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_webhook.ts"
SAVED_CONFIG_WEBHOOK_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_webhook_generated.h"
SAVED_CONFIG_SUBPAGE_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_subpage.ts"
SAVED_CONFIG_SUBPAGE_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_subpage_generated.h"
SAVED_CONFIG_SWITCH_TS = ROOT / "src" / "webserver" / "generated" / "saved_config_switch.ts"
SAVED_CONFIG_SWITCH_H = ROOT / "components" / "espcontrol" / "button_grid_saved_config_switch_generated.h"
CARD_DOCS_DIR = ROOT / "docs" / "generated" / "cards"
DEVICE_CAPABILITIES_JSON = ROOT / "docs" / "public" / "device-profiles.json"
DEVICE_DOCS_DIR = ROOT / "docs" / "generated" / "screens"


class BuildError(RuntimeError):
    pass


class GeneratedOutputTransaction:
    """Stage generated text and promote the complete set only after success."""

    def __init__(self, replace_file=os.replace):
        self._staged = {}
        self._replace_file = replace_file

    def stage_text(self, path, content):
        self._staged[Path(path).resolve()] = content

    def overlays(self):
        return {str(path): content for path, content in self._staged.items()}

    def commit(self):
        if not self._staged:
            return

        prepared = {}
        originals = {}
        replaced = []
        try:
            for path, content in self._staged.items():
                path.parent.mkdir(parents=True, exist_ok=True)
                target_mode = path.stat().st_mode & 0o777 if path.exists() else 0o644
                originals[path] = (path.read_bytes(), target_mode) if path.exists() else None
                with tempfile.NamedTemporaryFile(
                    mode="w",
                    encoding="utf-8",
                    dir=path.parent,
                    prefix=f".{path.name}.",
                    suffix=".tmp",
                    delete=False,
                ) as handle:
                    handle.write(content)
                    handle.flush()
                    os.fsync(handle.fileno())
                    os.chmod(handle.name, target_mode)
                    prepared[path] = Path(handle.name)

            for path, staged_path in prepared.items():
                self._replace_file(staged_path, path)
                replaced.append(path)
        except Exception as exc:
            for path in reversed(replaced):
                original = originals[path]
                if original is None:
                    path.unlink(missing_ok=True)
                    continue
                original_content, original_mode = original
                with tempfile.NamedTemporaryFile(
                    mode="wb",
                    dir=path.parent,
                    prefix=f".{path.name}.rollback.",
                    suffix=".tmp",
                    delete=False,
                ) as handle:
                    handle.write(original_content)
                    handle.flush()
                    os.fsync(handle.fileno())
                    os.chmod(handle.name, original_mode)
                    rollback_path = Path(handle.name)
                os.replace(rollback_path, path)
            raise BuildError(f"Unable to publish generated outputs; restored the previous set: {exc}") from exc
        finally:
            for staged_path in prepared.values():
                staged_path.unlink(missing_ok=True)


GENERATED_TRANSACTION = None


def write_generated_text(path, content):
    if GENERATED_TRANSACTION is None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        action = "updated"
    else:
        GENERATED_TRANSACTION.stage_text(path, content)
        action = "staged"
    print(f"  {action} {path.relative_to(ROOT)}")


def run_generated_transaction_self_test():
    with tempfile.TemporaryDirectory(prefix="espcontrol-generated-transaction-") as directory:
        root = Path(directory)
        first = root / "first.txt"
        second = root / "second.txt"
        third = root / "third.txt"
        first.write_text("first-old", encoding="utf-8")
        second.write_text("second-old", encoding="utf-8")
        first.chmod(0o640)

        transaction = GeneratedOutputTransaction()
        transaction.stage_text(first, "first-new")
        transaction.stage_text(second, "second-new")
        transaction.stage_text(third, "third-new")
        if first.read_text(encoding="utf-8") != "first-old" or second.read_text(encoding="utf-8") != "second-old":
            raise BuildError("Generated transaction changed files before commit")
        transaction.commit()
        if first.read_text(encoding="utf-8") != "first-new" or second.read_text(encoding="utf-8") != "second-new":
            raise BuildError("Generated transaction did not publish the complete set")
        if first.stat().st_mode & 0o777 != 0o640 or third.stat().st_mode & 0o777 != 0o644:
            raise BuildError("Generated transaction did not preserve safe file permissions")

        replacements = 0

        def fail_second_replace(source, destination):
            nonlocal replacements
            replacements += 1
            if replacements == 2:
                raise OSError("simulated publish failure")
            os.replace(source, destination)

        transaction = GeneratedOutputTransaction(replace_file=fail_second_replace)
        transaction.stage_text(first, "first-broken")
        transaction.stage_text(second, "second-broken")
        try:
            transaction.commit()
        except BuildError:
            pass
        else:
            raise BuildError("Generated transaction self-test did not exercise rollback")
        if first.read_text(encoding="utf-8") != "first-new" or second.read_text(encoding="utf-8") != "second-new":
            raise BuildError("Generated transaction did not restore the previous set")
        if first.stat().st_mode & 0o777 != 0o640:
            raise BuildError("Generated transaction rollback did not restore file permissions")

        marker = "espcontrol-generated-overlay-self-test"
        entry_path = ROOT / "src" / "webserver" / "entry.ts"
        entry_overlay = entry_path.read_text(encoding="utf-8") + (
            f'\n(globalThis as Record<string, unknown>)["{marker}"] = true;\n'
        )
        slug, config = next(iter(build_web_devices().items()))
        bundle_root = root / "bundle"
        result = subprocess.run(
            ["node", str(ROOT / "scripts" / "build_web_bundle.js")],
            input=json.dumps({
                "outputDir": str(bundle_root),
                "devices": {slug: config},
                "testHooks": False,
                "overlays": {str(entry_path): entry_overlay},
            }),
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            raise BuildError(result.stderr.strip() or "Generated overlay self-test could not build a web bundle")
        bundle = (bundle_root / "www.js").read_text(encoding="utf-8")
        if marker not in bundle:
            raise BuildError("Web bundle did not consume the staged generated overlay")

    print("Generated output transaction self-test passed.")


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
        write_generated_text(path, content)
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
        write_generated_text(I18N_GENERATED_H, generated)
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
        "import type { CardConfig, CardOptionSpec, CardRuntimeSpec, CardTypeSpec, MigrationActionSpec, ResolvedCardRuntimeSpec, SavedConfigField } from \"../contracts/types\";\n"
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
        f"export const CARD_RUNTIME_SPECS: Readonly<Record<string, CardRuntimeSpec>> = {json.dumps(data['runtime']['specs'], indent=2)};\n"
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
        "export function cardRuntimeSpec(type: string | null | undefined): CardRuntimeSpec | null {\n"
        "  return CARD_RUNTIME_SPECS[type || \"\"] || null;\n"
        "}\n"
        "\n"
        "export function resolveCardRuntimeSpec(config: CardConfig): ResolvedCardRuntimeSpec | null {\n"
        "  const spec = cardRuntimeSpec(config.type);\n"
        "  if (!spec) return null;\n"
        "  let driver = spec.driver;\n"
        "  if (spec.modeField && spec.modes) {\n"
        "    driver = spec.modes[config[spec.modeField] || \"\"] || spec.defaultDriver || driver;\n"
        "  }\n"
        "  return Object.assign({}, spec, { driver });\n"
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


def saved_config_vacuum_field_rules(data):
    fields = data["cards"]["vacuum"]["normalization"]["fields"]
    sensor = fields["sensor"]
    icon_on = fields["icon_on"]
    if sensor.get("policy") != "allowed" or not sensor.get("values") or not isinstance(sensor.get("fallback"), str):
        raise BuildError("vacuum production sensor normalization requires authored allowed values and fallback")
    if icon_on.get("policy") != "default" or not isinstance(icon_on.get("value"), str):
        raise BuildError("vacuum production icon_on normalization requires the authored default policy")
    for field in ("precision", "options"):
        if fields[field].get("policy") != "clear":
            raise BuildError(f"vacuum production {field} normalization requires the authored clear policy")
    return sensor, icon_on


def saved_config_vacuum_migrations(data):
    names = data["cards"]["vacuum"]["normalization"].get("migrationActions", [])
    migrations = []
    for name in names:
        action = data["migrationActions"].get(name)
        if not action or action.get("hook") != "normalize_vacuum_fields":
            raise BuildError(f"vacuum production migration {name} requires the authored vacuum hook")
        for condition in action.get("when", []):
            if condition.get("source") != "field" or condition.get("operator") != "equals":
                raise BuildError(f"vacuum production migration {name} has an unsupported condition")
        migrations.append((name, action))
    if not migrations:
        raise BuildError("vacuum production normalization requires authored migration actions")
    return migrations


def saved_config_sensor_field_migrations(data):
    migrations = []
    for name in data["cards"]["sensor"]["normalization"].get("migrationActions", []):
        action = data["migrationActions"].get(name)
        if not action or action.get("hook") != "normalize_sensor_fields":
            continue
        conditions = action.get("when", [])
        if any(condition.get("source") != "field" or condition.get("operator") != "equals" for condition in conditions):
            raise BuildError(f"sensor production migration {name} has an unsupported field condition")
        migrations.append((name, action))
    if not migrations:
        raise BuildError("sensor production normalization requires an authored field migration")
    return migrations


def saved_config_action_field_migrations(data):
    migrations = []
    for name in data["cards"]["action"]["normalization"].get("migrationActions", []):
        action = data["migrationActions"].get(name)
        if not action or action.get("hook") != "normalize_action_fields":
            continue
        conditions = action.get("when", [])
        if any(condition.get("source") != "field" or condition.get("operator") != "equals" for condition in conditions):
            raise BuildError(f"action production migration {name} has an unsupported field condition")
        migrations.append((name, action))
    if not migrations:
        raise BuildError("action production normalization requires an authored field migration")
    return migrations


def saved_config_action_normalization(data):
    normalization = data["cards"]["action"]["normalization"]
    fields = normalization["fields"]
    field_hooks = {
        rule.get("hook")
        for name, rule in fields.items()
        if name != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_action_fields"}:
        raise BuildError("action production field normalization requires the authored Action field hook")
    option_rule = fields.get("options", {})
    option_hook = normalization.get("optionHook")
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != option_hook:
        raise BuildError("action production option normalization requires the authored option hook")
    if option_hook != "normalize_action_options":
        raise BuildError("action production normalization requires the authored Action option hook")
    return "action"


def saved_config_media_normalization(data):
    normalization = data["cards"]["media"]["normalization"]
    fields = normalization["fields"]
    type_rule = fields.get("type", {})
    if type_rule.get("policy") != "default" or not isinstance(type_rule.get("value"), str):
        raise BuildError("media production normalization requires an authored default type")
    field_hooks = {
        rule.get("hook")
        for name, rule in fields.items()
        if name != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_media_fields"}:
        raise BuildError("media production field normalization requires the authored Media field hook")
    option_rule = fields.get("options", {})
    option_hook = normalization.get("optionHook")
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != option_hook:
        raise BuildError("media production option normalization requires the authored option hook")
    if option_hook != "normalize_media_options":
        raise BuildError("media production normalization requires the authored Media option hook")
    return type_rule["value"]


def saved_config_sensor_normalization(data):
    normalization = data["cards"]["sensor"]["normalization"]
    fields = normalization["fields"]
    type_rule = fields.get("type", {})
    if type_rule.get("policy") != "default" or not isinstance(type_rule.get("value"), str):
        raise BuildError("sensor production normalization requires an authored default type")
    field_hooks = {
        rule.get("hook")
        for name, rule in fields.items()
        if name != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_sensor_fields"}:
        raise BuildError("sensor production field normalization requires the authored Sensor field hook")
    option_rule = fields.get("options", {})
    option_hook = normalization.get("optionHook")
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != option_hook:
        raise BuildError("sensor production option normalization requires the authored option hook")
    if option_hook != "normalize_sensor_options":
        raise BuildError("sensor production normalization requires the authored Sensor option hook")
    return type_rule["value"]


def saved_config_migration_condition(condition, target, language):
    name = condition["name"]
    value = json.dumps(condition["value"], ensure_ascii=False)
    operator = "===" if language == "ts" else "=="
    expression = f"{target}.{name} {operator} {value}"
    return f"!({expression})" if condition.get("negate") else expression


def gen_saved_config_vacuum_ts(data):
    sensor, icon_on = saved_config_vacuum_field_rules(data)
    migrations = saved_config_vacuum_migrations(data)
    sensor_values = json.dumps(sensor["values"], ensure_ascii=False)
    sensor_aliases = json.dumps(sensor.get("aliases", {}), ensure_ascii=False)
    fallback = json.dumps(sensor["fallback"], ensure_ascii=False)
    icon_on_default = json.dumps(icon_on["value"], ensure_ascii=False)
    migration_lines = ["export function migrateSavedConfigVacuumLegacy(config: CardConfig): boolean {\n"]
    for _, action in migrations:
        conditions = " && ".join(saved_config_migration_condition(condition, "config", "ts") for condition in action["when"])
        migration_lines.append(f"  if ({conditions}) {{\n")
        for field, value in action["set"].items():
            migration_lines.append(f"    config.{field} = {json.dumps(value, ensure_ascii=False)};\n")
        migration_lines.append("    return true;\n  }\n")
    migration_lines.append("  return false;\n}\n\n")
    return (
        "// =============================================================================\n"
        "// GENERATED SAVED-CONFIG VACUUM HELPERS - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// =============================================================================\n"
        "import type { CardConfig } from \"../contracts/types\";\n\n"
        + "".join(migration_lines)
        +
        f"const SAVED_CONFIG_VACUUM_SENSOR_VALUES = new Set<string>({sensor_values});\n"
        f"const SAVED_CONFIG_VACUUM_SENSOR_ALIASES: Readonly<Record<string, string>> = {sensor_aliases};\n\n"
        "export function normalizeSavedConfigVacuumSensor(sensor: string): string {\n"
        "  sensor = SAVED_CONFIG_VACUUM_SENSOR_ALIASES[sensor] || sensor;\n"
        f"  return SAVED_CONFIG_VACUUM_SENSOR_VALUES.has(sensor) ? sensor : {fallback};\n"
        "}\n\n"
        "export function normalizeSavedConfigVacuumIconOn(_iconOn: string): string {\n"
        f"  return {icon_on_default};\n"
        "}\n\n"
        "export function normalizeSavedConfigVacuumPrecision(_precision: string): string {\n"
        "  return \"\";\n"
        "}\n\n"
        "export function normalizeSavedConfigVacuumOptions(_options: string): string {\n"
        "  return \"\";\n"
        "}\n"
    )


def gen_saved_config_vacuum_h(data):
    sensor, icon_on = saved_config_vacuum_field_rules(data)
    migrations = saved_config_vacuum_migrations(data)
    sensor_aliases = sensor.get("aliases", {})
    allowed = " || ".join(f"sensor == {json.dumps(value, ensure_ascii=False)}" for value in sensor["values"])
    fallback = json.dumps(sensor["fallback"], ensure_ascii=False)
    icon_on_default = json.dumps(icon_on["value"], ensure_ascii=False)
    migration_lines = ["template<typename Config>\ninline bool migrate_saved_config_vacuum_legacy(Config &config) {\n"]
    for _, action in migrations:
        conditions = " && ".join(saved_config_migration_condition(condition, "config", "cpp") for condition in action["when"])
        migration_lines.append(f"  if ({conditions}) {{\n")
        for field, value in action["set"].items():
            migration_lines.append(f"    config.{field} = {json.dumps(value, ensure_ascii=False)};\n")
        migration_lines.append("    return true;\n  }\n")
    migration_lines.append("  return false;\n}\n\n")
    return (
        "#pragma once\n\n"
        "// =============================================================================\n"
        "// GENERATED SAVED-CONFIG VACUUM HELPERS - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// =============================================================================\n\n"
        "#include <string>\n\n"
        + "".join(migration_lines)
        +
        "inline std::string normalize_saved_config_vacuum_sensor(const std::string &sensor) {\n"
        + "".join(f"  if (sensor == {json.dumps(alias, ensure_ascii=False)}) return {json.dumps(target, ensure_ascii=False)};\n" for alias, target in sensor_aliases.items())
        +
        f"  return {allowed} ? sensor : {fallback};\n"
        "}\n\n"
        "inline std::string normalize_saved_config_vacuum_icon_on(const std::string &) {\n"
        f"  return {icon_on_default};\n"
        "}\n\n"
        "inline std::string normalize_saved_config_vacuum_precision(const std::string &) {\n"
        "  return \"\";\n"
        "}\n\n"
        "inline std::string normalize_saved_config_vacuum_options(const std::string &) {\n"
        "  return \"\";\n"
        "}\n"
    )


def gen_saved_config_sensor_ts(data):
    migrations = saved_config_sensor_field_migrations(data)
    sensor_type = json.dumps(saved_config_sensor_normalization(data), ensure_ascii=False)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SENSOR HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export function migrateSavedConfigSensorLegacy(config: CardConfig): boolean {\n",
    ]
    for _, action in migrations:
        conditions = " && ".join(saved_config_migration_condition(condition, "config", "ts") for condition in action["when"])
        lines.append(f"  if ({conditions}) {{\n")
        for field, value in action["set"].items():
            lines.append(f"    config.{field} = {json.dumps(value, ensure_ascii=False)};\n")
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n\n")
    lines.extend([
        "export type SavedConfigSensorFieldHook = (config: CardConfig, wasLegacyTextSensor: boolean) => void;\n",
        "export type SavedConfigSensorOptionHook = (options: string, precision: string) => string;\n\n",
        "export function normalizeSavedConfigSensor(\n",
        "  config: CardConfig,\n",
        "  wasLegacyTextSensor: boolean,\n",
        "  normalizeFields: SavedConfigSensorFieldHook,\n",
        "  normalizeOptions: SavedConfigSensorOptionHook,\n",
        "): boolean {\n",
        f"  if (config.type !== {sensor_type}) return false;\n",
        "  normalizeFields(config, wasLegacyTextSensor);\n",
        "  config.options = normalizeOptions(config.options || \"\", config.precision || \"\");\n",
        "  return true;\n",
        "}\n",
    ])
    return "".join(lines)


def gen_saved_config_sensor_h(data):
    migrations = saved_config_sensor_field_migrations(data)
    sensor_type = json.dumps(saved_config_sensor_normalization(data), ensure_ascii=False)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SENSOR HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config>\ninline bool migrate_saved_config_sensor_legacy(Config &config) {\n",
    ]
    for _, action in migrations:
        conditions = " && ".join(saved_config_migration_condition(condition, "config", "cpp") for condition in action["when"])
        lines.append(f"  if ({conditions}) {{\n")
        for field, value in action["set"].items():
            lines.append(f"    config.{field} = {json.dumps(value, ensure_ascii=False)};\n")
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n\n")
    lines.extend([
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_sensor(Config &config, bool was_legacy_text_sensor,\n",
        "                                          FieldHook normalize_fields, OptionHook normalize_options) {\n",
        f"  if (config.type != {sensor_type}) return false;\n",
        "  normalize_fields(config, was_legacy_text_sensor);\n",
        "  config.options = normalize_options(config.options, config.precision);\n",
        "  return true;\n",
        "}\n",
    ])
    return "".join(lines)


def gen_saved_config_action_ts(data):
    migrations = saved_config_action_field_migrations(data)
    action_type = json.dumps(saved_config_action_normalization(data), ensure_ascii=False)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG ACTION HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export function migrateSavedConfigActionLegacy(config: CardConfig): boolean {\n",
    ]
    for _, action in migrations:
        conditions = " && ".join(saved_config_migration_condition(condition, "config", "ts") for condition in action["when"])
        lines.append(f"  if ({conditions}) {{\n")
        for field, value in action["set"].items():
            lines.append(f"    config.{field} = {json.dumps(value, ensure_ascii=False)};\n")
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n\n")
    lines.extend([
        "export type SavedConfigActionFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigActionOptionHook = (options: string, action: string) => string;\n\n",
        "export function normalizeSavedConfigAction(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigActionFieldHook,\n",
        "  normalizeOptions: SavedConfigActionOptionHook,\n",
        "): boolean {\n",
        f"  if (config.type !== {action_type}) return false;\n",
        "  normalizeFields(config);\n",
        "  config.options = normalizeOptions(config.options || \"\", config.sensor || \"\");\n",
        "  return true;\n",
        "}\n",
    ])
    return "".join(lines)


def gen_saved_config_action_h(data):
    migrations = saved_config_action_field_migrations(data)
    action_type = json.dumps(saved_config_action_normalization(data), ensure_ascii=False)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG ACTION HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config>\ninline bool migrate_saved_config_action_legacy(Config &config) {\n",
    ]
    for _, action in migrations:
        conditions = " && ".join(saved_config_migration_condition(condition, "config", "cpp") for condition in action["when"])
        lines.append(f"  if ({conditions}) {{\n")
        for field, value in action["set"].items():
            if isinstance(value, str) and value == "":
                lines.append(f"    config.{field}.clear();\n")
            else:
                lines.append(f"    config.{field} = {json.dumps(value, ensure_ascii=False)};\n")
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n\n")
    lines.extend([
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_action(Config &config, FieldHook normalize_fields,\n",
        "                                          OptionHook normalize_options) {\n",
        f"  if (config.type != {action_type}) return false;\n",
        "  normalize_fields(config);\n",
        "  config.options = normalize_options(config.options, config.sensor);\n",
        "  return true;\n",
        "}\n",
    ])
    return "".join(lines)


def gen_saved_config_media_ts(data):
    media_type = json.dumps(saved_config_media_normalization(data), ensure_ascii=False)
    return (
        "// =============================================================================\n"
        "// GENERATED SAVED-CONFIG MEDIA HELPERS - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// =============================================================================\n"
        'import type { CardConfig } from "../contracts/types";\n\n'
        "export type SavedConfigMediaFieldHook = (config: CardConfig) => void;\n"
        "export type SavedConfigMediaOptionHook = (options: string, mode: string) => string;\n\n"
        "export function normalizeSavedConfigMedia(\n"
        "  config: CardConfig,\n"
        "  normalizeFields: SavedConfigMediaFieldHook,\n"
        "  normalizeOptions: SavedConfigMediaOptionHook,\n"
        "): boolean {\n"
        f"  if (config.type !== {media_type}) return false;\n"
        "  normalizeFields(config);\n"
        "  config.options = normalizeOptions(config.options || \"\", config.sensor || \"\");\n"
        "  return true;\n"
        "}\n"
    )


def gen_saved_config_media_h(data):
    media_type = json.dumps(saved_config_media_normalization(data), ensure_ascii=False)
    return (
        "#pragma once\n\n"
        "// =============================================================================\n"
        "// GENERATED SAVED-CONFIG MEDIA HELPERS - do not edit by hand\n"
        "// Generated by scripts/build.py from common/config/card_contract.json.\n"
        "// =============================================================================\n\n"
        "template<typename Config, typename FieldHook, typename OptionHook>\n"
        "inline bool normalize_saved_config_media(Config &config, FieldHook normalize_fields,\n"
        "                                         OptionHook normalize_options) {\n"
        f"  if (config.type != {media_type}) return false;\n"
        "  normalize_fields(config);\n"
        "  config.options = normalize_options(config.options, config.sensor);\n"
        "  return true;\n"
        "}\n"
    )


SAVED_CONFIG_STATIC_CARD_TYPES = (
    "internal",
    "light_brightness",
    "light_switch",
    "light_temperature",
    "push",
    "screen_lock",
    "screensaver",
    "slider",
)


def saved_config_static_normalizations(data):
    normalizations = []
    for card_type in SAVED_CONFIG_STATIC_CARD_TYPES:
        normalization = data["cards"][card_type].get("normalization")
        if not normalization:
            raise BuildError(f"static saved-config card {card_type} requires authored normalization")
        if normalization.get("migrationActions") or normalization.get("optionHook"):
            raise BuildError(f"static saved-config card {card_type} must be fully declarative")
        hooks = [
            field for field, rule in normalization["fields"].items()
            if rule.get("policy") == "hook"
        ]
        if hooks:
            raise BuildError(f"static saved-config card {card_type} has unsupported hooks: {', '.join(hooks)}")
        normalizations.append((card_type, normalization))
    return normalizations


def saved_config_declarative_field_lines(field, rule, language):
    policy = rule["policy"]
    target = f"config.{field}"
    if policy == "keep":
        return []
    if policy == "clear":
        return [f"    {target} = \"\";\n"] if language == "ts" else [f"    {target}.clear();\n"]
    if policy == "default":
        return [f"    {target} = {json.dumps(rule['value'], ensure_ascii=False)};\n"]
    if policy == "default_if_empty":
        value = json.dumps(rule["value"], ensure_ascii=False)
        if language == "ts":
            return [f"    if (!{target}) {target} = {value};\n"]
        return [f"    if ({target}.empty()) {target} = {value};\n"]
    if policy in ("alias", "allowed"):
        lines = []
        for alias, value in rule.get("aliases", {}).items():
            operator = "===" if language == "ts" else "=="
            lines.append(
                f"    if ({target} {operator} {json.dumps(alias, ensure_ascii=False)}) "
                f"{target} = {json.dumps(value, ensure_ascii=False)};\n"
            )
        if policy == "allowed":
            operator = "!==" if language == "ts" else "!="
            invalid = " && ".join(
                f"{target} {operator} {json.dumps(value, ensure_ascii=False)}"
                for value in rule["values"]
            )
            lines.append(f"    if ({invalid}) {target} = {json.dumps(rule['fallback'], ensure_ascii=False)};\n")
        return lines
    raise BuildError(f"unsupported declarative saved-config policy {policy!r} for {field}")


def gen_saved_config_static_ts(data):
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG STATIC CARD HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export function normalizeSavedConfigStatic(config: CardConfig): boolean {\n",
    ]
    for card_type, normalization in saved_config_static_normalizations(data):
        lines.append(f"  if (config.type === {json.dumps(card_type)}) {{\n")
        for field in data["fields"]:
            lines.extend(saved_config_declarative_field_lines(field, normalization["fields"][field], "ts"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_static_h(data):
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG STATIC CARD HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config>\n",
        "inline bool normalize_saved_config_static(Config &config) {\n",
    ]
    for card_type, normalization in saved_config_static_normalizations(data):
        lines.append(f"  if (config.type == {json.dumps(card_type)}) {{\n")
        for field in data["fields"]:
            lines.extend(saved_config_declarative_field_lines(field, normalization["fields"][field], "cpp"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


SAVED_CONFIG_FAN_CARD_TYPES = (
    "fan_direction",
    "fan_oscillate",
    "fan_preset",
    "fan_speed",
    "fan_control",
    "fan_switch",
)


def saved_config_fan_normalizations(data):
    normalizations = []
    for card_type in SAVED_CONFIG_FAN_CARD_TYPES:
        normalization = data["cards"][card_type].get("normalization")
        if not normalization:
            raise BuildError(f"fan saved-config card {card_type} requires authored normalization")
        field_hooks = {
            rule.get("hook") for field, rule in normalization["fields"].items()
            if field != "options" and rule.get("policy") == "hook"
        }
        if field_hooks != {"normalize_fan_fields"}:
            raise BuildError(f"fan saved-config card {card_type} requires the authored Fan field hook")
        option_rule = normalization["fields"]["options"]
        if card_type == "fan_control":
            if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_fan_options":
                raise BuildError("fan_control saved-config normalization requires the authored Fan option hook")
            if normalization.get("optionHook") != "normalize_fan_options":
                raise BuildError("fan_control saved-config optionHook must name normalize_fan_options")
        elif option_rule.get("policy") != "clear" or normalization.get("optionHook") is not None:
            raise BuildError(f"fan saved-config card {card_type} must declaratively clear options")
        normalizations.append((card_type, normalization))
    return normalizations


def gen_saved_config_fan_ts(data):
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG FAN HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigFanFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigFanOptionHook = (options: string) => string;\n\n",
        "export function normalizeSavedConfigFan(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigFanFieldHook,\n",
        "  normalizeOptions: SavedConfigFanOptionHook,\n",
        "): boolean {\n",
    ]
    for card_type, normalization in saved_config_fan_normalizations(data):
        lines.append(f"  if (config.type === {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_fan_fields":
                    if not field_hook_called:
                        lines.append("    normalizeFields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_fan_options":
                    lines.append("    config.options = normalizeOptions(config.options || \"\");\n")
                else:
                    raise BuildError(f"unsupported Fan saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_fan_h(data):
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG FAN HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_fan(Config &config, FieldHook normalize_fields,\n",
        "                                      OptionHook normalize_options) {\n",
    ]
    for card_type, normalization in saved_config_fan_normalizations(data):
        lines.append(f"  if (config.type == {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_fan_fields":
                    if not field_hook_called:
                        lines.append("    normalize_fields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_fan_options":
                    lines.append("    config.options = normalize_options(config.options);\n")
                else:
                    raise BuildError(f"unsupported Fan saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


SAVED_CONFIG_DATE_TIME_CARD_TYPES = ("calendar", "clock", "timezone")


def saved_config_date_time_normalizations(data):
    normalizations = []
    for card_type in SAVED_CONFIG_DATE_TIME_CARD_TYPES:
        normalization = data["cards"][card_type].get("normalization")
        if not normalization:
            raise BuildError(f"date/time saved-config card {card_type} requires authored normalization")
        field_hooks = {
            rule.get("hook") for field, rule in normalization["fields"].items()
            if field != "options" and rule.get("policy") == "hook"
        }
        expected_hooks = set() if card_type == "clock" else {"normalize_date_time_fields"}
        if field_hooks != expected_hooks:
            raise BuildError(f"date/time saved-config card {card_type} has unexpected field hooks")
        option_rule = normalization["fields"]["options"]
        if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_date_time_options":
            raise BuildError(f"date/time saved-config card {card_type} requires the authored option hook")
        if normalization.get("optionHook") != "normalize_date_time_options":
            raise BuildError(f"date/time saved-config card {card_type} has an unexpected optionHook")
        normalizations.append((card_type, normalization))
    return normalizations


def gen_saved_config_date_time_ts(data):
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG DATE/TIME HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigDateTimeFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigDateTimeOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigDateTime(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigDateTimeFieldHook,\n",
        "  normalizeOptions: SavedConfigDateTimeOptionHook,\n",
        "): boolean {\n",
    ]
    for card_type, normalization in saved_config_date_time_normalizations(data):
        lines.append(f"  if (config.type === {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_date_time_fields":
                    if not field_hook_called:
                        lines.append("    normalizeFields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_date_time_options":
                    lines.append("    config.options = normalizeOptions(config.options || \"\", config);\n")
                else:
                    raise BuildError(f"unsupported date/time saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_date_time_h(data):
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG DATE/TIME HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_date_time(Config &config, FieldHook normalize_fields,\n",
        "                                            OptionHook normalize_options) {\n",
    ]
    for card_type, normalization in saved_config_date_time_normalizations(data):
        lines.append(f"  if (config.type == {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_date_time_fields":
                    if not field_hook_called:
                        lines.append("    normalize_fields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_date_time_options":
                    lines.append("    config.options = normalize_options(config.options, config);\n")
                else:
                    raise BuildError(f"unsupported date/time saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def saved_config_mower_normalization(data):
    normalization = data["cards"]["lawn_mower"].get("normalization")
    if not normalization:
        raise BuildError("lawn mower saved-config card requires authored normalization")
    field_hooks = {
        rule.get("hook") for field, rule in normalization["fields"].items()
        if field != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_mower_fields"}:
        raise BuildError("lawn mower saved-config card requires the authored field hook")
    if normalization["fields"]["options"].get("policy") != "clear":
        raise BuildError("lawn mower saved-config card must declaratively clear options")
    if normalization.get("migrationActions") or normalization.get("optionHook"):
        raise BuildError("lawn mower saved-config card has unexpected migration or option hooks")
    return normalization


def gen_saved_config_mower_ts(data):
    normalization = saved_config_mower_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG LAWN MOWER HELPER - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigMowerFieldHook = (config: CardConfig) => void;\n\n",
        "export function normalizeSavedConfigMower(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigMowerFieldHook,\n",
        "): boolean {\n",
        '  if (config.type === "lawn_mower") {\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") != "normalize_mower_fields":
                raise BuildError(f"unsupported lawn mower saved-config hook {rule.get('hook')!r}")
            if not field_hook_called:
                lines.append("    normalizeFields(config);\n")
                field_hook_called = True
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("    return true;\n  }\n  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_mower_h(data):
    normalization = saved_config_mower_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG LAWN MOWER HELPER - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook>\n",
        "inline bool normalize_saved_config_mower(Config &config, FieldHook normalize_fields) {\n",
        '  if (config.type == "lawn_mower") {\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") != "normalize_mower_fields":
                raise BuildError(f"unsupported lawn mower saved-config hook {rule.get('hook')!r}")
            if not field_hook_called:
                lines.append("    normalize_fields(config);\n")
                field_hook_called = True
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("    return true;\n  }\n  return false;\n}\n")
    return "".join(lines)


SAVED_CONFIG_OCCUPANCY_CARD_TYPES = ("door_window", "presence")


def saved_config_occupancy_normalizations(data):
    normalizations = []
    for card_type in SAVED_CONFIG_OCCUPANCY_CARD_TYPES:
        normalization = data["cards"][card_type].get("normalization")
        if not normalization:
            raise BuildError(f"occupancy saved-config card {card_type} requires authored normalization")
        field_hooks = {
            rule.get("hook") for field, rule in normalization["fields"].items()
            if field != "options" and rule.get("policy") == "hook"
        }
        if field_hooks != {"normalize_occupancy_fields"}:
            raise BuildError(f"occupancy saved-config card {card_type} requires the authored field hook")
        option_rule = normalization["fields"]["options"]
        if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_occupancy_options":
            raise BuildError(f"occupancy saved-config card {card_type} requires the authored option hook")
        if normalization.get("optionHook") != "normalize_occupancy_options":
            raise BuildError(f"occupancy saved-config card {card_type} has an unexpected optionHook")
        normalizations.append((card_type, normalization))
    return normalizations


def gen_saved_config_occupancy_ts(data):
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG OCCUPANCY HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigOccupancyFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigOccupancyOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigOccupancy(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigOccupancyFieldHook,\n",
        "  normalizeOptions: SavedConfigOccupancyOptionHook,\n",
        "): boolean {\n",
    ]
    for card_type, normalization in saved_config_occupancy_normalizations(data):
        lines.append(f"  if (config.type === {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_occupancy_fields":
                    if not field_hook_called:
                        lines.append("    normalizeFields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_occupancy_options":
                    lines.append("    config.options = normalizeOptions(config.options || \"\", config);\n")
                else:
                    raise BuildError(f"unsupported occupancy saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_occupancy_h(data):
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG OCCUPANCY HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_occupancy(Config &config, FieldHook normalize_fields,\n",
        "                                             OptionHook normalize_options) {\n",
    ]
    for card_type, normalization in saved_config_occupancy_normalizations(data):
        lines.append(f"  if (config.type == {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_occupancy_fields":
                    if not field_hook_called:
                        lines.append("    normalize_fields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_occupancy_options":
                    lines.append("    config.options = normalize_options(config.options, config);\n")
                else:
                    raise BuildError(f"unsupported occupancy saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


SAVED_CONFIG_ACCESS_CARD_TYPES = ("cover", "garage", "gate", "lock")


def saved_config_access_normalizations(data):
    normalizations = []
    for card_type in SAVED_CONFIG_ACCESS_CARD_TYPES:
        normalization = data["cards"][card_type].get("normalization")
        if not normalization:
            raise BuildError(f"access saved-config card {card_type} requires authored normalization")
        field_hooks = {
            rule.get("hook") for field, rule in normalization["fields"].items()
            if field != "options" and rule.get("policy") == "hook"
        }
        if field_hooks != {"normalize_access_fields"}:
            raise BuildError(f"access saved-config card {card_type} requires the authored field hook")
        option_rule = normalization["fields"]["options"]
        if card_type == "lock":
            if option_rule.get("policy") != "clear" or normalization.get("optionHook") is not None:
                raise BuildError("lock saved-config normalization must declaratively clear options")
        else:
            if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_access_options":
                raise BuildError(f"access saved-config card {card_type} requires the authored option hook")
            if normalization.get("optionHook") != "normalize_access_options":
                raise BuildError(f"access saved-config card {card_type} has an unexpected optionHook")
        normalizations.append((card_type, normalization))
    return normalizations


def gen_saved_config_access_ts(data):
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG ACCESS HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigAccessFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigAccessOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigAccess(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigAccessFieldHook,\n",
        "  normalizeOptions: SavedConfigAccessOptionHook,\n",
        "): boolean {\n",
    ]
    for card_type, normalization in saved_config_access_normalizations(data):
        lines.append(f"  if (config.type === {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_access_fields":
                    if not field_hook_called:
                        lines.append("    normalizeFields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_access_options":
                    lines.append("    config.options = normalizeOptions(config.options || \"\", config);\n")
                else:
                    raise BuildError(f"unsupported access saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_access_h(data):
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG ACCESS HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_access(Config &config, FieldHook normalize_fields,\n",
        "                                          OptionHook normalize_options) {\n",
    ]
    for card_type, normalization in saved_config_access_normalizations(data):
        lines.append(f"  if (config.type == {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_access_fields":
                    if not field_hook_called:
                        lines.append("    normalize_fields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_access_options":
                    lines.append("    config.options = normalize_options(config.options, config);\n")
                else:
                    raise BuildError(f"unsupported access saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


SAVED_CONFIG_SECURITY_CARD_TYPES = ("alarm", "alarm_action")


def saved_config_security_normalizations(data):
    normalizations = []
    for card_type in SAVED_CONFIG_SECURITY_CARD_TYPES:
        normalization = data["cards"][card_type].get("normalization")
        if not normalization:
            raise BuildError(f"security saved-config card {card_type} requires authored normalization")
        field_hooks = {
            rule.get("hook") for field, rule in normalization["fields"].items()
            if field != "options" and rule.get("policy") == "hook"
        }
        if field_hooks != {"normalize_security_fields"}:
            raise BuildError(f"security saved-config card {card_type} requires the authored field hook")
        option_rule = normalization["fields"]["options"]
        if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_security_options":
            raise BuildError(f"security saved-config card {card_type} requires the authored option hook")
        if normalization.get("optionHook") != "normalize_security_options":
            raise BuildError(f"security saved-config card {card_type} has an unexpected optionHook")
        normalizations.append((card_type, normalization))
    return normalizations


def gen_saved_config_security_ts(data):
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SECURITY HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigSecurityFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigSecurityOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigSecurity(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigSecurityFieldHook,\n",
        "  normalizeOptions: SavedConfigSecurityOptionHook,\n",
        "): boolean {\n",
    ]
    for card_type, normalization in saved_config_security_normalizations(data):
        lines.append(f"  if (config.type === {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_security_fields":
                    if not field_hook_called:
                        lines.append("    normalizeFields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_security_options":
                    lines.append("    config.options = normalizeOptions(config.options || \"\", config);\n")
                else:
                    raise BuildError(f"unsupported security saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def gen_saved_config_security_h(data):
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SECURITY HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_security(Config &config, FieldHook normalize_fields,\n",
        "                                            OptionHook normalize_options) {\n",
    ]
    for card_type, normalization in saved_config_security_normalizations(data):
        lines.append(f"  if (config.type == {json.dumps(card_type)}) {{\n")
        field_hook_called = False
        for field in data["fields"]:
            rule = normalization["fields"][field]
            if rule.get("policy") == "hook":
                if rule.get("hook") == "normalize_security_fields":
                    if not field_hook_called:
                        lines.append("    normalize_fields(config);\n")
                        field_hook_called = True
                elif rule.get("hook") == "normalize_security_options":
                    lines.append("    config.options = normalize_options(config.options, config);\n")
                else:
                    raise BuildError(f"unsupported security saved-config hook {rule.get('hook')!r}")
            else:
                lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
        lines.append("    return true;\n  }\n")
    lines.append("  return false;\n}\n")
    return "".join(lines)


def saved_config_weather_normalization(data):
    normalization = data["cards"]["weather"].get("normalization")
    if not normalization:
        raise BuildError("weather saved-config card requires authored normalization")
    field_hooks = {
        rule.get("hook") for field, rule in normalization["fields"].items()
        if field != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_weather_fields"}:
        raise BuildError("weather saved-config card requires the authored field hook")
    option_rule = normalization["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_weather_options":
        raise BuildError("weather saved-config card requires the authored option hook")
    if normalization.get("optionHook") != "normalize_weather_options":
        raise BuildError("weather saved-config card has an unexpected optionHook")
    if normalization.get("migrationActions") != ["legacy_weather_forecast"]:
        raise BuildError("weather saved-config card requires the legacy forecast migration")
    migration = data["migrationActions"]["legacy_weather_forecast"]
    if migration.get("set") != {"type": "weather", "precision": "tomorrow"}:
        raise BuildError("legacy weather forecast migration has unexpected field updates")
    return normalization


def gen_saved_config_weather_ts(data):
    normalization = saved_config_weather_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG WEATHER HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigWeatherFieldHook = (config: CardConfig, wasLegacyForecast: boolean) => void;\n",
        "export type SavedConfigWeatherOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function migrateSavedConfigWeatherLegacy(config: CardConfig): boolean {\n",
        '  if (config.type !== "weather_forecast") return false;\n',
        '  config.type = "weather";\n',
        '  config.precision = "tomorrow";\n',
        "  return true;\n",
        "}\n\n",
        "export function normalizeSavedConfigWeather(\n",
        "  config: CardConfig,\n",
        "  wasLegacyForecast: boolean,\n",
        "  normalizeFields: SavedConfigWeatherFieldHook,\n",
        "  normalizeOptions: SavedConfigWeatherOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "weather") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_weather_fields":
                if not field_hook_called:
                    lines.append("  normalizeFields(config, wasLegacyForecast);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_weather_options":
                lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
            else:
                raise BuildError(f"unsupported weather saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_weather_h(data):
    normalization = saved_config_weather_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG WEATHER HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config>\n",
        "inline bool migrate_saved_config_weather_legacy(Config &config) {\n",
        '  if (config.type != "weather_forecast") return false;\n',
        '  config.type = "weather";\n',
        '  config.precision = "tomorrow";\n',
        "  return true;\n",
        "}\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_weather(Config &config, bool was_legacy_forecast,\n",
        "                                           FieldHook normalize_fields,\n",
        "                                           OptionHook normalize_options) {\n",
        '  if (config.type != "weather") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_weather_fields":
                if not field_hook_called:
                    lines.append("  normalize_fields(config, was_legacy_forecast);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_weather_options":
                lines.append("  config.options = normalize_options(config.options, config);\n")
            else:
                raise BuildError(f"unsupported weather saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def saved_config_image_normalization(data):
    normalization = data["cards"]["image"].get("normalization")
    if not normalization:
        raise BuildError("image saved-config card requires authored normalization")
    field_hooks = {
        rule.get("hook") for field, rule in normalization["fields"].items()
        if field != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_image_fields"}:
        raise BuildError("image saved-config card requires the authored field hook")
    option_rule = normalization["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_image_options":
        raise BuildError("image saved-config card requires the authored option hook")
    if normalization.get("optionHook") != "normalize_image_options":
        raise BuildError("image saved-config card has an unexpected optionHook")
    return normalization


def gen_saved_config_image_ts(data):
    normalization = saved_config_image_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG IMAGE HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigImageFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigImageOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigImage(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigImageFieldHook,\n",
        "  normalizeOptions: SavedConfigImageOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "image") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_image_fields":
                if not field_hook_called:
                    lines.append("  normalizeFields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_image_options":
                lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
            else:
                raise BuildError(f"unsupported image saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_image_h(data):
    normalization = saved_config_image_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG IMAGE HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_image(Config &config, FieldHook normalize_fields,\n",
        "                                         OptionHook normalize_options) {\n",
        '  if (config.type != "image") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_image_fields":
                if not field_hook_called:
                    lines.append("  normalize_fields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_image_options":
                lines.append("  config.options = normalize_options(config.options, config);\n")
            else:
                raise BuildError(f"unsupported image saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def saved_config_climate_normalization(data):
    climate = data["cards"]["climate"].get("normalization")
    control = data["cards"]["climate_control"].get("normalization")
    if not climate or not control:
        raise BuildError("climate saved-config cards require authored normalization")
    if climate["fields"] != control["fields"]:
        raise BuildError("climate saved-config cards must share field normalization")
    field_hooks = {
        rule.get("hook") for field, rule in control["fields"].items()
        if field != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_climate_fields"}:
        raise BuildError("climate saved-config cards require the authored field hook")
    option_rule = control["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_climate_options":
        raise BuildError("climate saved-config cards require the authored option hook")
    if control.get("optionHook") != "normalize_climate_options":
        raise BuildError("climate saved-config cards have an unexpected optionHook")
    return control


def gen_saved_config_climate_ts(data):
    normalization = saved_config_climate_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG CLIMATE HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigClimateFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigClimateOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigClimate(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigClimateFieldHook,\n",
        "  normalizeOptions: SavedConfigClimateOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "climate" && config.type !== "climate_control") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_climate_fields":
                if not field_hook_called:
                    lines.append("  normalizeFields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_climate_options":
                lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
            else:
                raise BuildError(f"unsupported climate saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_climate_h(data):
    normalization = saved_config_climate_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG CLIMATE HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_climate(Config &config, FieldHook normalize_fields,\n",
        "                                           OptionHook normalize_options) {\n",
        '  if (config.type != "climate" && config.type != "climate_control") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_climate_fields":
                if not field_hook_called:
                    lines.append("  normalize_fields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_climate_options":
                lines.append("  config.options = normalize_options(config.options, config);\n")
            else:
                raise BuildError(f"unsupported climate saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def saved_config_light_control_normalization(data):
    normalization = data["cards"]["light_control"].get("normalization")
    if not normalization:
        raise BuildError("light control saved-config card requires authored normalization")
    option_rule = normalization["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_light_control_options":
        raise BuildError("light control saved-config card requires the authored option hook")
    if normalization.get("optionHook") != "normalize_light_control_options":
        raise BuildError("light control saved-config card has an unexpected optionHook")
    return normalization


def gen_saved_config_light_control_ts(data):
    normalization = saved_config_light_control_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG LIGHT CONTROL HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigLightControlOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigLightControl(\n",
        "  config: CardConfig,\n",
        "  normalizeOptions: SavedConfigLightControlOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "light_control") return false;\n',
    ]
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") != "normalize_light_control_options":
                raise BuildError(f"unsupported light control saved-config hook {rule.get('hook')!r}")
            lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_light_control_h(data):
    normalization = saved_config_light_control_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG LIGHT CONTROL HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename OptionHook>\n",
        "inline bool normalize_saved_config_light_control(Config &config,\n",
        "                                                 OptionHook normalize_options) {\n",
        '  if (config.type != "light_control") return false;\n',
    ]
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") != "normalize_light_control_options":
                raise BuildError(f"unsupported light control saved-config hook {rule.get('hook')!r}")
            lines.append("  config.options = normalize_options(config.options, config);\n")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def saved_config_webhook_normalization(data):
    normalization = data["cards"]["webhook"].get("normalization")
    if not normalization:
        raise BuildError("webhook saved-config card requires authored normalization")
    field_hooks = {
        rule.get("hook") for field, rule in normalization["fields"].items()
        if field != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_webhook_fields"}:
        raise BuildError("webhook saved-config card requires the authored field hook")
    option_rule = normalization["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_webhook_options":
        raise BuildError("webhook saved-config card requires the authored option hook")
    if normalization.get("optionHook") != "normalize_webhook_options":
        raise BuildError("webhook saved-config card has an unexpected optionHook")
    return normalization


def gen_saved_config_webhook_ts(data):
    normalization = saved_config_webhook_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG WEBHOOK HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigWebhookFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigWebhookOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigWebhook(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigWebhookFieldHook,\n",
        "  normalizeOptions: SavedConfigWebhookOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "webhook") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_webhook_fields":
                if not field_hook_called:
                    lines.append("  normalizeFields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_webhook_options":
                lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
            else:
                raise BuildError(f"unsupported webhook saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_webhook_h(data):
    normalization = saved_config_webhook_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG WEBHOOK HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_webhook(Config &config, FieldHook normalize_fields,\n",
        "                                           OptionHook normalize_options) {\n",
        '  if (config.type != "webhook") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_webhook_fields":
                if not field_hook_called:
                    lines.append("  normalize_fields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_webhook_options":
                lines.append("  config.options = normalize_options(config.options, config);\n")
            else:
                raise BuildError(f"unsupported webhook saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def saved_config_subpage_normalization(data):
    normalization = data["cards"]["subpage"].get("normalization")
    if not normalization:
        raise BuildError("subpage saved-config card requires authored normalization")
    field_hooks = {
        rule.get("hook") for field, rule in normalization["fields"].items()
        if field != "options" and rule.get("policy") == "hook"
    }
    if field_hooks != {"normalize_subpage_fields"}:
        raise BuildError("subpage saved-config card requires the authored field hook")
    option_rule = normalization["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_subpage_options":
        raise BuildError("subpage saved-config card requires the authored option hook")
    if normalization.get("optionHook") != "normalize_subpage_options":
        raise BuildError("subpage saved-config card has an unexpected optionHook")
    return normalization


def gen_saved_config_subpage_ts(data):
    normalization = saved_config_subpage_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SUBPAGE HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigSubpageFieldHook = (config: CardConfig) => void;\n",
        "export type SavedConfigSubpageOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigSubpage(\n",
        "  config: CardConfig,\n",
        "  normalizeFields: SavedConfigSubpageFieldHook,\n",
        "  normalizeOptions: SavedConfigSubpageOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "subpage") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_subpage_fields":
                if not field_hook_called:
                    lines.append("  normalizeFields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_subpage_options":
                lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
            else:
                raise BuildError(f"unsupported subpage saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_subpage_h(data):
    normalization = saved_config_subpage_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SUBPAGE HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename FieldHook, typename OptionHook>\n",
        "inline bool normalize_saved_config_subpage(Config &config, FieldHook normalize_fields,\n",
        "                                           OptionHook normalize_options) {\n",
        '  if (config.type != "subpage") return false;\n',
    ]
    field_hook_called = False
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") == "normalize_subpage_fields":
                if not field_hook_called:
                    lines.append("  normalize_fields(config);\n")
                    field_hook_called = True
            elif rule.get("hook") == "normalize_subpage_options":
                lines.append("  config.options = normalize_options(config.options, config);\n")
            else:
                raise BuildError(f"unsupported subpage saved-config hook {rule.get('hook')!r}")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def saved_config_switch_normalization(data):
    normalization = data["cards"][""].get("normalization")
    if not normalization:
        raise BuildError("switch saved-config card requires authored normalization")
    option_rule = normalization["fields"]["options"]
    if option_rule.get("policy") != "hook" or option_rule.get("hook") != "normalize_switch_options":
        raise BuildError("switch saved-config card requires the authored option hook")
    if normalization.get("optionHook") != "normalize_switch_options":
        raise BuildError("switch saved-config card has an unexpected optionHook")
    return normalization


def gen_saved_config_switch_ts(data):
    normalization = saved_config_switch_normalization(data)
    lines = [
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SWITCH HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        'import type { CardConfig } from "../contracts/types";\n\n',
        "export type SavedConfigSwitchOptionHook = (options: string, config: CardConfig) => string;\n\n",
        "export function normalizeSavedConfigSwitch(\n",
        "  config: CardConfig,\n",
        "  normalizeOptions: SavedConfigSwitchOptionHook,\n",
        "): boolean {\n",
        '  if (config.type !== "") return false;\n',
    ]
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") != "normalize_switch_options":
                raise BuildError(f"unsupported switch saved-config hook {rule.get('hook')!r}")
            lines.append('  config.options = normalizeOptions(config.options || "", config);\n')
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "ts"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_switch_h(data):
    normalization = saved_config_switch_normalization(data)
    lines = [
        "#pragma once\n\n",
        "// =============================================================================\n",
        "// GENERATED SAVED-CONFIG SWITCH HELPERS - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n\n",
        "template<typename Config, typename OptionHook>\n",
        "inline bool normalize_saved_config_switch(Config &config,\n",
        "                                         OptionHook normalize_options) {\n",
        '  if (!config.type.empty()) return false;\n',
    ]
    for field in data["fields"]:
        rule = normalization["fields"][field]
        if rule.get("policy") == "hook":
            if rule.get("hook") != "normalize_switch_options":
                raise BuildError(f"unsupported switch saved-config hook {rule.get('hook')!r}")
            lines.append("  config.options = normalize_options(config.options);\n")
        else:
            lines.extend(saved_config_declarative_field_lines(field, rule, "cpp"))
    lines.append("  return true;\n}\n")
    return "".join(lines)


def gen_saved_config_shadow_ts(data):
    policies = shadow_pilot_policies(data)
    media = data["cards"]["media"]
    media_behavior = media["behavior"]["media"]
    media_volume = contract_card_option(media and data["cards"], "media", "volume_max")
    media_modes = contract_card_option_values(data["cards"], "media", "media_mode")
    media_now_playing = contract_card_option_values(data["cards"], "media", "media_now_playing_controls")
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
        f"const ACTION_OPTION_SELECT_ACTIONS = {json.dumps(data['optionSelect']['actions'])} as const;\n"
        f"const MEDIA_MODES = {json.dumps(media_modes)} as const;\n"
        f"const MEDIA_DEFAULT_MODE = {json.dumps(media_behavior['defaultMode'])};\n"
        f"const MEDIA_MODE_ALIASES: Readonly<Record<string, string>> = {json.dumps(media_behavior.get('legacyModes', {}))};\n"
        f"const MEDIA_STATE_DISPLAY_MODES = {json.dumps(media_behavior.get('stateDisplayModes', []))} as const;\n"
        f"const MEDIA_NOW_PLAYING_CONTROLS = {json.dumps(media_now_playing)} as const;\n"
        f"const MEDIA_VOLUME_MIN = {int(media_volume['min'])};\n"
        f"const MEDIA_VOLUME_MAX = {int(media_volume['max'])};\n"
        f"const MEDIA_VOLUME_DEFAULT = {json.dumps(media_volume['defaultValue'])};\n"
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
        "    else if (policy.policy === \"allowed\") { config[field] = policy.aliases?.[config[field]] || config[field]; if (policy.values.indexOf(config[field]) < 0) config[field] = policy.fallback; }\n"
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
        "  if (config.type === \"text_sensor\") { config.type = \"sensor\"; config.precision = \"text\"; config.entity = \"\"; config.label = \"\"; config.unit = \"\"; config.icon_on = \"Auto\"; }\n"
        "  if (config.type === \"local_sensor\") { config.type = \"sensor\"; config.sensor = \"local\"; config.icon_on = \"Auto\"; config.options = \"\"; }\n"
        "  if (config.type !== \"sensor\") return null;\n"
        "  if (config.sensor !== \"local\" && config.precision === \"time\") { config.unit = \"\"; config.icon = \"Auto\"; config.icon_on = \"Auto\"; }\n"
        "  if (config.sensor === \"local\") {\n"
        "    config.icon_on = \"Auto\"; config.options = \"\";\n"
        "    if ([\"text\", \"1\", \"2\"].indexOf(config.precision) < 0) config.precision = \"\";\n"
        "    if (config.precision !== \"text\" && (!config.icon || config.icon === \"Auto\")) config.icon = \"Auto\";\n"
        "    return config;\n"
        "  }\n"
        "  const source = config.options; const out: string[] = [];\n"
        "  if (config.precision !== \"icon\" && config.precision !== \"text\" && config.precision !== \"time\") {\n"
        "    if (optionValue(source, \"large_numbers\") === \"off\") out.push(\"large_numbers=off\");\n"
        "    else if (optionPresent(source, \"large_numbers\")) out.push(\"large_numbers\");\n"
        "  }\n"
        "  if (config.precision !== \"time\" && optionPresent(source, \"active_color\")) out.push(\"active_color\");\n"
        "  if (config.precision === \"text\" && optionPresent(source, \"state_labels\")) {\n"
        "    out.push(\"state_labels\");\n"
        "    let stateInput = optionValue(source, \"state_input\"); let stateOutput = optionValue(source, \"state_output\");\n"
        "    if (!stateInput && optionValue(source, \"state_high_label\")) { stateInput = \"high\"; if (!stateOutput) stateOutput = optionValue(source, \"state_high_label\"); }\n"
        "    else if (!stateInput && optionValue(source, \"state_low_label\")) { stateInput = \"low\"; if (!stateOutput) stateOutput = optionValue(source, \"state_low_label\"); }\n"
        "    for (const [name, value] of [[\"state_input\", stateInput], [\"state_output\", stateOutput], [\"state_input_2\", optionValue(source, \"state_input_2\")], [\"state_output_2\", optionValue(source, \"state_output_2\")]] as const) {\n"
        "      const trimmed = value.trim();\n"
        "      if (trimmed) out.push(name + \"=\" + encodeOptionValue(trimmed));\n"
        "    }\n"
        "  }\n"
        "  if (config.precision === \"time\") { const timeUnit = optionValue(source, \"time_unit\"); if ([\"seconds\", \"minutes\", \"hours\", \"days\"].indexOf(timeUnit) >= 0) out.push(\"time_unit=\" + timeUnit); }\n"
        "  config.options = out.join(\",\"); return config;\n"
        "}\n"
        "\n"
        "export function normalizeSavedConfigActionShadow(input: Partial<CardConfig>): CardConfig | null {\n"
        "  const config = shaped(input);\n"
        "  if (config.type === \"local\") { config.type = \"action\"; config.sensor = \"local\"; }\n"
        "  if (config.type === \"option_select\") { config.type = \"action\"; config.sensor = \"input_select.select_option\"; }\n"
        "  if (config.type !== \"action\") return null;\n"
        "  if (ACTION_OPTION_SELECT_ACTIONS.indexOf(config.sensor as typeof ACTION_OPTION_SELECT_ACTIONS[number]) >= 0) {\n"
        "    config.sensor = \"input_select.select_option\"; config.unit = \"\"; config.precision = \"\"; config.options = \"\"; config.icon_on = \"Auto\";\n"
        "    if (!config.icon || config.icon === \"Auto\" || config.icon === \"Chevron Down\") config.icon = \"Flash\"; return config;\n"
        "  }\n"
        "  if (config.sensor === \"local\") {\n"
        "    config.unit = \"\"; config.precision = \"\"; config.options = \"\"; config.icon_on = \"Auto\";\n"
        "    if (!config.icon || config.icon === \"Auto\" || config.icon === \"Flash\") config.icon = \"Gesture Tap\"; return config;\n"
        "  }\n"
        "  config.precision = \"\"; const source = config.options; const out: string[] = []; const stateEntity = optionValue(source, \"state_entity\").trim();\n"
        "  if (stateEntity) {\n"
        "    out.push(\"state_entity=\" + encodeOptionValue(stateEntity)); const rawPrecision = optionValue(source, \"state_precision\");\n"
        "    if (rawPrecision === \"icon\" || rawPrecision === \"text\") out.push(\"state_precision=\" + rawPrecision);\n"
        "    else {\n"
        "      const stateUnit = optionValue(source, \"state_unit\").trim(); const numericPrecision = [\"0\", \"1\", \"2\"].indexOf(rawPrecision) >= 0;\n"
        "      if (stateUnit) out.push(\"state_unit=\" + encodeOptionValue(stateUnit));\n"
        "      if (numericPrecision) out.push(\"state_precision=\" + rawPrecision);\n"
        "      if (optionValue(source, \"large_numbers\") === \"off\") out.push(\"large_numbers=off\"); else if (optionPresent(source, \"large_numbers\")) out.push(\"large_numbers\");\n"
        "    }\n"
        "  }\n"
        "  if (config.sensor === \"script.turn_on\") {\n"
        "    const fields = optionValue(source, \"script_fields\").trim(); if (fields) out.push(\"script_fields=\" + encodeOptionValue(fields));\n"
        "    if (optionPresent(source, \"confirm_on\")) {\n"
        "      out.push(\"confirm_on\"); const values: readonly (readonly [string, string])[] = [[\"confirm_message\", \"Run this script?\"], [\"confirm_yes\", \"Yes\"], [\"confirm_no\", \"No\"]];\n"
        "      for (const [name, defaultValue] of values) { const value = optionValue(source, name).trim(); if (value && value !== defaultValue) out.push(name + \"=\" + encodeOptionValue(value)); }\n"
        "    }\n"
        "  }\n"
        "  config.options = out.join(\",\"); return config;\n"
        "}\n"
        "\n"
        "function normalizedMediaVolume(value: string): string {\n"
        "  if (!value) return MEDIA_VOLUME_DEFAULT; const parsed = parseInt(value, 10);\n"
        "  if (!isFinite(parsed)) return MEDIA_VOLUME_DEFAULT; return String(Math.max(MEDIA_VOLUME_MIN, Math.min(MEDIA_VOLUME_MAX, parsed)));\n"
        "}\n"
        "export function normalizeSavedConfigMediaShadow(input: Partial<CardConfig>): CardConfig | null {\n"
        "  const config = shaped(input); if (config.type !== \"media\") return null; const rawMode = config.sensor; const source = config.options;\n"
        "  const aliasedMode = MEDIA_MODE_ALIASES[rawMode] || rawMode; config.sensor = MEDIA_MODES.indexOf(aliasedMode as typeof MEDIA_MODES[number]) >= 0 ? aliasedMode : MEDIA_DEFAULT_MODE;\n"
        "  if (config.sensor === \"now_playing\" && optionPresent(source, \"media_cover_art\")) config.sensor = \"cover_art\";\n"
        "  if (rawMode === \"controls\" && (!config.icon || config.icon === \"Speaker\")) config.icon = \"Auto\";\n"
        "  if (config.sensor === \"previous\" && config.label === \"Skip Previous\") config.label = \"Previous\";\n"
        "  if (config.sensor === \"next\" && config.label === \"Skip Next\") config.label = \"Next\";\n"
        "  if (config.sensor === \"volume\") { if (!config.label || config.label === \"Media\") config.label = \"Volume\"; config.icon = \"Auto\"; }\n"
        "  if (config.sensor === \"playlist\") { if (!config.label || config.label === \"Media\") config.label = \"Playlist\"; if (!config.icon || config.icon === \"Auto\") config.icon = \"Music\"; }\n"
        "  if (config.sensor === \"position\" && (!config.label || config.label === \"Track\")) config.label = \"Position\";\n"
        "  if (config.sensor === \"now_playing\") config.precision = MEDIA_NOW_PLAYING_CONTROLS.indexOf(config.precision as typeof MEDIA_NOW_PLAYING_CONTROLS[number]) >= 0 ? config.precision : \"\";\n"
        "  else if (MEDIA_STATE_DISPLAY_MODES.indexOf(config.sensor as typeof MEDIA_STATE_DISPLAY_MODES[number]) < 0 || config.precision !== \"state\") config.precision = \"\";\n"
        "  const out: string[] = []; const maxVolume = normalizedMediaVolume(optionValue(source, \"volume_max\"));\n"
        "  if (config.sensor === \"control_modal\") {\n"
        "    if (optionValue(source, \"label_display\").trim() === \"label\") out.push(\"label_display=label\"); if (optionValue(source, \"number_display\").trim() === \"volume\") out.push(\"number_display=volume\"); if (maxVolume !== MEDIA_VOLUME_DEFAULT) out.push(\"volume_max=\" + maxVolume);\n"
        "  } else if (config.sensor === \"playlist\") {\n"
        "    for (const [name, defaultValue] of [[\"playlist_content_id\", \"\"], [\"playlist_content_type\", \"playlist\"], [\"playlist_player_source\", \"\"]] as const) { const value = optionValue(source, name).trim() || defaultValue; if (value && value !== defaultValue) out.push(name + \"=\" + encodeOptionValue(value)); }\n"
        "  } else if (config.sensor === \"volume\" || config.sensor === \"position\") {\n"
        "    if (config.sensor === \"volume\" && maxVolume !== MEDIA_VOLUME_DEFAULT) out.push(\"volume_max=\" + maxVolume); if (optionValue(source, \"large_numbers\") === \"off\") out.push(\"large_numbers=off\"); else if (optionPresent(source, \"large_numbers\")) out.push(\"large_numbers\");\n"
        "  }\n"
        "  config.options = out.join(\",\"); return config;\n"
        "}\n"
        "\n"
        "export function normalizeSavedConfigShadow(input: Partial<CardConfig>): CardConfig | null {\n"
        "  return normalizeSavedConfigVacuumShadow(input) || normalizeSavedConfigSensorShadow(input) || normalizeSavedConfigActionShadow(input) || normalizeSavedConfigMediaShadow(input);\n"
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
    media_behavior = data["cards"]["media"]["behavior"]["media"]
    media_volume = contract_card_option(data["cards"], "media", "volume_max")
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
    vacuum_sensor = data["cards"]["vacuum"]["normalization"]["fields"]["sensor"]
    modes = vacuum_sensor["values"]
    mode_aliases = vacuum_sensor.get("aliases", {})
    lines.extend([
        cpp_string_array("SAVED_CONFIG_SHADOW_VACUUM_MODES", modes),
        cpp_string_array("SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES", hook["preserveUnitForModes"]),
        cpp_string_array("SAVED_CONFIG_SHADOW_ACTION_OPTION_SELECT_ACTIONS", data["optionSelect"]["actions"]),
        cpp_string_array("SAVED_CONFIG_SHADOW_MEDIA_MODES", contract_card_option_values(data["cards"], "media", "media_mode")),
        cpp_string_array("SAVED_CONFIG_SHADOW_MEDIA_STATE_DISPLAY_MODES", media_behavior.get("stateDisplayModes", [])),
        cpp_string_array("SAVED_CONFIG_SHADOW_MEDIA_NOW_PLAYING_CONTROLS", contract_card_option_values(data["cards"], "media", "media_now_playing_controls")),
        f"constexpr int SAVED_CONFIG_SHADOW_MEDIA_VOLUME_MIN = {int(media_volume['min'])};\n",
        f"constexpr int SAVED_CONFIG_SHADOW_MEDIA_VOLUME_MAX = {int(media_volume['max'])};\n",
        f"constexpr int SAVED_CONFIG_SHADOW_MEDIA_VOLUME_DEFAULT = {int(media_volume['defaultValue'])};\n",
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
        *[f"  if (config.sensor == {json.dumps(alias)}) config.sensor = {json.dumps(target)};\n" for alias, target in mode_aliases.items()],
        "  if (!saved_config_shadow_string_in(config.sensor, SAVED_CONFIG_SHADOW_VACUUM_MODES, sizeof(SAVED_CONFIG_SHADOW_VACUUM_MODES) / sizeof(SAVED_CONFIG_SHADOW_VACUUM_MODES[0]))) config.sensor = \"start_stop\";\n",
        "  config.type = \"vacuum\"; config.icon_on = \"Auto\"; config.precision.clear(); config.options.clear();\n",
        "  if (!saved_config_shadow_string_in(config.sensor, SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES, sizeof(SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES) / sizeof(SAVED_CONFIG_SHADOW_VACUUM_UNIT_MODES[0]))) config.unit.clear();\n",
        "  if (config.icon.empty() || config.icon == \"Auto\") config.icon = saved_config_shadow_vacuum_icon(config.sensor);\n",
        "  return true;\n}\n",
        "\ninline void saved_config_shadow_append_option(std::string &out, const std::string &name, const std::string &value = \"\") {\n",
        "  if (!out.empty()) out += ',';\n",
        "  out += name;\n",
        "  if (!value.empty()) out += \"=\" + encode_compact_field(value);\n",
        "}\n\n",
        "inline std::string saved_config_shadow_trim(const std::string &value) {\n",
        "  size_t begin = 0; while (begin < value.size() && (value[begin] == ' ' || value[begin] == '\\t' || value[begin] == '\\r' || value[begin] == '\\n')) ++begin;\n",
        "  size_t end = value.size(); while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\\t' || value[end - 1] == '\\r' || value[end - 1] == '\\n')) --end;\n",
        "  return value.substr(begin, end - begin);\n",
        "}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_sensor_shadow(Config &config) {\n",
        "  if (config.type == \"text_sensor\") { config.type = \"sensor\"; config.precision = \"text\"; config.entity.clear(); config.label.clear(); config.unit.clear(); config.icon_on = \"Auto\"; if (config.icon.empty()) config.icon = \"Auto\"; }\n",
        "  if (config.type == \"local_sensor\") { config.type = \"sensor\"; config.sensor = \"local\"; config.icon_on = \"Auto\"; config.options.clear(); }\n",
        "  if (config.type != \"sensor\") return false;\n",
        "  if (config.icon.empty()) config.icon = \"Auto\";\n",
        "  if (config.icon_on.empty()) config.icon_on = \"Auto\";\n",
        "  if (config.sensor != \"local\" && config.precision == \"time\") { config.unit.clear(); config.icon = \"Auto\"; config.icon_on = \"Auto\"; }\n",
        "  if (config.sensor == \"local\") { config.icon_on = \"Auto\"; config.options.clear(); if (config.precision != \"text\" && config.precision != \"1\" && config.precision != \"2\") config.precision.clear(); if (config.precision != \"text\" && (config.icon.empty() || config.icon == \"Auto\")) config.icon = \"Auto\"; return true; }\n",
        "  const std::string source = config.options; std::string out;\n",
        "  if (config.precision != \"icon\" && config.precision != \"text\" && config.precision != \"time\") append_large_numbers_option(out, source);\n",
        "  if (config.precision != \"time\" && cfg_option_token_present(source, \"active_color\")) saved_config_shadow_append_option(out, \"active_color\");\n",
        "  if (config.precision == \"text\" && cfg_option_token_present(source, \"state_labels\")) {\n",
        "    saved_config_shadow_append_option(out, \"state_labels\"); std::string input = cfg_option_value(source, \"state_input\"); std::string output = cfg_option_value(source, \"state_output\");\n",
        "    if (input.empty() && !cfg_option_value(source, \"state_high_label\").empty()) { input = \"high\"; if (output.empty()) output = cfg_option_value(source, \"state_high_label\"); }\n",
        "    else if (input.empty() && !cfg_option_value(source, \"state_low_label\").empty()) { input = \"low\"; if (output.empty()) output = cfg_option_value(source, \"state_low_label\"); }\n",
        "    input = saved_config_shadow_trim(input); output = saved_config_shadow_trim(output);\n",
        "    if (!input.empty()) saved_config_shadow_append_option(out, \"state_input\", input);\n",
        "    if (!output.empty()) saved_config_shadow_append_option(out, \"state_output\", output);\n",
        "    const std::string input_2 = cfg_option_value(source, \"state_input_2\"); const std::string output_2 = cfg_option_value(source, \"state_output_2\");\n",
        "    const std::string input_2_trimmed = saved_config_shadow_trim(input_2); const std::string output_2_trimmed = saved_config_shadow_trim(output_2);\n",
        "    if (!input_2_trimmed.empty()) saved_config_shadow_append_option(out, \"state_input_2\", input_2_trimmed);\n",
        "    if (!output_2_trimmed.empty()) saved_config_shadow_append_option(out, \"state_output_2\", output_2_trimmed);\n",
        "  }\n",
        "  if (config.precision == \"time\") { const std::string time_unit = cfg_option_value(source, \"time_unit\"); if (time_unit == \"seconds\" || time_unit == \"minutes\" || time_unit == \"hours\" || time_unit == \"days\") saved_config_shadow_append_option(out, \"time_unit\", time_unit); }\n",
        "  config.options = out; return true;\n}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_action_shadow(Config &config) {\n",
        "  if (config.type == \"local\") { config.type = \"action\"; config.sensor = \"local\"; }\n",
        "  if (config.type == \"option_select\") { config.type = \"action\"; config.sensor = \"input_select.select_option\"; }\n",
        "  if (config.type != \"action\") return false;\n",
        "  if (config.icon.empty()) config.icon = \"Auto\";\n",
        "  if (config.icon_on.empty()) config.icon_on = \"Auto\";\n",
        "  if (saved_config_shadow_string_in(config.sensor, SAVED_CONFIG_SHADOW_ACTION_OPTION_SELECT_ACTIONS, sizeof(SAVED_CONFIG_SHADOW_ACTION_OPTION_SELECT_ACTIONS) / sizeof(SAVED_CONFIG_SHADOW_ACTION_OPTION_SELECT_ACTIONS[0]))) {\n",
        "    config.sensor = \"input_select.select_option\"; config.unit.clear(); config.precision.clear(); config.options.clear(); config.icon_on = \"Auto\";\n",
        "    if (config.icon.empty() || config.icon == \"Auto\" || config.icon == \"Chevron Down\") config.icon = \"Flash\";\n",
        "    return true;\n",
        "  }\n",
        "  if (config.sensor == \"local\") {\n",
        "    config.unit.clear(); config.precision.clear(); config.options.clear(); config.icon_on = \"Auto\";\n",
        "    if (config.icon.empty() || config.icon == \"Auto\" || config.icon == \"Flash\") config.icon = \"Gesture Tap\";\n",
        "    return true;\n",
        "  }\n",
        "  config.precision.clear(); const std::string source = config.options; std::string out; const std::string state_entity = saved_config_shadow_trim(cfg_option_value(source, \"state_entity\"));\n",
        "  if (!state_entity.empty()) { saved_config_shadow_append_option(out, \"state_entity\", state_entity); const std::string raw_precision = cfg_option_value(source, \"state_precision\");\n",
        "    if (raw_precision == \"icon\" || raw_precision == \"text\") saved_config_shadow_append_option(out, \"state_precision\", raw_precision);\n",
        "    else {\n",
        "      const std::string state_unit = saved_config_shadow_trim(cfg_option_value(source, \"state_unit\"));\n",
        "      const bool numeric_precision = raw_precision == \"0\" || raw_precision == \"1\" || raw_precision == \"2\";\n",
        "      if (!state_unit.empty()) saved_config_shadow_append_option(out, \"state_unit\", state_unit);\n",
        "      if (numeric_precision) saved_config_shadow_append_option(out, \"state_precision\", raw_precision);\n",
        "      append_large_numbers_option(out, source);\n",
        "    }\n",
        "  }\n",
        "  if (config.sensor == \"script.turn_on\") {\n",
        "    const std::string fields = saved_config_shadow_trim(cfg_option_value(source, \"script_fields\"));\n",
        "    if (!fields.empty()) saved_config_shadow_append_option(out, \"script_fields\", fields);\n",
        "    if (cfg_option_token_present(source, \"confirm_on\")) {\n",
        "      saved_config_shadow_append_option(out, \"confirm_on\");\n",
        "      const std::string message = saved_config_shadow_trim(cfg_option_value(source, \"confirm_message\"));\n",
        "      const std::string yes = saved_config_shadow_trim(cfg_option_value(source, \"confirm_yes\"));\n",
        "      const std::string no = saved_config_shadow_trim(cfg_option_value(source, \"confirm_no\"));\n",
        "      if (!message.empty() && message != \"Run this script?\") saved_config_shadow_append_option(out, \"confirm_message\", message);\n",
        "      if (!yes.empty() && yes != \"Yes\") saved_config_shadow_append_option(out, \"confirm_yes\", yes);\n",
        "      if (!no.empty() && no != \"No\") saved_config_shadow_append_option(out, \"confirm_no\", no);\n",
        "    }\n",
        "  }\n",
        "  config.options = out; return true;\n}\n\n",
        "inline std::string saved_config_shadow_media_mode(const std::string &mode) {\n",
        f"  if (mode == \"controls\") return {json.dumps(media_behavior['defaultMode'])};\n",
        "  if (saved_config_shadow_string_in(mode, SAVED_CONFIG_SHADOW_MEDIA_MODES, sizeof(SAVED_CONFIG_SHADOW_MEDIA_MODES) / sizeof(SAVED_CONFIG_SHADOW_MEDIA_MODES[0]))) return mode;\n",
        f"  return {json.dumps(media_behavior['defaultMode'])};\n}}\n\n",
        "inline int saved_config_shadow_media_volume(const std::string &value) {\n",
        "  if (value.empty()) return SAVED_CONFIG_SHADOW_MEDIA_VOLUME_DEFAULT;\n",
        "  char *end = nullptr;\n",
        "  long parsed = std::strtol(value.c_str(), &end, 10);\n",
        "  if (end == value.c_str()) return SAVED_CONFIG_SHADOW_MEDIA_VOLUME_DEFAULT;\n",
        "  if (parsed < SAVED_CONFIG_SHADOW_MEDIA_VOLUME_MIN) return SAVED_CONFIG_SHADOW_MEDIA_VOLUME_MIN;\n",
        "  if (parsed > SAVED_CONFIG_SHADOW_MEDIA_VOLUME_MAX) return SAVED_CONFIG_SHADOW_MEDIA_VOLUME_MAX;\n",
        "  return static_cast<int>(parsed);\n",
        "}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_media_shadow(Config &config) {\n",
        "  if (config.type != \"media\") return false;\n",
        "  if (config.icon.empty()) config.icon = \"Auto\";\n",
        "  if (config.icon_on.empty()) config.icon_on = \"Auto\";\n",
        "  const std::string raw_mode = config.sensor;\n",
        "  const std::string source = config.options;\n",
        "  config.sensor = saved_config_shadow_media_mode(raw_mode);\n",
        "  if (config.sensor == \"now_playing\" && cfg_option_token_present(source, \"media_cover_art\")) config.sensor = \"cover_art\";\n",
        "  if (raw_mode == \"controls\" && (config.icon.empty() || config.icon == \"Speaker\")) config.icon = \"Auto\";\n",
        "  if (config.sensor == \"previous\" && config.label == \"Skip Previous\") config.label = \"Previous\";\n",
        "  if (config.sensor == \"next\" && config.label == \"Skip Next\") config.label = \"Next\";\n",
        "  if (config.sensor == \"volume\") {\n",
        "    if (config.label.empty() || config.label == \"Media\") config.label = \"Volume\";\n",
        "    config.icon = \"Auto\";\n",
        "  }\n",
        "  if (config.sensor == \"playlist\") {\n",
        "    if (config.label.empty() || config.label == \"Media\") config.label = \"Playlist\";\n",
        "    if (config.icon.empty() || config.icon == \"Auto\") config.icon = \"Music\";\n",
        "  }\n",
        "  if (config.sensor == \"position\" && (config.label.empty() || config.label == \"Track\")) config.label = \"Position\";\n",
        "  if (config.sensor == \"now_playing\") { if (!saved_config_shadow_string_in(config.precision, SAVED_CONFIG_SHADOW_MEDIA_NOW_PLAYING_CONTROLS, sizeof(SAVED_CONFIG_SHADOW_MEDIA_NOW_PLAYING_CONTROLS) / sizeof(SAVED_CONFIG_SHADOW_MEDIA_NOW_PLAYING_CONTROLS[0]))) config.precision.clear(); }\n",
        "  else if (!saved_config_shadow_string_in(config.sensor, SAVED_CONFIG_SHADOW_MEDIA_STATE_DISPLAY_MODES, sizeof(SAVED_CONFIG_SHADOW_MEDIA_STATE_DISPLAY_MODES) / sizeof(SAVED_CONFIG_SHADOW_MEDIA_STATE_DISPLAY_MODES[0])) || config.precision != \"state\") config.precision.clear();\n",
        "  std::string out; const int max_volume = saved_config_shadow_media_volume(cfg_option_value(source, \"volume_max\"));\n",
        "  if (config.sensor == \"control_modal\") {\n",
        "    if (saved_config_shadow_trim(cfg_option_value(source, \"label_display\")) == \"label\") saved_config_shadow_append_option(out, \"label_display\", \"label\");\n",
        "    if (saved_config_shadow_trim(cfg_option_value(source, \"number_display\")) == \"volume\") saved_config_shadow_append_option(out, \"number_display\", \"volume\");\n",
        "    if (max_volume != SAVED_CONFIG_SHADOW_MEDIA_VOLUME_DEFAULT) saved_config_shadow_append_option(out, \"volume_max\", std::to_string(max_volume));\n",
        "  } else if (config.sensor == \"playlist\") {\n",
        "    const std::string content_id = saved_config_shadow_trim(cfg_option_value(source, \"playlist_content_id\"));\n",
        "    const std::string content_type = saved_config_shadow_trim(cfg_option_value(source, \"playlist_content_type\"));\n",
        "    const std::string player_source = saved_config_shadow_trim(cfg_option_value(source, \"playlist_player_source\"));\n",
        "    if (!content_id.empty()) saved_config_shadow_append_option(out, \"playlist_content_id\", content_id);\n",
        "    if (!content_type.empty() && content_type != \"playlist\") saved_config_shadow_append_option(out, \"playlist_content_type\", content_type);\n",
        "    if (!player_source.empty()) saved_config_shadow_append_option(out, \"playlist_player_source\", player_source);\n",
        "  } else if (config.sensor == \"volume\" || config.sensor == \"position\") {\n",
        "    if (config.sensor == \"volume\" && max_volume != SAVED_CONFIG_SHADOW_MEDIA_VOLUME_DEFAULT) saved_config_shadow_append_option(out, \"volume_max\", std::to_string(max_volume));\n",
        "    append_large_numbers_option(out, source);\n",
        "  }\n",
        "  config.options = out; return true;\n}\n\n",
        "template<typename Config>\ninline bool normalize_saved_config_shadow(Config &config) {\n",
        "  if (normalize_saved_config_vacuum_shadow(config)) return true;\n",
        "  if (normalize_saved_config_sensor_shadow(config)) return true;\n",
        "  if (normalize_saved_config_action_shadow(config)) return true;\n",
        "  return normalize_saved_config_media_shadow(config);\n",
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


def runtime_enum_name(value, empty_name="SWITCH"):
    if not value:
        return empty_name
    return re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_").upper()


def runtime_capability_enum_name(value):
    words = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", value)
    return f"CAPABILITY_{runtime_enum_name(words)}"


def gen_card_runtime_h(data):
    runtime = data["runtime"]
    specs = runtime["specs"]
    capabilities = runtime["capabilities"]
    lines = [
        "namespace espcontrol::card_runtime {\n",
        "\n",
        "enum class CardTypeId : uint8_t {\n",
    ]
    for card_type in specs:
        lines.append(f"  {runtime_enum_name(card_type)},\n")
    lines.extend([
        "  UNKNOWN,\n",
        "};\n",
        "\n",
        "enum class CardDriverId : uint8_t {\n",
    ])
    for driver in runtime["drivers"]:
        lines.append(f"  {runtime_enum_name(driver)},\n")
    lines.extend([
        "  UNKNOWN,\n",
        "};\n",
        "\n",
        "enum CardCapabilityFlag : uint16_t {\n",
        "  CAPABILITY_NONE = 0,\n",
    ])
    for index, capability in enumerate(capabilities):
        lines.append(f"  {runtime_capability_enum_name(capability)} = 1u << {index},\n")
    lines.extend([
        "};\n",
        "\n",
        "struct CardRuntimeSpec {\n",
        "  CardTypeId type = CardTypeId::UNKNOWN;\n",
        "  CardDriverId driver = CardDriverId::UNKNOWN;\n",
        "  uint16_t capabilities = CAPABILITY_NONE;\n",
        "};\n",
        "\n",
        "constexpr bool has_capability(const CardRuntimeSpec &spec, CardCapabilityFlag capability) {\n",
        "  return (spec.capabilities & static_cast<uint16_t>(capability)) != 0;\n",
        "}\n",
        "\n",
        "inline CardTypeId card_type_id(const std::string &type) {\n",
    ])
    for card_type in specs:
        condition = "type.empty()" if not card_type else f"type == {json.dumps(card_type)}"
        lines.append(f"  if ({condition}) return CardTypeId::{runtime_enum_name(card_type)};\n")
    lines.extend([
        "  return CardTypeId::UNKNOWN;\n",
        "}\n",
        "\n",
        "inline CardRuntimeSpec card_runtime_spec(CardTypeId type) {\n",
        "  switch (type) {\n",
    ])
    for card_type, spec in specs.items():
        enabled = [
            runtime_capability_enum_name(capability)
            for capability in capabilities
            if spec["capabilities"][capability]
        ]
        mask = " | ".join(enabled) if enabled else "CAPABILITY_NONE"
        lines.append(
            f"    case CardTypeId::{runtime_enum_name(card_type)}: return "
            f"{{type, CardDriverId::{runtime_enum_name(spec['driver'])}, static_cast<uint16_t>({mask})}};\n"
        )
    lines.extend([
        "    default: return {};\n",
        "  }\n",
        "}\n",
        "\n",
        "inline CardDriverId resolve_card_driver(CardTypeId type, const std::string &mode) {\n",
        "  switch (type) {\n",
    ])
    mode_specs = []
    for card_type, spec in specs.items():
        if "modeField" not in spec:
            continue
        mode_specs.append((card_type, spec))
        lines.append(f"    case CardTypeId::{runtime_enum_name(card_type)}:\n")
        for mode, driver in spec["modes"].items():
            lines.append(f"      if (mode == {json.dumps(mode)}) return CardDriverId::{runtime_enum_name(driver)};\n")
        lines.append(f"      return CardDriverId::{runtime_enum_name(spec['defaultDriver'])};\n")
    lines.extend([
        "    default: return card_runtime_spec(type).driver;\n",
        "  }\n",
        "}\n",
        "\n",
        "template<typename Config>\n",
        "inline CardRuntimeSpec resolve_card_runtime(const Config &config) {\n",
        "  CardRuntimeSpec spec = card_runtime_spec(card_type_id(config.type));\n",
        "  switch (spec.type) {\n",
    ])
    for card_type, spec in mode_specs:
        lines.extend([
            f"    case CardTypeId::{runtime_enum_name(card_type)}:\n",
            f"      spec.driver = resolve_card_driver(spec.type, config.{spec['modeField']});\n",
            "      break;\n",
        ])
    lines.extend([
        "    default:\n",
        "      break;\n",
        "  }\n",
        "  return spec;\n",
        "}\n",
        "\n",
        "}  // namespace espcontrol::card_runtime\n",
        "\n",
    ])
    return "".join(lines)


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
        "#include <cstdint>\n",
        "#include <string>\n",
        "\n",
        "// =============================================================================\n",
        "// GENERATED CARD CONFIG CONTRACT - do not edit by hand\n",
        "// Generated by scripts/build.py from common/config/card_contract.json.\n",
        "// =============================================================================\n",
        "\n",
        f"constexpr int CARD_CONTRACT_VERSION = {int(data['contractVersion'])};\n",
        gen_card_runtime_h(data),
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
        (SAVED_CONFIG_VACUUM_TS, gen_saved_config_vacuum_ts(data)),
        (SAVED_CONFIG_VACUUM_H, gen_saved_config_vacuum_h(data)),
        (SAVED_CONFIG_SENSOR_TS, gen_saved_config_sensor_ts(data)),
        (SAVED_CONFIG_SENSOR_H, gen_saved_config_sensor_h(data)),
        (SAVED_CONFIG_ACTION_TS, gen_saved_config_action_ts(data)),
        (SAVED_CONFIG_ACTION_H, gen_saved_config_action_h(data)),
        (SAVED_CONFIG_MEDIA_TS, gen_saved_config_media_ts(data)),
        (SAVED_CONFIG_MEDIA_H, gen_saved_config_media_h(data)),
        (SAVED_CONFIG_STATIC_TS, gen_saved_config_static_ts(data)),
        (SAVED_CONFIG_STATIC_H, gen_saved_config_static_h(data)),
        (SAVED_CONFIG_FAN_TS, gen_saved_config_fan_ts(data)),
        (SAVED_CONFIG_FAN_H, gen_saved_config_fan_h(data)),
        (SAVED_CONFIG_DATE_TIME_TS, gen_saved_config_date_time_ts(data)),
        (SAVED_CONFIG_DATE_TIME_H, gen_saved_config_date_time_h(data)),
        (SAVED_CONFIG_MOWER_TS, gen_saved_config_mower_ts(data)),
        (SAVED_CONFIG_MOWER_H, gen_saved_config_mower_h(data)),
        (SAVED_CONFIG_OCCUPANCY_TS, gen_saved_config_occupancy_ts(data)),
        (SAVED_CONFIG_OCCUPANCY_H, gen_saved_config_occupancy_h(data)),
        (SAVED_CONFIG_ACCESS_TS, gen_saved_config_access_ts(data)),
        (SAVED_CONFIG_ACCESS_H, gen_saved_config_access_h(data)),
        (SAVED_CONFIG_SECURITY_TS, gen_saved_config_security_ts(data)),
        (SAVED_CONFIG_SECURITY_H, gen_saved_config_security_h(data)),
        (SAVED_CONFIG_WEATHER_TS, gen_saved_config_weather_ts(data)),
        (SAVED_CONFIG_WEATHER_H, gen_saved_config_weather_h(data)),
        (SAVED_CONFIG_IMAGE_TS, gen_saved_config_image_ts(data)),
        (SAVED_CONFIG_IMAGE_H, gen_saved_config_image_h(data)),
        (SAVED_CONFIG_CLIMATE_TS, gen_saved_config_climate_ts(data)),
        (SAVED_CONFIG_CLIMATE_H, gen_saved_config_climate_h(data)),
        (SAVED_CONFIG_LIGHT_CONTROL_TS, gen_saved_config_light_control_ts(data)),
        (SAVED_CONFIG_LIGHT_CONTROL_H, gen_saved_config_light_control_h(data)),
        (SAVED_CONFIG_WEBHOOK_TS, gen_saved_config_webhook_ts(data)),
        (SAVED_CONFIG_WEBHOOK_H, gen_saved_config_webhook_h(data)),
        (SAVED_CONFIG_SUBPAGE_TS, gen_saved_config_subpage_ts(data)),
        (SAVED_CONFIG_SUBPAGE_H, gen_saved_config_subpage_h(data)),
        (SAVED_CONFIG_SWITCH_TS, gen_saved_config_switch_ts(data)),
        (SAVED_CONFIG_SWITCH_H, gen_saved_config_switch_h(data)),
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
        write_generated_text(path, content)
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
    image_slots = capability["imageSlots"]
    image_slot_text = (
        "Not supported"
        if image_slots == 0
        else f"Up to {image_slots} simultaneous Image or Media Cover Art cards"
    )
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
        f"| Image-based cards | {image_slot_text} |\n"
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
        write_generated_text(path, generated)
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
            write_generated_text(path, content)
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
    """Build one shared www.js containing the validated device profiles."""
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
        input=json.dumps({
            "outputDir": str(build_root),
            "devices": devices,
            "testHooks": test_hooks,
            "overlays": GENERATED_TRANSACTION.overlays() if GENERATED_TRANSACTION is not None else {},
        }),
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        if temporary_root:
            temporary_root.cleanup()
        raise BuildError(result.stderr.strip() or "esbuild failed while building web bundles")

    if output_dir is not None:
        print(f"Built shared www.js bundle and {len(devices)} compatibility loader(s) in {build_root}")
        return []

    outputs = [(WWW_OUTPUT_DIR / "www.js", (build_root / "www.js").read_text())]
    outputs.extend(
        (WWW_OUTPUT_DIR / slug / "www.js", (build_root / slug / "www.js").read_text())
        for slug in devices
    )
    dirty = [
        str(path.relative_to(WWW_OUTPUT_DIR))
        for path, generated in outputs
        if not path.exists() or path.read_text() != generated
    ]

    if not check_only:
        for path, generated in outputs:
            if not path.exists() or path.read_text() != generated:
                write_generated_text(path, generated)

    if check_only and dirty:
        print("www.js outputs are out of date. Run 'python scripts/build.py www' to fix:")
        for relative_path in dirty:
            print(f"  docs/public/webserver/{relative_path}")
    if temporary_root:
        temporary_root.cleanup()
    return dirty


# ===========================================================================
# Main
# ===========================================================================

def main():
    global GENERATED_TRANSACTION
    args = sys.argv[1:]
    if args == ["--self-test"]:
        try:
            run_generated_transaction_self_test()
        except BuildError as exc:
            print(exc)
            return 1
        return 0
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
    transaction = None
    if not check_only and temporary_output is None:
        transaction = GeneratedOutputTransaction()
        GENERATED_TRANSACTION = transaction

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
        if exit_code == 0 and transaction is not None:
            transaction.commit()
    except (BuildError, ProductSchemaError) as exc:
        print(exc)
        return 1
    finally:
        GENERATED_TRANSACTION = None

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
