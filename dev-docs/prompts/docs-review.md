# Documentation Review Prompt

Use this prompt to review a pull request, branch, or local diff for missing,
stale, or overly long documentation. The goal is not to document every code
change. The goal is to make sure new user-visible behavior, setup changes,
device support, compatibility notes, and maintainer workflows are easy to
understand without bloating the docs.

```text
You are reviewing EspControl documentation coverage for this change.

Goal:
Make sure new features, behavior changes, supported-device changes, setup steps,
compatibility impacts, and maintainer workflows are documented clearly and
concisely. Do not ask for docs when the change is purely internal and no user,
tester, or maintainer would reasonably need to know about it.

Review inputs:
- Read the PR summary, changed files, and tests.
- Compare the diff with the public docs under docs/, internal developer docs
  under dev-docs/, README.md, DEVELOPERS.md, and generated docs where relevant.
- Treat generated docs as outputs. If generated docs are stale, ask for the
  source or generator to be updated rather than hand-editing generated files.

Check for missing documentation when the change:
- Adds, removes, renames, or changes a card type, card option, modal, setup
  field, backup/import/export behavior, or Home Assistant entity mapping.
- Changes visible firmware behavior, display layout, language strings, icons,
  fonts, screen timing, update flow, or device-specific behavior.
- Adds or changes supported hardware, build outputs, flashing steps, release
  behavior, or device profile data.
- Changes compatibility expectations for saved configs, backups, migrations,
  public web assets, or existing installed devices.
- Changes maintainer workflows, checks, generators, release steps, or source of
  truth ownership.

Prefer concise documentation:
- Ask for one precise sentence, bullet, table row, or short section when that is
  enough.
- Prefer updating an existing page over adding a new page.
- Prefer user-facing docs for install, setup, card usage, and visible behavior.
- Prefer dev-docs for internal build, release, generator, source-of-truth, and
  maintainer workflow changes.
- Avoid repeating implementation details unless they affect a user's setup,
  testing, troubleshooting, or upgrade path.

When reporting findings:
- Lead with actionable issues only.
- For each issue, name the missing or stale documentation target and explain who
  would be confused without it.
- Suggest the smallest useful wording or location when practical.
- Do not block the PR for broad documentation polish, duplicated explanations,
  or unrelated docs cleanup.
- If no docs change is needed, say that clearly and explain why in one sentence.

Output format:
- Findings first, ordered by importance.
- Each finding should include the affected change, the recommended docs location,
  and the smallest useful update.
- Then list any checks you used, such as npm run check:dev-docs or
  npm run docs:build.
```

## Quick Decision Guide

| Change type | Usually document in |
|---|---|
| New or changed card behavior | `docs/card-types/` and possibly `docs/generated/cards/capabilities.md` through the generator |
| Setup page or backup behavior | `docs/features/setup.md`, `docs/features/backup.md`, or `dev-docs/web-configurator.md` |
| Device support or flashing behavior | `docs/screens/`, `docs/generated/screens/`, or `dev-docs/devices-and-builds.md` |
| Firmware UI behavior | The relevant `docs/features/` or `docs/card-types/` page |
| Compatibility or migration behavior | `dev-docs/compatibility-contract.md` and any affected public troubleshooting/setup page |
| Maintainer workflow or checks | `dev-docs/`, `DEVELOPERS.md`, or `.github/pull_request_template.md` |

Keep review comments small and specific. A good documentation review prevents a
tester or maintainer from guessing what changed; it does not turn every feature
PR into a writing project.
