---
name: translation-audit
description: Audit EspControl physical-display text for translation wiring and complete language coverage. Use for translation audits, hard-coded firmware UI text checks, missing language entries, or English fallback reviews. Web configurator localization is outside this skill unless explicitly requested.
---

# Translation Audit

Audit the current EspControl worktree and report evidence separately for translation wiring, dictionary coverage, and wording. Default to a read-only audit; apply fixes when the user requests them. Follow the active repository instructions for any branch, commit, push, or PR work. Do not create a worktree just to inspect files.

## Run the deterministic audit

Resolve the repository with `git rev-parse --show-toplevel` and record the branch/commit being audited. Run the bundled helper from that root:

```bash
python3 .agents/skills/translation-audit/scripts/audit_translations.py --root .
```

Use `--json` when a detailed machine-readable report is useful. The helper reads files without modifying them and checks:

- Languages declared by `language_select` in `common/addon/time.yaml` against `product/v2/translations/strings.*.txt`.
- Duplicate keys, malformed entries, missing/extra keys, and whitespace-only translations.
- Direct literal `espcontrol_i18n(...)` and `espcontrol_i18n_key(...)` calls in recursive firmware C++ and device/addon YAML sources against the English dictionary, excluding generated files.
- Entries identical to English, grouped by language as review candidates, not automatic failures.

A clean helper result is structural coverage, not proof that every visible string is translated. Unexpected parser shapes or undecodable literals require inspection; do not silently count them as covered.

## Trace device text manually

Inspect candidates in `components/espcontrol/`, `common/device/`, and `common/addon/`. Start with label setters, YAML `text:`, status helpers, default card labels, and static arrays/return values:

```bash
rg -n 'lv_label_set_(display_)?text|text:|espcontrol_i18n' components/espcontrol common/device common/addon --glob '!**/*generated*'
rg -n 'set_status|set_loading_state|show_failure|const char \*|return "' components/espcontrol --glob '!**/*generated*'
```

Trace raw English through its caller and rendering helper before classifying it. A helper may already translate it; conversely, a translated-looking call can still return English if its source value is absent. Check indirect month/weekday/compass labels, modal headings, error/loading/empty states, confirmation defaults, setup/update/connection messages, and built-in alarm/climate/robot modes. Include multiline and concatenated strings.

Exclude logs, icons, API/service/entity identifiers, Home Assistant-provided content, user-authored labels, URLs, numeric placeholders, and product names. Inspect text in `src/webserver/` only if it is shared with physical firmware output or the user explicitly expands scope. Do not feed arbitrary entity names or options into translation lookup just because they resemble English labels.

Review identical-to-English entries in context. Proper names, loanwords, and shared spellings are legitimate. Phrases such as an English connection explanation in every locale are likely fallbacks. Do not use a percentage of differing strings as a translation-quality score, or replace correct native wording merely because it differs from a preferred translation.

## Fix and verify when requested

- Prefer nearby translation patterns and stable existing keys. For a new source string, add a descriptive `snake_case` key to English and every supported language. Use key lookup for context-sensitive static labels where text lookup could collide with dynamic content.
- Add reasonable native translations; explicitly report any uncertain English fallbacks instead of claiming full language coverage. Do not invent unsupported wording or imply native-speaker review occurred.
- Regenerate with `python3 scripts/build.py i18n`, then rerun this audit and `python3 scripts/build.py i18n --check`.
- Run `npm run check:product` for translation fixes. If dependencies are missing, use the repository's locked dependency installation and its documented Python requirements; report unrelated advisories without changing dependencies.
- For generator/lookup changes, or when useful to verify escaping and dispatch, compile a temporary host C++ harness against `i18n_generated.h`. Compare all language/key lookups with decoded source dictionaries. Literal lookup uses the first matching English value, excludes `label_` keys, and may legitimately return the original text. Also check that arbitrary dynamic strings remain unchanged. Temporary harnesses should not enter the repository unless ongoing regression coverage is warranted.
- Firmware compilation and physical testing are separate evidence. Do not claim either from host or product checks. When opening a PR, use generated testing guidance and preserve required checklist formatting, including `Docs notes:` on its own line. Name affected displays and screens. Honor an explicit physical-testing waiver without marking the device tested or bypassing GitHub protections.

## Report

Lead with concrete gaps found or no gaps found in the inspected scope. Include language/key counts, unresolved structural errors, direct-call coverage, manually inspected rendering paths, and confirmed versus uncertain English fallbacks. State which checks actually ran and any uninspected scope. Explain that text fit, glyphs, line breaks, RTL layout, and native wording require appropriate device/language review. Avoid claiming exhaustive UI or linguistic correctness from key parity alone.
