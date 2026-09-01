#!/usr/bin/env python3
"""Generate and validate internal developer documentation control tables."""

from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

from check_tasks_data import PROFILES, TASKS


ROOT = Path(__file__).resolve().parents[1]

SOURCE_BEGIN = "<!-- BEGIN GENERATED SOURCE OF TRUTH -->"
SOURCE_END = "<!-- END GENERATED SOURCE OF TRUTH -->"
CARD_BEGIN = "<!-- BEGIN GENERATED CARD TYPE MAP -->"
CARD_END = "<!-- END GENERATED CARD TYPE MAP -->"
CHECK_BEGIN = "<!-- BEGIN GENERATED CHECK MATRIX -->"
CHECK_END = "<!-- END GENERATED CHECK MATRIX -->"
BUILD_FLAGS_BEGIN = "<!-- BEGIN GENERATED DEVICE BUILD FLAGS -->"
BUILD_FLAGS_END = "<!-- END GENERATED DEVICE BUILD FLAGS -->"
HISTORICAL_MARKER = "<!-- DEV-DOC-STATUS: historical -->"


@dataclass(frozen=True)
class SourceTruthRow:
    source: str
    outputs: tuple[str, ...]
    generator: str
    checks: str


@dataclass(frozen=True)
class CheckMatrixRow:
    changed_paths: str
    task: str
    run_first: str
    broaden_when: str


SOURCE_TRUTH_ROWS: tuple[SourceTruthRow, ...] = (
    SourceTruthRow(
        "product/v2/card_contract.json",
        (
            "src/webserver/generated/card_contract.ts",
            "components/espcontrol/button_grid_contract_generated.h",
            "docs/generated/cards/capabilities.md",
        ),
        "python3 scripts/build.py contract",
        "`npm run check:card-contract-outputs` and `npm run check:product`",
    ),
    SourceTruthRow(
        "common/config/card_runtime_inventory.json",
        (
            "common/config/card_runtime_baseline_card_normalization_fixtures.json",
            "compatibility/fixtures/card_runtime_surface_baseline.json",
            "docs/generated/cards/runtime-coverage.md",
        ),
        "node scripts/generate_card_runtime_coverage.js",
        "`npm run check:card-runtime-coverage` and `npm run check:saved-config-parity`",
    ),
    SourceTruthRow(
        "product/v2/entity_names.json",
        ("common/config/entity_names.yaml", "src/webserver/generated/entity_catalog.ts"),
        "python3 scripts/build.py entities",
        "`python3 scripts/build.py entities --check` and `npm run check:product`",
    ),
    SourceTruthRow(
        "product/v2/device_catalog.json",
        ("devices/manifest.json",),
        "python3 scripts/generate_device_manifest.py",
        "`python3 scripts/generate_device_manifest.py --check` and `npm run check:product`",
    ),
    SourceTruthRow(
        "dev-docs/build-flag-notes.json and per-device YAML `build_flags`",
        ("generated section inside `dev-docs/devices-and-builds.md`",),
        "python3 scripts/check_dev_docs.py --update",
        "`npm run check:dev-docs`",
    ),
    SourceTruthRow(
        "devices/manifest.json",
        ("docs/public/device-profiles.json", "docs/generated/screens/*.md"),
        "python3 scripts/build.py devices",
        "`npm run check:device-profiles` and `npm run check:product`",
    ),
    SourceTruthRow(
        "devices/manifest.json device slot, font role, and profile data",
        ("generated blocks inside `devices/*/packages.yaml`", "generated blocks inside `devices/*/device/sensors.yaml`"),
        "python3 scripts/generate_device_slots.py",
        "`python3 scripts/generate_device_slots.py --check` and `npm run check:product`",
    ),
    SourceTruthRow(
        "product/v2/icons.json",
        (
            "generated sections inside `common/assets/icon_glyphs.yaml`",
            "generated sections inside `components/espcontrol/icons.h`",
            "`src/webserver/generated/icons.ts`",
        ),
        "python3 scripts/build.py icons",
        "`python3 scripts/build.py icons --check` and `npm run check:product`",
    ),
    SourceTruthRow(
        "common/assets/*glyphs.yaml, except generated sections in `icon_glyphs.yaml`",
        ("no generated output; firmware font inputs consume these glyph lists directly",),
        "none",
        "compile the affected firmware before publishing",
    ),
    SourceTruthRow(
        "product/v2/translations/strings.*.txt",
        ("components/espcontrol/i18n_generated.h",),
        "python3 scripts/build.py i18n",
        "`python3 scripts/build.py i18n --check` and `npm run check:product`",
    ),
    SourceTruthRow(
        "src/webserver/model/index.ts",
        ("no intermediate output; imported directly into each web bundle",),
        "python3 scripts/build.py www",
        "`npm run check:model-contract`",
    ),
    SourceTruthRow(
        "src/webserver/",
        (
            "docs/public/webserver/www.js",
            "docs/public/webserver/web-assets.json",
            "docs/public/webserver/embedded/www.js",
            "docs/public/webserver/bundles/*/www.js",
            "docs/public/webserver/*/www.js",
        ),
        "python3 scripts/build.py www",
        "`npm run check:web-smoke`, `npm run check:web-asset-manifest`, and `npm run check:product`",
    ),
    SourceTruthRow(
        "product/v2/product_compatibility.json",
        ("no generated output; protects saved config, backup, layout, and migration behavior",),
        "none",
        "`npm run check:backup-contract` and `npm run check:product`",
    ),
    SourceTruthRow(
        "`product/v2/device_catalog.json`, `product/v2/card_contract.json`, `product/v2/entity_names.json`, `product/v2/icons.json`, `product/v2/product_compatibility.json`",
        ("product/product_snapshot.json",),
        "python3 scripts/check_product_snapshot.py --update",
        "`npm run check:product-snapshot` and `npm run check:product`",
    ),
)


PUBLIC_DOCS_BY_TYPE: dict[str, str] = {
    "": "docs/card-types/switches.md",
    "action": "docs/card-types/actions.md",
    "alarm": "docs/card-types/alarms.md",
    "alarm_action": "docs/card-types/alarms.md",
    "calendar": "docs/card-types/calendar.md",
    "clock": "docs/card-types/calendar.md",
    "climate": "docs/card-types/climate.md",
    "climate_control": "docs/card-types/climate.md",
    "cover": "docs/card-types/covers.md",
    "door_window": "docs/card-types/doors-windows.md",
    "presence": "docs/card-types/presence.md",
    "fan_control": "docs/card-types/fans.md",
    "fan_direction": "docs/card-types/fans.md",
    "fan_oscillate": "docs/card-types/fans.md",
    "fan_preset": "docs/card-types/fans.md",
    "fan_speed": "docs/card-types/fans.md",
    "fan_switch": "docs/card-types/fans.md",
    "garage": "docs/card-types/garage-doors.md",
    "gate": "docs/card-types/gates.md",
    "internal": "docs/card-types/internal-relays.md",
    "light_brightness": "docs/card-types/lights.md",
    "light_control": "docs/card-types/lights.md",
    "light_switch": "docs/card-types/lights.md",
    "light_temperature": "docs/card-types/lights.md",
    "local_sensor": "docs/card-types/local-sensors.md",
    "lock": "docs/card-types/locks.md",
    "media": "docs/card-types/media.md",
    "option_select": "docs/card-types/option-select.md",
    "push": "docs/card-types/buttons.md",
    "screen_lock": "docs/card-types/screen-lock.md",
    "webhook": "docs/card-types/webhooks.md",
    "sensor": "docs/card-types/sensors.md",
    "slider": "docs/card-types/sliders.md",
    "subpage": "docs/features/subpages.md",
    "timezone": "docs/card-types/timezones.md",
    "vacuum": "docs/card-types/vacuum.md",
    "lawn_mower": "docs/card-types/lawn-mower.md",
    "wifi_qr": "docs/card-types/wifi-share.md",
    "wifi_qr_card": "docs/card-types/wifi-share.md",
    "weather": "docs/card-types/weather.md",
    "image": "docs/card-types/cameras.md",
    "weather_forecast": "docs/card-types/weather-forecast.md",
}


CHECK_MATRIX_ROWS: tuple[CheckMatrixRow, ...] = (
    CheckMatrixRow(
        "`product/v2/card_contract.json`",
        "Card metadata, defaults, domains, picker grouping, option definitions, generated card capability docs",
        "`npm run check:card-contract-outputs`",
        "`npm run check:product` when firmware, web, backup, or release-facing generated output changes",
    ),
    CheckMatrixRow(
        "`common/config/card_runtime_inventory.json`, card registrations, or the firmware family registry",
        "Card runtime coverage, legacy classification, picker/preview baseline, and lifecycle responsibilities",
        "`npm run check:card-runtime-coverage`",
        "`npm run check:product` when the reviewed baseline or a runtime registration changes",
    ),
    CheckMatrixRow(
        "`src/webserver/`",
        "Web configurator behavior, settings panels, preview rendering, backup UI, served `www.js` bundles",
        "`npm run check:web-smoke`",
        "`npm run check:web-browser-smoke` for browser behavior; `npm run check:product` before release-facing commits",
    ),
    CheckMatrixRow(
        "`components/espcontrol/*.h`",
        "Firmware card rendering, LVGL layout, modals, Home Assistant actions/subscriptions, parser behavior",
        "`npm run check:firmware-parser` plus the relevant firmware check",
        "`npm run check:fast` or compile affected firmware when display layout or device behavior changes",
    ),
    CheckMatrixRow(
        "`src/webserver/application/config_codec.ts`, `components/espcontrol/button_grid_config.h`, `product/v2/product_compatibility.json`",
        "Saved card strings, backup/import/export shape, migration compatibility",
        "`npm run check:backup-contract` and `npm run check:firmware-parser`",
        "`npm run check:product` when compact config, backup, or migration behavior changes",
    ),
    CheckMatrixRow(
        "`product/v2/device_catalog.json`, `devices/<slug>/`, `builds/*.yaml`",
        "Supported hardware, layout slots, firmware package shape, release build metadata",
        "`python3 scripts/check_tasks.py run-task device-manifest-output`, `npm run check:device-profiles`, and `npm run check:device-matrix`",
        "`npm run check:product`; compile affected firmware before publishing new or changed device support",
    ),
    CheckMatrixRow(
        "`product/v2/icons.json`, `common/assets/*glyphs.yaml`, `devices/<slug>/device/fonts.yaml`",
        "Icon names, glyph coverage, firmware font roles, device font mappings",
        "`python3 scripts/check_icon_groups.py`",
        "`npm run check:product`; compile affected firmware when a visible font role or small-screen layout changes",
    ),
    CheckMatrixRow(
        "`product/v2/entity_names.json`, entity name consumers",
        "Shared Home Assistant entity names consumed by firmware YAML and the web setup page",
        "`python3 scripts/build.py entities --check`",
        "`npm run check:product` when generated entity files or web behavior changes",
    ),
    CheckMatrixRow(
        "`product/product_snapshot.json`",
        "Generated combined product model snapshot",
        "`npm run check:product-snapshot`",
        "`npm run check:product` when authored product sources also changed",
    ),
    CheckMatrixRow(
        "`product/v2/translations/strings.*.txt`",
        "Firmware translations and generated i18n header",
        "`python3 scripts/build.py i18n --check`",
        "`npm run check:product` when translated UI strings affect release output",
    ),
    CheckMatrixRow(
        "`src/webserver/model/*.ts`, `src/webserver/contracts/*.ts`",
        "Typed model shape and generated browser model constants",
        "`npm run check:model-contract` and `npm run check:types`",
        "`npm run check:product` when backup, web, or model behavior changes",
    ),
    CheckMatrixRow(
        "`docs/`, `dev-docs/`, `DEVELOPERS.md`, `README.md`",
        "Public docs, internal maintainer docs, generated doc-control tables",
        "`npm run check:dev-docs` and `npm run docs:build`",
        "`npm run check:all` before publishing broad docs plus code changes",
    ),
    CheckMatrixRow(
        "`scripts/build.py`, `scripts/check_*.py`, `scripts/check_*.js`, `package.json`",
        "Generators, validators, and check orchestration",
        "Run the changed script directly with its `--check` or self-test mode when available",
        "`npm run check:fast` because generator/check changes can invalidate several safety nets",
    ),
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def read_json(path: str) -> object:
    return json.loads((ROOT / path).read_text())


def normalize_build_flag(value: str) -> str:
    value = value.strip().strip('"\'')
    return value[2:] if value.startswith("-D") else value


def device_build_flags() -> dict[str, set[str]]:
    """Return exact flag-to-device mappings from device YAML build flag lists."""
    mappings: dict[str, set[str]] = {}
    key_re = re.compile(r"^(\s*)(?:build_flags|build_src_flags):\s*$")
    unsupported_key_re = re.compile(r"^\s*(?:build_flags|build_src_flags):\s*\S")
    item_re = re.compile(r"^\s*-\s*(.+?)\s*$")

    for path in sorted((ROOT / "devices").glob("**/*.yaml")):
        parts = path.relative_to(ROOT).parts
        if len(parts) < 2:
            continue
        device = parts[1]
        lines = path.read_text().splitlines()
        index = 0
        while index < len(lines):
            if unsupported_key_re.match(lines[index]):
                raise ValueError(
                    f"{rel(path)} uses inline build flags; use a YAML list so documentation can validate each flag"
                )
            key_match = key_re.match(lines[index])
            if not key_match:
                index += 1
                continue
            base_indent = len(key_match.group(1))
            index += 1
            while index < len(lines):
                line = lines[index]
                stripped = line.strip()
                indent = len(line) - len(line.lstrip())
                if stripped and not stripped.startswith("#") and indent <= base_indent:
                    break
                item_match = item_re.match(line)
                if item_match:
                    raw = item_match.group(1).split(" #", 1)[0].strip()
                    flag = normalize_build_flag(raw)
                    if flag:
                        mappings.setdefault(flag, set()).add(device)
                index += 1
    return mappings


def build_flag_notes() -> dict[str, dict[str, str]]:
    notes = read_json("dev-docs/build-flag-notes.json")
    if not isinstance(notes, dict):
        raise ValueError("dev-docs/build-flag-notes.json must contain an object")
    return notes  # type: ignore[return-value]


def replace_between(text: str, begin: str, end: str, replacement: str) -> str:
    block = f"{begin}\n{replacement.rstrip()}\n{end}"
    if begin in text and end in text:
        pattern = re.compile(re.escape(begin) + r".*?" + re.escape(end), re.S)
        return pattern.sub(block, text)
    return text.rstrip() + "\n\n" + block + "\n"


def markdown_table(headers: tuple[str, ...], rows: list[tuple[str, ...]]) -> str:
    lines = [
        "| " + " | ".join(headers) + " |",
        "| " + " | ".join("---" for _ in headers) + " |",
    ]
    for row in rows:
        lines.append("| " + " | ".join(cell.replace("\n", "<br>") for cell in row) + " |")
    return "\n".join(lines)


def validate_check_guidance(value: str) -> str:
    """Keep generated command guidance tied to registered tasks and profiles."""
    task_ids = {item.id for item in TASKS}
    aliases = {f"check:{item.id}" for item in TASKS}
    aliases.update(f"check:{profile}" for profile in PROFILES if profile != "release")
    aliases.add("check:release-preflight")
    for alias in re.findall(r"npm run (check:[\w:-]+)", value):
        if alias not in aliases:
            raise ValueError(f"developer guidance references an unregistered check alias: {alias}")
        task_id = alias[len("check:"):]
        if task_id not in task_ids and task_id not in PROFILES and alias != "check:release-preflight":
            raise ValueError(f"developer guidance references an unknown task: {task_id}")
    return value


def source_truth_table() -> str:
    def code_if_path(value: str) -> str:
        if value.startswith(("generated ", "no generated ", "compile ")):
            return value
        if " " in value and not value.startswith(("common/", "components/", "compatibility/", "devices/", "docs/", "scripts/", "src/")):
            return value
        return value if "`" in value else f"`{value}`"

    rows = []
    for row in SOURCE_TRUTH_ROWS:
        source = code_if_path(row.source)
        outputs = "<br>".join(code_if_path(output) for output in row.outputs)
        rows.append((source, outputs, f"`{row.generator}`" if row.generator != "none" else "none", validate_check_guidance(row.checks)))
    return markdown_table(("Authored source", "Generated outputs", "Generator", "Required check"), rows)


def generated_build_flag_table() -> str:
    mappings = device_build_flags()
    notes = build_flag_notes()
    missing = sorted(set(mappings) - set(notes))
    extra = sorted(set(notes) - set(mappings))
    if missing:
        raise ValueError(
            "Missing purpose/removal notes in dev-docs/build-flag-notes.json for: "
            + ", ".join(missing)
        )
    if extra:
        raise ValueError(
            "Build flag notes no longer used by device YAML: " + ", ".join(extra)
        )

    rows: list[tuple[str, ...]] = []
    for flag in sorted(mappings):
        note = notes[flag]
        purpose = note.get("purpose") if isinstance(note, dict) else None
        remove_when = note.get("remove_when") if isinstance(note, dict) else None
        if not isinstance(purpose, str) or not purpose.strip():
            raise ValueError(f"Build flag {flag} needs a non-empty purpose")
        if not isinstance(remove_when, str) or not remove_when.strip():
            raise ValueError(f"Build flag {flag} needs non-empty removal criteria")
        devices = "<br>".join(f"`{device}`" for device in sorted(mappings[flag]))
        rows.append((f"`{flag}`", devices, purpose, remove_when))
    return markdown_table(("Flag", "Devices from YAML", "Purpose", "Remove when"), rows)


def contract_cards() -> dict[str, dict]:
    return read_json("product/v2/card_contract.json")["cards"]  # type: ignore[index]


def package_scripts() -> set[str]:
    package = read_json("package.json")
    return set(package["scripts"].keys())  # type: ignore[index, union-attr]


def web_registration_map() -> dict[str, str]:
    out: dict[str, str] = {}
    for path in sorted((ROOT / "src/webserver/cards").glob("*.ts")):
        text = path.read_text()
        for match in re.finditer(r"registry\.register\(\s*([\"'])(.*?)\1", text):
            out[match.group(2)] = rel(path)
        for match in re.finditer(
            r"registerCard\(\s*\{.*?\btype\s*:\s*([\"'])(.*?)\1",
            text,
            flags=re.DOTALL,
        ):
            out[match.group(2)] = rel(path)
    return out


def firmware_header_map(card_types: list[str]) -> dict[str, list[str]]:
    out = {card_type: [] for card_type in card_types}
    runtime_boundary = "components/espcontrol/button_grid_card_runtime.h"
    extra_by_type = {
        "weather": ["components/espcontrol/button_grid_weather_forecast.h"],
    }
    headers = [
        path for path in sorted((ROOT / "components/espcontrol").glob("button_grid*.h"))
        if not path.name.endswith("_generated.h")
    ]
    for card_type in card_types:
        if not card_type:
            needles = ("p.type.empty()", 'type == ""', 'type.empty()', 'card type == switch')
        else:
            needles = (f'"{card_type}"',)
        for path in headers:
            text = path.read_text(errors="ignore")
            for include in re.findall(r'#include\s+"(button_grid_saved_config_[^"/]*_generated\.h)"', text):
                generated = path.parent / include
                if generated.exists():
                    text += "\n" + generated.read_text(errors="ignore")
            if any(needle in text for needle in needles):
                out[card_type].append(rel(path))
        if runtime_boundary not in out[card_type]:
            out[card_type].append(runtime_boundary)
        for extra in extra_by_type.get(card_type, []):
            if extra not in out[card_type]:
                out[card_type].append(extra)
    return out


def option_summary(card: dict) -> str:
    options = card.get("options") or []
    if not options:
        return "None"
    labels = []
    for option in options:
        if option.get("docsHidden"):
            continue
        values = option.get("values") or []
        label = option.get("label") or option.get("name") or ""
        if values:
            labels.append(f"{label}: {', '.join('default' if v == '' else str(v) for v in values)}")
        else:
            labels.append(str(label))
    return "; ".join(labels) if labels else "None"


def docs_link(path: str) -> str:
    label = path.rsplit("/", 1)[-1]
    return f"[{label}](../{path})" if path.startswith("docs/") else f"`{path}`"


def generated_card_map() -> str:
    cards = contract_cards()
    card_types = list(cards.keys())
    web_files = web_registration_map()
    firmware_files = firmware_header_map(card_types)

    missing_public = sorted(set(card_types) - set(PUBLIC_DOCS_BY_TYPE))
    if missing_public:
        raise ValueError("Missing public docs mapping for card types: " + ", ".join(missing_public))

    public_rows = []
    matrix_rows = []
    for card_type, card in cards.items():
        type_label = "`(empty)`" if card_type == "" else f"`{card_type}`"
        public_doc = PUBLIC_DOCS_BY_TYPE[card_type]
        public_rows.append((docs_link(public_doc), type_label))

        domains = ", ".join(f"`{domain}`" for domain in card.get("domains", [])) or "None"
        subpages = "Yes" if card.get("allowInSubpage") else "No"
        status = "Hidden" if card.get("hidden") else "Visible"
        firmware = ", ".join(f"`{path}`" for path in firmware_files.get(card_type, [])) or "No direct match"
        web = f"`{web_files[card_type]}`" if card_type in web_files else "No current web type"
        checks = ["Contract", "Codec", "Parser"]
        if card.get("domains") or card_type in {"action", "push", "webhook", "weather", "image"}:
            checks.append("HA")
        if "modal" in " ".join(firmware_files.get(card_type, [])).lower() or card_type in {"alarm", "alarm_action", "climate", "climate_control", "media", "option_select", "image"}:
            checks.append("Modals")
        if card.get("options"):
            checks.append("Backup")
        matrix_rows.append((
            type_label,
            str(card.get("label") or "Switch"),
            web,
            firmware,
            domains,
            subpages,
            option_summary(card),
            status,
            ", ".join(dict.fromkeys(checks)),
        ))

    return "\n\n".join((
        "## Generated Public Documentation Map\n\n"
        "This table is generated by `python3 scripts/check_dev_docs.py --update` from "
        "`product/v2/card_contract.json` and the public documentation mapping in that script.\n\n"
        + markdown_table(("Public card page", "Covered saved type"), public_rows),
        "## Generated Matrix\n\n"
        "This table is generated from the card contract and typed `registry.register(...)` calls in "
        "`src/webserver/cards/`, and matching firmware header references under "
        "`components/espcontrol/`.\n\n"
        + markdown_table((
            "Type",
            "Label",
            "Web file",
            "Firmware references",
            "Entity domains",
            "Subpages",
            "Options",
            "Status",
            "Key checks",
        ), matrix_rows),
    ))


def generated_check_matrix() -> str:
    rows = [
        (
            row.changed_paths,
            row.task,
            validate_check_guidance(row.run_first),
            validate_check_guidance(row.broaden_when),
        )
        for row in CHECK_MATRIX_ROWS
    ]
    registered_rows = []
    package = read_json("package.json")["scripts"]  # type: ignore[index]
    for item in TASKS:
        alias = f"check:{item.id}"
        command = f"`npm run {alias}`" if alias in package else f"`python3 scripts/check_tasks.py run-task {item.id}`"
        inputs = "<br>".join(f"`{path}`" for path in item.inputs)
        cache_inputs = "<br>".join(f"`{path}`" for path in item.cache_inputs) or "—"
        cache_env = "<br>".join(f"`{name}`" for name in item.cache_env) or "—"
        cache_tools = "<br>".join(f"`{name}`" for name in item.cache_tools) or "—"
        registered_rows.append((
            f"`{item.id}`",
            ", ".join(item.domains),
            "Yes" if item.parallel_safe else "No",
            item.cache,
            cache_env,
            cache_tools,
            inputs,
            cache_inputs,
            command,
        ))
    return "\n\n".join((
        markdown_table(("Changed paths", "Likely task", "Run first", "Broaden when"), rows),
        "### Registered Check Tasks\n\n"
        "This detailed routing table is generated directly from `scripts/check_tasks_data.py`.\n\n"
        + markdown_table(
            (
                "Task",
                "Domains",
                "Parallel-safe",
                "Cache",
                "Cache environment",
                "Cache tools",
                "Declared inputs",
                "Cache-only inputs",
                "Focused command",
            ),
            registered_rows,
        ),
    ))


def update_generated_files() -> None:
    updates = {
        "dev-docs/source-of-truth.md": (SOURCE_BEGIN, SOURCE_END, source_truth_table()),
        "dev-docs/card-type-map.md": (CARD_BEGIN, CARD_END, generated_card_map()),
        "dev-docs/check-matrix.md": (CHECK_BEGIN, CHECK_END, generated_check_matrix()),
        "dev-docs/devices-and-builds.md": (
            BUILD_FLAGS_BEGIN,
            BUILD_FLAGS_END,
            generated_build_flag_table(),
        ),
    }
    for path, (begin, end, content) in updates.items():
        full = ROOT / path
        if full.exists():
            text = full.read_text()
        else:
            title = "# Check Matrix\n\nUse this page to choose the narrowest useful verification command from the files changed.\n"
            text = title if path.endswith("check-matrix.md") else ""
        full.write_text(replace_between(text, begin, end, content))


def expected_generated_text(path: str) -> str:
    if path == "dev-docs/source-of-truth.md":
        content = source_truth_table()
        begin, end = SOURCE_BEGIN, SOURCE_END
    elif path == "dev-docs/card-type-map.md":
        content = generated_card_map()
        begin, end = CARD_BEGIN, CARD_END
    elif path == "dev-docs/check-matrix.md":
        content = generated_check_matrix()
        begin, end = CHECK_BEGIN, CHECK_END
    elif path == "dev-docs/devices-and-builds.md":
        content = generated_build_flag_table()
        begin, end = BUILD_FLAGS_BEGIN, BUILD_FLAGS_END
    else:
        raise ValueError(path)
    current = (ROOT / path).read_text() if (ROOT / path).exists() else ""
    return replace_between(current, begin, end, content)


def check_generated_files(errors: list[str]) -> None:
    for path in (
        "dev-docs/source-of-truth.md",
        "dev-docs/card-type-map.md",
        "dev-docs/check-matrix.md",
        "dev-docs/devices-and-builds.md",
    ):
        full = ROOT / path
        if not full.exists():
            errors.append(f"{path} is missing; run python3 scripts/check_dev_docs.py --update")
            continue
        expected = expected_generated_text(path)
        if full.read_text() != expected:
            errors.append(f"{path} generated section is stale; run python3 scripts/check_dev_docs.py --update")


def source_truth_path_targets(value: str) -> list[str]:
    prefixes = (
        "common/",
        "components/",
        "compatibility/",
        "dev-docs/",
        "devices/",
        "docs/",
        "product/",
        "scripts/",
        "src/",
    )
    targets: list[str] = []
    quoted = re.findall(r"`([^`]+)`", value)
    if value.startswith(("generated ", "no generated ", "compile ")):
        return [target for target in quoted if target.startswith(prefixes)]
    targets.extend(target for target in quoted if target.startswith(prefixes))
    for prefix in prefixes:
        if value.startswith(prefix):
            targets.append(value.split(",", 1)[0].split(" ", 1)[0])
            break
    return list(dict.fromkeys(targets))


def check_source_truth_path(value: str, label: str, errors: list[str]) -> None:
    for target in source_truth_path_targets(value):
        if any(marker in target for marker in ("<", ">", "...")):
            continue
        matches = sorted(ROOT.glob(target)) if "*" in target else []
        if "*" in target:
            if not matches:
                errors.append(f"source-of-truth {label} pattern has no matches: {target}")
            continue
        if not (ROOT / target).exists():
            errors.append(f"source-of-truth {label} path is missing: {target}")


def check_source_truth_paths(errors: list[str]) -> None:
    for row in SOURCE_TRUTH_ROWS:
        check_source_truth_path(row.source, "source", errors)
        for output in row.outputs:
            check_source_truth_path(output, "output", errors)


def check_public_docs(errors: list[str]) -> None:
    card_types = set(contract_cards())
    mapped = set(PUBLIC_DOCS_BY_TYPE)
    missing = sorted(card_types - mapped)
    extra = sorted(mapped - card_types)
    if missing:
        errors.append("Missing PUBLIC_DOCS_BY_TYPE entries for: " + ", ".join(missing))
    if extra:
        errors.append("PUBLIC_DOCS_BY_TYPE has entries not in card contract: " + ", ".join(extra))
    for card_type, path in sorted(PUBLIC_DOCS_BY_TYPE.items()):
        if not (ROOT / path).exists():
            label = "(empty)" if card_type == "" else card_type
            errors.append(f"Public docs for card type {label} point to missing file: {path}")


def markdown_files() -> list[Path]:
    files = sorted((ROOT / "dev-docs").glob("**/*.md"))
    files.extend([ROOT / "DEVELOPERS.md", ROOT / "README.md", ROOT / "product/README.md"])
    return [path for path in files if path.exists()]


def workflow_files() -> list[Path]:
    workflow_dir = ROOT / ".github" / "workflows"
    files = sorted(workflow_dir.glob("*.yml"))
    files.extend(sorted(workflow_dir.glob("*.yaml")))
    return files


def check_markdown_links(errors: list[str]) -> None:
    link_re = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    for path in markdown_files():
        text = path.read_text()
        for match in link_re.finditer(text):
            target = match.group(1).split("#", 1)[0]
            if not target or re.match(r"^[a-z]+:", target) or target.startswith("#"):
                continue
            if target.startswith("/"):
                continue
            linked = (path.parent / target).resolve()
            if not linked.exists():
                errors.append(f"{rel(path)} links to missing file {target}")


def dev_doc_links(path: Path) -> set[Path]:
    link_re = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    dev_root = (ROOT / "dev-docs").resolve()
    links: set[Path] = set()
    for target in link_re.findall(path.read_text()):
        target = target.split("#", 1)[0]
        if not target or re.match(r"^[a-z]+:", target) or target.startswith(("#", "/")):
            continue
        linked = (path.parent / target).resolve()
        try:
            linked.relative_to(dev_root)
        except ValueError:
            continue
        if linked.suffix == ".md" and linked.exists():
            links.add(linked)
    return links


def check_indexed_dev_docs(errors: list[str]) -> None:
    dev_root = ROOT / "dev-docs"
    start = (dev_root / "README.md").resolve()
    reachable: set[Path] = set()
    pending = [start]
    while pending:
        path = pending.pop()
        if path in reachable:
            continue
        reachable.add(path)
        pending.extend(sorted(dev_doc_links(path) - reachable))

    all_pages = {path.resolve() for path in dev_root.glob("**/*.md")}
    orphaned = sorted(rel(path) for path in all_pages - reachable)
    if orphaned:
        errors.append("Unindexed developer documentation pages: " + ", ".join(orphaned))


def generated_output_paths() -> set[str]:
    paths: set[str] = set()
    for row in SOURCE_TRUTH_ROWS:
        for output in row.outputs:
            if output.startswith(("generated ", "no generated ")):
                continue
            for target in re.findall(r"`([^`]+)`", output):
                if "*" in target:
                    paths.update(rel(path) for path in ROOT.glob(target) if path.is_file())
                elif (ROOT / target).exists():
                    paths.add(target)
            if "`" not in output and "*" not in output and (ROOT / output).exists():
                paths.add(output)
    return paths


def check_generated_edit_instructions(errors: list[str]) -> None:
    edit_pattern = (
        r"\b(?:edit|hand-edit|modify|update|change|write(?:\s+directly)?\s+to|"
        r"add\s+[^.\n]{0,60}\s+to|remove\s+[^.\n]{0,60}\s+from)\b"
    )
    negative_re = re.compile(r"\b(?:do not|don't|never|avoid)\s+(?:hand-)?edit\b", re.IGNORECASE)
    list_intro_re = re.compile(
        r"^\s*(?:edit|hand-edit|modify|update|change)\s+"
        r"(?:these|the following|the)?\s*(?:generated\s+)?files?:\s*$",
        re.IGNORECASE,
    )
    generated_paths = generated_output_paths()
    for path in markdown_files():
        lines = path.read_text().splitlines()
        for line_number, line in enumerate(lines, start=1):
            if negative_re.search(line):
                continue
            for target in generated_paths:
                instruction_re = re.compile(
                    edit_pattern + rf"[^.\n]{{0,120}}`{re.escape(target)}`",
                    re.IGNORECASE,
                )
                if instruction_re.search(line):
                    errors.append(
                        f"{rel(path)}:{line_number} tells maintainers to edit generated file {target}"
                    )
            if not list_intro_re.match(line):
                continue
            for offset, listed_line in enumerate(lines[line_number:], start=1):
                if not listed_line.strip():
                    break
                if not re.match(r"^\s*[-*]\s+", listed_line):
                    continue
                for target in generated_paths:
                    if f"`{target}`" in listed_line:
                        errors.append(
                            f"{rel(path)}:{line_number + offset} lists generated file {target} under an edit instruction"
                        )


def check_device_build_flag_documentation(errors: list[str]) -> None:
    mappings = device_build_flags()
    notes = build_flag_notes()
    missing = sorted(set(mappings) - set(notes))
    extra = sorted(set(notes) - set(mappings))
    if missing:
        errors.append("Device build flags missing documentation: " + ", ".join(missing))
    if extra:
        errors.append("Documented device build flags no longer used: " + ", ".join(extra))
    for flag in sorted(set(mappings) & set(notes)):
        note = notes[flag]
        if not isinstance(note, dict) or not str(note.get("purpose", "")).strip():
            errors.append(f"Device build flag {flag} is missing a purpose")
        if not isinstance(note, dict) or not str(note.get("remove_when", "")).strip():
            errors.append(f"Device build flag {flag} is missing removal criteria")


def check_historical_records(errors: list[str]) -> None:
    dev_root = ROOT / "dev-docs"
    history_root = dev_root / "history"
    for path in sorted(dev_root.glob("**/*.md")):
        is_record = history_root in path.parents and path.name != "README.md"
        text = path.read_text()
        has_marker = HISTORICAL_MARKER in text
        describes_history = bool(
            re.search(r"(?im)^>\s*Historical record:|^Test window:\s*\d{4}", text)
        )
        if is_record and not has_marker:
            errors.append(f"Historical record is missing status marker: {rel(path)}")
        if has_marker and not is_record:
            errors.append(f"Historical record must live under dev-docs/history/: {rel(path)}")
        if describes_history and not is_record:
            errors.append(f"Historical guidance must live under dev-docs/history/: {rel(path)}")
        if path.name.endswith("-baseline.md") and not is_record:
            errors.append(f"Baseline record must live under dev-docs/history/: {rel(path)}")

    forbidden_sources = [dev_root / "task-router.md", *sorted((dev_root / "playbooks").glob("*.md"))]
    for source in forbidden_sources:
        for target in dev_doc_links(source):
            if history_root.resolve() in target.parents:
                errors.append(f"Current workflow {rel(source)} links to historical guidance {rel(target)}")

    heading = ""
    index = dev_root / "README.md"
    for line_number, line in enumerate(index.read_text().splitlines(), start=1):
        heading_match = re.match(r"^##\s+(.+?)\s*$", line)
        if heading_match:
            heading = heading_match.group(1)
        for target in re.findall(r"\[[^\]]+\]\(([^)]+)\)", line):
            linked = (index.parent / target.split("#", 1)[0]).resolve()
            if history_root.resolve() in linked.parents and heading != "Historical Records":
                errors.append(
                    f"dev-docs/README.md:{line_number} presents a historical page under {heading or 'no section'}"
                )


def check_referenced_commands(errors: list[str]) -> None:
    scripts = package_scripts()
    npm_re = re.compile(r"\bnpm run ([A-Za-z0-9:_-]+)")
    py_re = re.compile(r"\bpython3 (scripts/[A-Za-z0-9_./-]+)")
    for path in [*markdown_files(), *workflow_files()]:
        text = path.read_text()
        for cmd in npm_re.findall(text):
            if cmd not in scripts:
                errors.append(f"{rel(path)} references unknown npm script: {cmd}")
        for script_path in py_re.findall(text):
            if not (ROOT / script_path).exists():
                errors.append(f"{rel(path)} references missing script: {script_path}")


def check_referenced_paths(errors: list[str]) -> None:
    path_re = re.compile(r"`([^`]+)`")
    prefixes = (
        "common/",
        "components/",
        "compatibility/",
        "dev-docs/",
        "devices/",
        "docs/",
        "product/",
        "scripts/",
        "src/",
        "README.md",
        "DEVELOPERS.md",
        "package.json",
    )
    for path in markdown_files():
        for raw in path_re.findall(path.read_text()):
            for token in re.split(r"[\s,]+", raw):
                token = token.strip(".,:;()[]")
                if not token.startswith(prefixes):
                    continue
                if any(marker in token for marker in ("*", "<", ">", "...")):
                    continue
                if "/example." in token or token.endswith("/example"):
                    continue
                full = ROOT / token
                if not full.exists():
                    errors.append(f"{rel(path)} references missing path: {token}")


def check_local_artifacts(errors: list[str]) -> None:
    artifact = ROOT / "dev-docs/.DS_Store"
    if artifact.exists():
        errors.append("Remove local artifact dev-docs/.DS_Store")


def check_package_script(errors: list[str]) -> None:
    scripts = package_scripts()
    if "check:dev-docs" not in scripts:
        errors.append("package.json must define check:dev-docs")


def run_checks() -> list[str]:
    errors: list[str] = []
    check_package_script(errors)
    check_public_docs(errors)
    check_device_build_flag_documentation(errors)
    check_generated_files(errors)
    check_source_truth_paths(errors)
    check_markdown_links(errors)
    check_indexed_dev_docs(errors)
    check_generated_edit_instructions(errors)
    check_historical_records(errors)
    check_referenced_commands(errors)
    check_referenced_paths(errors)
    check_local_artifacts(errors)
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--update", action="store_true", help="rewrite generated dev-docs sections")
    parser.add_argument("--check", action="store_true", help="fail if dev-docs generated sections or references are stale")
    args = parser.parse_args()

    if args.update:
        update_generated_files()

    errors = run_checks() if args.check or not args.update else []
    if errors:
        for error in errors:
            print(f"dev-docs check failed: {error}", file=sys.stderr)
        return 1
    if args.check:
        print("dev-docs check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
