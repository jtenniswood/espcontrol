#!/usr/bin/env python3
"""Generate compact English source strings for firmware screen text."""

from __future__ import annotations

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


def main() -> None:
    strings = extract_strings()
    OUTPUT.write_text("\n".join(build_lines(strings)) + "\n")
    print(f"Wrote {OUTPUT.relative_to(ROOT)}")
    print(f"Strings: {len(strings)}")


if __name__ == "__main__":
    main()
