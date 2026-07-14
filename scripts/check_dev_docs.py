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
        "common/config/card_contract.json",
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
            "common/config/card_runtime_baseline_fixtures.json",
            "compatibility/fixtures/card_runtime_surface_baseline.json",
            "docs/generated/cards/runtime-coverage.md",
        ),
        "node scripts/generate_card_runtime_coverage.js",
        "`npm run check:card-runtime-coverage` and `npm run check:saved-config-parity`",
    ),
    SourceTruthRow(
        "common/config/entity_names.json",
        ("common/config/entity_names.yaml", "src/webserver/generated/entity_catalog.ts"),
        "python3 scripts/build.py entities",
        "`python3 scripts/build.py entities --check` and `npm run check:product`",
    ),
    SourceTruthRow(
        "devices/catalog.json",
        ("devices/manifest.json",),
        "python3 scripts/generate_device_manifest.py",
        "`python3 scripts/generate_device_manifest.py --check` and `npm run check:product`",
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
        "common/assets/icons.json",
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
        "common/config/strings.*.txt",
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
        ("docs/public/webserver/www.js",),
        "python3 scripts/build.py www",
        "`npm run check:web-smoke` and `npm run check:product`",
    ),
    SourceTruthRow(
        "compatibility/fixtures/product_compatibility.json",
        ("no generated output; protects saved config, backup, layout, and migration behavior",),
        "none",
        "`npm run check:backup-contract` and `npm run check:product`",
    ),
    SourceTruthRow(
        "`devices/catalog.json`, `common/config/card_contract.json`, `common/config/entity_names.json`, `common/assets/icons.json`, `compatibility/fixtures/product_compatibility.json`",
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
    "weather": "docs/card-types/weather.md",
    "image": "docs/card-types/cameras.md",
    "weather_forecast": "docs/card-types/weather-forecast.md",
}


CHECK_MATRIX_ROWS: tuple[CheckMatrixRow, ...] = (
    CheckMatrixRow(
        "`common/config/card_contract.json`",
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
        "`src/webserver/application/config_codec.ts`, `components/espcontrol/button_grid_config.h`, `compatibility/fixtures/product_compatibility.json`",
        "Saved card strings, backup/import/export shape, migration compatibility",
        "`npm run check:backup-contract` and `npm run check:firmware-parser`",
        "`npm run check:product` when compact config, backup, or migration behavior changes",
    ),
    CheckMatrixRow(
        "`devices/manifest.json`, `devices/<slug>/`, `builds/*.yaml`",
        "Supported hardware, layout slots, firmware package shape, release build metadata",
        "`npm run check:device-profiles` and `npm run check:device-matrix`",
        "`npm run check:product`; compile affected firmware before publishing new or changed device support",
    ),
    CheckMatrixRow(
        "`common/assets/icons.json`, `common/assets/*glyphs.yaml`, `devices/<slug>/device/fonts.yaml`",
        "Icon names, glyph coverage, firmware font roles, device font mappings",
        "`python3 scripts/check_icon_groups.py`",
        "`npm run check:product`; compile affected firmware when a visible font role or small-screen layout changes",
    ),
    CheckMatrixRow(
        "`common/config/entity_names.json`, entity name consumers",
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
        "`common/config/strings.*.txt`",
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


def contract_cards() -> dict[str, dict]:
    return read_json("common/config/card_contract.json")["cards"]  # type: ignore[index]


def package_scripts() -> set[str]:
    package = read_json("package.json")
    return set(package["scripts"].keys())  # type: ignore[index, union-attr]


def web_registration_map() -> dict[str, str]:
    out: dict[str, str] = {}
    for path in sorted((ROOT / "src/webserver/cards").glob("*.ts")):
        text = path.read_text()
        for match in re.finditer(r"registerButtonType\(\s*([\"'])(.*?)\1", text):
            out[match.group(2)] = rel(path)
        for match in re.finditer(
            r"registerCoverLikeCardType\(\s*\{.*?\btype\s*:\s*([\"'])(.*?)\1",
            text,
            flags=re.DOTALL,
        ):
            out[match.group(2)] = rel(path)
    return out


def firmware_header_map(card_types: list[str]) -> dict[str, list[str]]:
    out = {card_type: [] for card_type in card_types}
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
        "`common/config/card_contract.json` and the public documentation mapping in that script.\n\n"
        + markdown_table(("Public card page", "Covered saved type"), public_rows),
        "## Generated Matrix\n\n"
        "This table is generated from the card contract, `registerButtonType(...)` calls in "
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
    else:
        raise ValueError(path)
    current = (ROOT / path).read_text() if (ROOT / path).exists() else ""
    return replace_between(current, begin, end, content)


def check_generated_files(errors: list[str]) -> None:
    for path in ("dev-docs/source-of-truth.md", "dev-docs/card-type-map.md", "dev-docs/check-matrix.md"):
        full = ROOT / path
        if not full.exists():
            errors.append(f"{path} is missing; run python3 scripts/check_dev_docs.py --update")
            continue
        expected = expected_generated_text(path)
        if full.read_text() != expected:
            errors.append(f"{path} generated section is stale; run python3 scripts/check_dev_docs.py --update")


def source_truth_path_targets(value: str) -> list[str]:
    prefixes = ("common/", "components/", "compatibility/", "devices/", "docs/", "scripts/", "src/")
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
    check_generated_files(errors)
    check_source_truth_paths(errors)
    check_markdown_links(errors)
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
