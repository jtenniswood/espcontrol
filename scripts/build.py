#!/usr/bin/env python3
"""Unified build script for espcontrol.

Combines icon synchronization and www.js generation into a single tool.

Usage:
    python scripts/build.py               # run all generators
    python scripts/build.py --check       # exit 1 if any output is stale
    python scripts/build.py icons         # sync icons only
    python scripts/build.py www           # build www.js only
    python scripts/build.py icons --check # check icons only
"""
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# ---------------------------------------------------------------------------
# Shared paths
# ---------------------------------------------------------------------------
DEVICES_JSON = ROOT / "src" / "webserver" / "devices.json"
ICONS_JSON = ROOT / "common" / "assets" / "icons.json"


def load_json(path):
    with open(path) as f:
        return json.load(f)


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
# Icon sync (formerly sync_icons.py)
# ===========================================================================

def gen_icon_glyphs(data):
    """Font glyph codepoint list for LVGL font subsetting."""
    fb = data["fallback"]
    lines = [f'- "\\U{fb["codepoint"]:>08s}"  # mdi-{fb["mdi"]} (Auto fallback)\n']
    for icon in data.get("structural", []):
        comment = icon.get("comment", "")
        suffix = f" ({comment})" if comment else ""
        lines.append(f'- "\\U{icon["codepoint"]:>08s}"  # mdi-{icon["mdi"]}{suffix}\n')
    for icon in data["icons"]:
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
    """JS ICON_EXCEPTIONS + ICON_NAMES for www.js."""
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
    dirty = []

    icons_h = ROOT / "components" / "espcontrol" / "icons.h"
    icon_glyphs = ROOT / "common" / "assets" / "icon_glyphs.yaml"
    www_js = ROOT / "src" / "webserver" / "www.js"

    patches = [
        (icon_glyphs, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_icon_glyphs),
        (icons_h, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_icons_h_entries),
        (icons_h, "GENERATED:DOMAIN_ICONS START", "GENERATED:DOMAIN_ICONS END", gen_icons_h_domain_icons),
        (www_js, "GENERATED:ICONS START", "GENERATED:ICONS END", gen_www_js_icon_map),
        (www_js, "GENERATED:DOMAIN_ICONS START", "GENERATED:DOMAIN_ICONS END", gen_www_js_domain_icons),
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

WWW_SOURCE = ROOT / "src" / "webserver" / "www.js"
TYPES_DIR = ROOT / "src" / "webserver" / "types"
WWW_OUTPUT_DIR = ROOT / "docs" / "public" / "webserver"

CONFIG_START = "__DEVICE_CONFIG_START__"
CONFIG_END = "__DEVICE_CONFIG_END__"
TYPES_START = "__BUTTON_TYPES_START__"
TYPES_END = "__BUTTON_TYPES_END__"


def build_config_block(slug, cfg):
    cfg_lines = json.dumps(cfg, indent=2).splitlines()
    cfg_body = "\n".join("  " + line for line in cfg_lines[1:])
    return (
        f'  var DEVICE_ID = "{slug}";\n'
        f"  var CFG = {cfg_lines[0]}\n"
        f"{cfg_body};\n"
    )


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


def replace_types(source_text):
    pattern = re.compile(
        r"(^[^\n]*" + re.escape(TYPES_START) + r"[^\n]*\n)"
        r"(.*?)"
        r"(^[^\n]*" + re.escape(TYPES_END) + r"[^\n]*$)",
        re.MULTILINE | re.DOTALL,
    )
    m = pattern.search(source_text)
    if not m:
        return source_text
    return source_text[: m.start(2)] + load_button_types() + source_text[m.start(3) :]


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


def build_www(check_only=False):
    """Build per-device www.js from the single source template."""
    devices = load_json(DEVICES_JSON)
    source_text = WWW_SOURCE.read_text()
    source_text = replace_types(source_text)
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

    for cmd in commands:
        if cmd == "all":
            icon_dirty = sync_icons(check_only=check_only)
            www_dirty = build_www(check_only=check_only)
            if check_only and (icon_dirty or www_dirty):
                exit_code = 1
            elif not icon_dirty and not www_dirty:
                print("All outputs are up to date.")
            else:
                total = len(icon_dirty) + len(www_dirty)
                print(f"Updated {total} target(s).")
        elif cmd == "icons":
            dirty = sync_icons(check_only=check_only)
            if check_only and dirty:
                exit_code = 1
            elif not dirty:
                print("Icon data is in sync.")
            else:
                print(f"Synced {len(dirty)} section(s).")
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
            print("Usage: python scripts/build.py [all|icons|www] [--check]")
            exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
