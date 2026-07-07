---
name: translations
description: Find static user-visible words shown on the physical EspControl device and integrate them into the firmware translation process. Use when the user asks to add, audit, fix, or review firmware/device UI translations, hard-coded display text, i18n keys, or physical screen/card/modal/status text in the espcontrol repository.
---

# Translations

## Purpose

Use this skill to move static physical-device UI text into the EspControl firmware translation workflow. Keep the work narrow: firmware display text only, no webserver/setup-configurator-only strings, no Home Assistant dynamic content, and no unrelated refactors.

## Scope

Count only text that can appear on the physical EspControl display, including:

- Screen, card, modal, setup, and status labels.
- Button labels and confirmation text.
- Empty states and user-visible error/status messages.

Do not translate or change:

- `src/webserver/` setup/configurator-only UI text.
- Home Assistant-provided entity names, states, attributes, service names, IDs, or icon names.
- Log messages, code comments, internal code strings, API strings, generated identifiers, or CSS/HTML implementation details.
- Dynamic content supplied by integrations or Home Assistant.

## Workflow

1. Start from a clean feature branch/worktree based on latest `origin/main` unless the user explicitly says otherwise.
2. Search for hard-coded user-visible strings mainly in:
   - `components/espcontrol/`
   - `common/device/`
   - `common/addon/` when text can appear on the display
   - `common/config/strings.en.txt`
3. Avoid treating `src/webserver/` strings as in scope unless the same string is shared with firmware output and appears on the physical display.
4. For each candidate string, classify it before editing:
   - Already present in `common/config/strings.en.txt` and rendered through `espcontrol_i18n(...)` or `espcontrol_i18n_key(...)`: leave it alone.
   - Present in `common/config/strings.en.txt` but displayed as raw English: update the firmware code to use the translation helper.
   - Passed into a local helper that later renders text through `espcontrol_i18n(...)`: make sure the English value exists in every `common/config/strings.*.txt` file.
   - Returned from a local helper and later displayed without another translation call: translate the known static return values before returning or before display.
   - Missing from `common/config/strings.en.txt`: add a stable `snake_case` key to `strings.en.txt` and add the matching key to every `common/config/strings.*.txt` file.
5. For non-English translation files, add a reasonable translation. If uncertain, use the English source text instead of guessing badly.
6. Regenerate firmware translation output:

```bash
python3 scripts/build.py i18n
```

7. Verify:

```bash
python3 scripts/build.py i18n --check
npm run check:product
```

If `npm run check:product` fails because `esbuild` is missing in a fresh worktree, run `npm ci` in that worktree and rerun the check. Mention any npm audit warnings, but do not fix unrelated dependency issues as part of translation work.

8. Commit and push the branch, then open a ready-for-review pull request.

## Search Guidance

Use targeted searches and then inspect context manually. Useful starting points:

```bash
rg --line-number --glob '!src/webserver/**' '"[^"]*[A-Za-z][^"]*"' components/espcontrol common/device common/addon common/config
rg --line-number "espcontrol_i18n|espcontrol_i18n_key|strings\\.en\\.txt" components/espcontrol common/device common/addon common/config
rg --line-number "lv_label_set_text\([^\n]*(\"|std::string|sentence_cap_text)|text:\s*\"|text:\s*!lambda" components/espcontrol common/device common/addon --glob '!components/espcontrol/i18n_generated.h'
rg --line-number "static const char \*|const char \*.*\[\]|std::array<.*char|std::vector<.*string" components/espcontrol --glob '!components/espcontrol/i18n_generated.h'
```

Treat search results as candidates, not proof. Many strings in firmware code are not translatable UI text.

When the broad search is too noisy, prioritize:

- `lv_label_set_text(...)` and YAML `text:` values that can render on the physical display.
- Local status/helper functions such as `*_set_status(...)`, `*_label(...)`, or `*_loading_state(...)` that accept a static English string and later set a label.
- Static arrays or return branches that provide user-visible labels, not icon names or API values.
- Existing helper-wrapped literals missing from the translation source files.

Use this audit script after inspecting likely candidates to catch helper-wrapped text that is missing from `strings.en.txt`:

```bash
python3 - <<'PY'
from pathlib import Path
import re

root = Path.cwd()
files = (
    list((root / "components/espcontrol").glob("*.h")) +
    list((root / "common/device").glob("*.yaml")) +
    list((root / "common/addon").glob("*.yaml"))
)
pattern = re.compile(r'espcontrol_i18n\(\s*(?:std::string\()?"((?:[^"\\]|\\.)*)"')
used = {}
for path in files:
    if path.name == "i18n_generated.h":
        continue
    text = path.read_text(errors="ignore")
    for match in pattern.finditer(text):
        value = bytes(match.group(1), "utf-8").decode("unicode_escape")
        used.setdefault(value, []).append(
            (str(path.relative_to(root)), text.count("\n", 0, match.start()) + 1)
        )

english = {}
for line in (root / "common/config/strings.en.txt").read_text().splitlines():
    if not line.strip() or line.lstrip().startswith("#") or "=" not in line:
        continue
    key, value = line.split("=", 1)
    english[value.encode("utf-8").decode("unicode_escape")] = key

missing = {value: locations for value, locations in used.items() if value not in english}
print(f"used literals {len(used)} missing {len(missing)}")
for value, locations in sorted(missing.items()):
    print(repr(value))
    for filename, line_no in locations[:8]:
        print(f"  {filename}:{line_no}")
PY
```

Use this narrower raw-literal audit to find English-looking text that is not directly wrapped in `espcontrol_i18n(...)`. Inspect each hit manually; many are logs, templates, icons, option values, or Home Assistant data and must not be translated.

```bash
python3 - <<'PY'
from pathlib import Path
import re

root = Path.cwd()
paths = (
    list((root / "components/espcontrol").glob("*.h")) +
    list((root / "common/device").glob("*.yaml")) +
    list((root / "common/addon").glob("*.yaml"))
)
skip_context = [
    "ESP_LOG", "find_icon", "icon_", "mdi:", "name:", "id:", "platform:",
    "unit_of_measurement:", "entity_", "CARD_CONTRACT", "#include", "font:",
    "color:", "file:", "path:", "url:", "mode:", "type:", "sensor:",
    "service:", "attribute:", "lambda:",
]
string_re = re.compile(r'"((?:[^"\\]|\\.)*)"')
for path in paths:
    if path.name in {"i18n_generated.h", "button_grid_contract_generated.h"}:
        continue
    for line_no, line in enumerate(path.read_text(errors="ignore").splitlines(), 1):
        if "espcontrol_i18n" in line or "espcontrol_i18n_key" in line:
            continue
        if any(value in line for value in skip_context):
            continue
        for match in string_re.finditer(line):
            raw = match.group(1)
            try:
                value = bytes(raw, "utf-8").decode("unicode_escape")
            except Exception:
                value = raw
            if not re.search(r"[A-Za-z]", value):
                continue
            if not (re.search(r"[A-Z][a-z]", value) or " " in value or "?" in value):
                continue
            if len(value) <= 2:
                continue
            print(f"{path.relative_to(root)}:{line_no}: {value} | {line.strip()}")
PY
```

## Editing Rules

- Keep changes focused on translation wiring and translation files.
- Prefer existing i18n patterns in nearby code.
- Use stable, descriptive `snake_case` keys.
- Do not rename existing keys unless required for correctness.
- Do not translate Home Assistant dynamic content.
- Do not change icons, entity IDs, service names, API strings, generated constants, or unrelated UI behavior.
- Do not refactor unrelated code while touching translation call sites.

## Pull Request Requirements

The PR description must include:

- What device UI text was added to translations.
- What code paths were changed to use translation helpers.
- Which checks passed.
- Device testing notes explaining which screen, card, modal, or setup/status flow should be checked after flashing.

If firmware flashing is useful for confidence, name the affected display or device in the PR body. Do not close related GitHub issues until the user confirms device testing works.
