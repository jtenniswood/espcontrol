#!/usr/bin/env python3
"""Generate compact English source strings for firmware screen text."""

from __future__ import annotations

import argparse
import re
import unicodedata
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "common" / "config" / "strings.en.txt"
SOURCE_GLOBS = ("components/espcontrol/**/*.h",)
SKIP_FILES = {"sun_calc.h"}

STRING_RE = re.compile(r"(?P<q>[\"'])(?P<body>(?:\\.|(?!\1).)*?)(?P=q)")
LOWER_DISPLAY_WORDS = {
    "activity",
    "auto",
    "away",
    "boost",
    "comfort",
    "eco",
    "high",
    "home",
    "low",
    "medium",
    "off",
    "on",
    "sleep",
}
IGNORED_EXACT = {
    "block",
    "button",
    "change",
    "click",
    "false",
    "input",
    "json",
    "label",
    "modal closed",
    "none",
    "null",
    "options",
    "precision",
    "send failed",
    "sensor",
    "source",
    "state",
    "status",
    "target",
    "true",
    "type",
    "undefined",
    "unit",
    "value",
    "warning",
    "yaml",
}
IGNORED_SUBSTRINGS = (
    "&amp;",
    "&lt;",
    "&times;",
    "${",
    "%",
    "/api/",
    "://",
    "<",
    "=",
    ">",
    "__",
    "::",
    "|",
    "LV_",
    "alarm_control_panel.",
    "automation.",
    "binary_sensor.",
    "button.",
    "climate.",
    "cover.",
    "device_tracker.",
    "entity_id",
    "fan.",
    "firmware__",
    "homeassistant",
    "input_",
    "light.",
    "media_player.",
    "mdi-",
    "number-",
    "script.",
    "screen__",
    "select.",
    "select-",
    "sensor.",
    "sp-",
    "std::",
    "subpage_",
    "switch.",
    "switch-",
    "text-",
    "update-",
    "{",
    "}",
)
IGNORED_PATTERNS = (
    re.compile(r"^[a-z0-9_./:-]+$"),
    re.compile(r"^#[0-9A-Fa-f]{3,8}$"),
    re.compile(r"^[A-Z0-9_]+$"),
    re.compile(r"^\d+(\.\d+)?$"),
    re.compile(r"^\.[A-Za-z0-9_-]+"),
)
NON_DISPLAY_KEYS = {
    "buttonClass",
    "domain",
    "domains",
    "entity",
    "field",
    "icon",
    "iconHtml",
    "icon_on",
    "id",
    "idSuffix",
    "key",
    "kind",
    "name",
    "options",
    "pickerKey",
    "service",
    "sensor",
    "storage",
    "type",
    "unit",
    "value",
}


def decode_cpp_string(value: str) -> str:
    if "\\" not in value:
        return value
    try:
        return bytes(value, "utf-8").decode("unicode_escape")
    except UnicodeDecodeError:
        return value


def candidate_line(line: str) -> bool:
    if "find_icon(" in line:
        return False
    markers = (
        "lv_label_set_text",
        "return ",
        "== ",
        "alarm_",
        "climate_",
        "fan_",
        "media_",
        "todo_",
    )
    return any(marker in line for marker in markers)


def display_context_allowed(line: str, start: int) -> bool:
    prefix = line[:start]
    key_match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*:\s*$", prefix)
    if key_match and key_match.group(1) in NON_DISPLAY_KEYS:
        return False
    comparison_match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:===|!==|==|!=)\s*$", prefix)
    if comparison_match and comparison_match.group(1) in {"icon", "service", "type", "value"}:
        return False
    if re.search(r"(?:^|[.\s])icon(?:_on)?\s*=\s*$", prefix):
        return False
    if re.search(r"find_icon\s*\(\s*$", prefix):
        return False
    return True


def is_display_string(value: str) -> bool:
    value = value.strip()
    if len(value) < 2 or len(value) > 120:
        return False
    if not any(ch.isalpha() for ch in value):
        return False
    if value.lower() in IGNORED_EXACT:
        return False
    if any(part in value for part in IGNORED_SUBSTRINGS):
        return False
    if "/" in value and value not in {"Heat/Cool", "Play/Pause"} and " / " not in value:
        return False
    if any(pattern.match(value) for pattern in IGNORED_PATTERNS):
        return False
    if len(value) <= 2 and value not in {"No", "On"}:
        return False
    if re.match(r"^[a-z][a-z0-9-]*$", value) and value not in LOWER_DISPLAY_WORDS:
        return False
    if re.match(r"^[A-Za-z]+[A-Za-z0-9_]*$", value) and not value[:1].isupper():
        return False
    if value.startswith(("icon_", "label_", "number_", "pin_")):
        return False
    return True


def slugify(value: str) -> str:
    normalized = unicodedata.normalize("NFKD", value)
    ascii_value = normalized.encode("ascii", "ignore").decode("ascii")
    slug = re.sub(r"[^a-zA-Z0-9]+", "_", ascii_value.lower()).strip("_")
    if not slug:
        slug = "string"
    if slug[0].isdigit():
        slug = f"n_{slug}"
    return slug


def add_string(strings: dict[str, set[str]], value: str, source: str) -> None:
    value = decode_cpp_string(value).strip()
    if not is_display_string(value):
        return
    strings.setdefault(value, set()).add(source)


def extract_strings() -> dict[str, set[str]]:
    strings: dict[str, set[str]] = {}
    files: list[Path] = []
    for pattern in SOURCE_GLOBS:
        files.extend(ROOT.glob(pattern))
    for path in sorted(set(files)):
        if path.name in SKIP_FILES or "generated" in path.name:
            continue
        rel = path.relative_to(ROOT).as_posix()
        for line_no, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
            if not candidate_line(line):
                continue
            for match in STRING_RE.finditer(line):
                if display_context_allowed(line, match.start()):
                    add_string(strings, match.group("body"), f"{rel}:{line_no}")
    return strings


def escape_compact_value(value: str) -> str:
    return value.replace("\\", "\\\\").replace("\n", "\\n").replace("=", "\\=")


def build_lines(strings: dict[str, set[str]]) -> list[str]:
    lines = [
        "# English source strings for hard-coded text rendered on screen by EspControl firmware.",
        "# Webserver-only text, entity names, service identifiers, icon names, and raw Home Assistant API values are excluded.",
        "# Format: key=value. Escape literal backslash, newline, and equals as \\\\, \\n, and \\=.",
    ]
    used_keys: set[str] = set()
    for value in sorted(strings, key=lambda item: item.lower()):
        key = slugify(value)
        base = key
        suffix = 2
        while key in used_keys:
            key = f"{base}_{suffix}"
            suffix += 1
        used_keys.add(key)
        lines.append(f"{key}={escape_compact_value(value)}")
    return lines


def run_self_test() -> None:
    assert candidate_line('lv_label_set_text(label, "WiFi Setup");')
    assert candidate_line('return "Heat/Cool";')
    assert not candidate_line('const auto icon = find_icon("Home");')

    display_line = 'lv_label_set_text(label, "WiFi Setup");'
    icon_line = 'icon = "Home";'
    service_line = 'service == "light.turn_on"'
    type_line = 'type: "media"'
    assert display_context_allowed(display_line, display_line.index('"WiFi Setup"'))
    assert not display_context_allowed(icon_line, icon_line.index('"Home"'))
    assert not display_context_allowed(service_line, service_line.index('"light.turn_on"'))
    assert not display_context_allowed(type_line, type_line.index('"media"'))

    accepted = ["WiFi Setup", "Heat/Cool", "Play/Pause", "On"]
    rejected = ["mdi-home", "light.kitchen", "label", "#ffffff", "bad_slug", "eco", "x"]
    for value in accepted:
        assert is_display_string(value), f"{value!r} should be accepted"
    for value in rejected:
        assert not is_display_string(value), f"{value!r} should be rejected"

    strings: dict[str, set[str]] = {}
    add_string(strings, "WiFi\\nSetup", "demo.h:1")
    add_string(strings, "Auto", "demo.h:2")
    add_string(strings, "mdi-home", "demo.h:3")
    assert "WiFi\nSetup" in strings
    assert "Auto" in strings
    assert "mdi-home" not in strings

    lines = build_lines(
        {
            "10 Minute Timer": {"demo.h:4"},
            "Mode=Auto": {"demo.h:5"},
            "Mode Auto": {"demo.h:6"},
        }
    )
    assert "n_10_minute_timer=10 Minute Timer" in lines
    assert "mode_auto=Mode Auto" in lines
    assert "mode_auto_2=Mode\\=Auto" in lines

    print("String inventory self-tests passed.")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true", help="run generator guardrail self-tests")
    args = parser.parse_args()

    if args.self_test:
        run_self_test()
        return

    strings = extract_strings()
    OUTPUT.write_text("\n".join(build_lines(strings)) + "\n")
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(f"Strings: {len(strings)}")


if __name__ == "__main__":
    main()
