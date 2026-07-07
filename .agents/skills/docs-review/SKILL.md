---
name: docs-review
description: Review EspControl repository changes specifically for missing, stale, excessive, or misplaced documentation. Use when the user asks for a documentation coverage review, asks whether a feature or behavior change is concisely documented, invokes $docs-review, or explicitly wants docs checked as a secondary pass during a pull request, branch, or local diff review.
---

# Docs Review

Review documentation coverage without turning every code change into a writing
task. Require docs only when a user, tester, or maintainer would reasonably need
the information to install, configure, test, troubleshoot, upgrade, release, or
maintain the change.

## Workflow

1. Read the request, PR summary, changed files, and tests.
2. Compare the diff with likely EspControl documentation targets:
   - Public docs: `docs/`, `README.md`, generated product pages, and card or
     feature pages.
   - Maintainer docs: `dev-docs/`, `DEVELOPERS.md`, workflow files, and PR
     templates.
   - Generated docs only as outputs; ask for the source or generator update when
     generated docs are stale.
3. Decide whether documentation is needed. Do not ask for docs when the change
   is purely internal and no user, tester, or maintainer workflow changes.
4. Report only actionable documentation findings, ordered by importance.
5. Suggest the smallest useful update: usually one sentence, one bullet, one
   table row, or a short section in an existing page.

## Ask For Docs When The Change

- Adds, removes, renames, or changes a card type, card option, modal, setup
  field, backup/import/export behavior, Home Assistant entity mapping, or saved
  config behavior.
- Changes visible firmware behavior, display layout, language strings, icons,
  fonts, screen timing, update flow, or device-specific behavior.
- Adds or changes supported hardware, build outputs, flashing steps, release
  behavior, or device profile data.
- Changes compatibility expectations for saved configs, backups, migrations,
  public web assets, existing installed devices, or Home Assistant integration.
- Changes maintainer workflows, checks, generators, release steps, source of
  truth ownership, branching, testing, or troubleshooting paths.

## Prefer Concise Placement

- Prefer updating an existing page over adding a new page.
- Prefer `docs/` for install, setup, card usage, visible behavior,
  troubleshooting, and upgrade impact.
- Prefer `dev-docs/`, `DEVELOPERS.md`, or `.github/` for build, release,
  generator, source-of-truth, test, and internal workflow changes.
- Avoid implementation detail unless it affects setup, testing, troubleshooting,
  compatibility, or upgrade decisions.
- Avoid blocking a PR for broad docs polish, duplicated explanations, or
  unrelated cleanup.

## Output

Lead with findings. For each finding include:

- The changed behavior or feature.
- The recommended documentation target.
- Who would be confused without it.
- The smallest useful wording or location when practical.

If no documentation change is needed, say that clearly in one sentence and give
the reason. Then mention any checks used, such as `npm run check:dev-docs`,
`npm run docs:build`, or other repository-specific documentation validators.
