#!/usr/bin/env python3
"""Read-only structural audit of EspControl firmware translation sources."""

import argparse
import ast
import json
from pathlib import Path
import re
import subprocess


def compact_value(value):
    # Match scripts/build.py without unicode_escape corrupting native UTF-8.
    return re.sub(r"\\(.)", lambda m: "\n" if m[1] == "n" else m[1], value)


def load_strings(path, root, errors):
    result = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        location = f"{path.relative_to(root)}:{number}"
        if "=" not in line:
            errors.append(f"{location}: malformed entry")
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        if not key or key in result:
            errors.append(f"{location}: empty or duplicate key {key!r}")
            continue
        result[key] = compact_value(value)
        if not result[key].strip():
            errors.append(f"{location}: empty translation for {key}")
    return result


def declared_languages(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    for index, line in enumerate(lines):
        if line.strip() != "id: language_select":
            continue
        indent = len(line) - len(line.lstrip())
        options = False
        codes = []
        for following in lines[index + 1:]:
            if not following.strip():
                continue
            level = len(following) - len(following.lstrip())
            if level < indent or (options and level <= indent):
                break
            if following.strip() == "options:":
                options = True
            elif options:
                match = re.fullmatch(r'''\s*-\s*["']?([a-z]+(?:-[a-z]+)?)["']?\s*''', following)
                if not match:
                    raise ValueError("Unrecognized language_select option: " + following.strip())
                codes.append(match[1])
        if codes:
            if len(set(codes)) != len(codes):
                raise ValueError("Duplicate language_select options")
            return set(codes)
    raise ValueError("Cannot read language_select options; inspect common/addon/time.yaml manually")


# Keep line offsets while removing C++ comments; preserve quoted strings/URLs.
TOKENS = re.compile(r'''"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|//[^\n]*|/\*[\s\S]*?\*/''')
STRING = r'"(?:\\.|[^"\\])*"'
CALL = re.compile(r'\bespcontrol_i18n(_key)?\s*\(\s*(?:std::string\s*\(\s*)?(' + STRING + r'(?:\s*' + STRING + r')*)')


def audit(root):
    errors = []
    directory = root / "product/v2/translations"
    english_path = directory / "strings.en.txt"
    if not english_path.is_file():
        raise ValueError("No product/v2/translations/strings.en.txt in the selected repository")
    english = load_strings(english_path, root, errors)
    if not english:
        errors.append("English source dictionary is empty")
    languages = {}
    identical = {}
    for path in sorted(directory.glob("strings.*.txt")):
        code = path.stem.split(".", 1)[1]
        values = english if code == "en" else load_strings(path, root, errors)
        languages[code] = len(values)
        missing, extra = sorted(english.keys() - values.keys()), sorted(values.keys() - english.keys())
        if missing:
            errors.append(f"{code}: missing keys: {', '.join(missing)}")
        if extra:
            errors.append(f"{code}: extra keys: {', '.join(extra)}")
        if code != "en":
            identical[code] = [key for key in english if key in values and values[key] == english[key]]
    try:
        declared = declared_languages(root / "common/addon/time.yaml")
        for code in sorted(declared - languages.keys()):
            errors.append(f"Missing language file: strings.{code}.txt")
        for code in sorted(languages.keys() - declared):
            errors.append(f"Language file has no language_select option: {code}")
    except (OSError, ValueError) as exc:
        errors.append(str(exc))
    calls = 0
    missing_calls = []
    literal_sources = {value for key, value in english.items() if not key.startswith("label_")}
    for folder in ("components/espcontrol", "common/device", "common/addon"):
        base = root / folder
        if not base.is_dir():
            errors.append(f"Missing source directory: {folder}")
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in {".h", ".hpp", ".cpp", ".cc", ".yaml", ".yml"} or "generated" in path.name:
                continue
            text = path.read_text(encoding="utf-8")
            if path.suffix in {".yaml", ".yml"}:
                text = re.sub(r"(?m)^[ \t]*#[^\n]*", lambda m: " " * len(m[0]), text)
            text = TOKENS.sub(lambda m: re.sub(r"[^\n]", " ", m[0]) if m[0].startswith(("//", "/*")) else m[0], text)
            for match in CALL.finditer(text):
                calls += 1
                location = f"{path.relative_to(root)}:{text.count(chr(10), 0, match.start()) + 1}"
                try:
                    value = "".join(ast.literal_eval(token[0]) for token in re.finditer(STRING, match[2]))
                except (SyntaxError, ValueError) as exc:
                    errors.append(f"{location}: cannot decode literal: {exc}")
                    continue
                exists = value in (english if match[1] else literal_sources)
                if not exists:
                    missing_calls.append({"location": location, "lookup": "key" if match[1] else "text", "value": value})
    for call in missing_calls:
        errors.append(f"{call['location']}: missing {call['lookup']} lookup {call['value']!r}")
    return {"root": str(root), "source_keys": len(english), "languages": languages,
            "direct_literal_calls": calls, "missing_calls": missing_calls,
            "identical_to_english_candidates": identical, "errors": errors,
            "limitations": "Direct literal calls only; trace raw labels, indirect helpers/arrays, YAML and rendering paths manually. Identical values require language review."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, help="Repository/worktree root; defaults to the current Git root")
    parser.add_argument("--json", action="store_true", help="Print detailed JSON")
    args = parser.parse_args()
    try:
        root = args.root or Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
        report = audit(root.resolve())
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.exit(2, f"Audit could not run: {exc}\n")
    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        print(f"{report['source_keys']} source keys, {len(report['languages'])} language files, {report['direct_literal_calls']} direct literal calls")
        for error in report["errors"]:
            print("ERROR:", error)
        print("Identical-to-English candidates (not automatic failures):")
        for code, keys in report["identical_to_english_candidates"].items():
            print(f"  {code}: {', '.join(keys) or '(none)'}")
        print("Structural result:", "FAIL" if report["errors"] else "PASS")
        print(report["limitations"])
    return 1 if report["errors"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
