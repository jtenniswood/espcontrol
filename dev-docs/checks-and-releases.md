# Checks and Releases

Use the generated [Check Matrix](check-matrix.md) first when you are choosing
checks from changed paths. This page explains the broader release-facing checks
and when to raise confidence beyond the minimum route.

Use the smallest check that covers the change while developing, then run the
broader checks before merging or publishing.

The public npm check commands are now compatibility entry points for the
dependency-aware task graph. Focused commands include their declared prerequisites
automatically, while the product, fast, CI, all, and release commands run complete
assurance profiles. Temporary `:legacy` aliases preserve the previous command
chains for one release cycle while the migration is observed.

Use `python3 scripts/check_tasks.py list` to see registered tasks or
`python3 scripts/check_tasks.py plan fast --explain` to preview a profile without
running it. The registry in `scripts/check_tasks_data.py` is the maintained source
for task commands, dependencies, profiles, domains, input paths, and parallel
safety.

All existing check aliases and CI use one worker by default. For an opt-in local
fast run with no more than four workers, use:

```bash
npm run check:parallel
```

Only dependency-independent tasks explicitly marked parallel-safe may overlap.
Their output is captured and replayed as complete task blocks. Browser, release,
Git-state, and shared-output checks always run alone; release profiles use one
worker even if a larger `--jobs` value is requested. After the first failure, no
new tasks start, already-running tasks finish, and dependent tasks are reported
as blocked.

The initial Darwin arm64 benchmark on 2026-07-12 ran the complete non-browser
fast profile 20 times sequentially and 20 times with four workers. Every run
passed with the same task statuses and left the tracked diff fingerprint
unchanged. Median duration improved from 11.850 seconds to 9.635 seconds, an
18.7% reduction. Because that is below the planned 20% threshold, parallel mode
remains explicit-only through `check:parallel`; normal npm aliases and CI retain
their one-worker default.

Successful deterministic local checks are cached by content. Entries live under
the repository's shared Git directory, so linked worktrees can reuse them without
copying or restoring generated files. A cache key includes the task and command,
dependency keys, every declared authored and generated input, the runner and
registry, lockfiles, operating system and architecture, tool versions, and each
environment variable declared by the task. Any change to those values causes a
fresh check.

Use `--no-cache` on a profile run when fresh local execution is required. Inspect
or remove the shared entries with:

```bash
python3 scripts/check_tasks.py cache status
python3 scripts/check_tasks.py cache clear
```

Only successful status is stored, and corrupt entries are treated as misses.
Local-artifact, PR-process, Git-history, release-confidence, changelog,
firmware-release, external-state, and shared-output checks are never cached.
Browser smoke is eligible only when Playwright, Node, the resolved browser
executable, generated layouts, and all web inputs are fingerprinted. `CI=true`
disables result caching entirely, so CI always validates from scratch.

For an advisory local route based on everything changed from `main`, including
committed, staged, unstaged, renamed, deleted, and untracked files, run:

```bash
python3 scripts/check_tasks.py changed --explain
```

This command never reduces CI coverage. Unknown paths and changes to shared
script helpers, generators, validators, the task runner, registry, package lock,
or workflow definitions select the complete fast profile. Profiles can also be
narrowed explicitly by domain, for example
`python3 scripts/check_tasks.py run ci --domain web`.

## Common Checks

| Command | Use when |
|---|---|
| `npm run check:product` | Product schema, generated outputs, web smoke, backup compatibility, device profiles, and release-facing metadata changed. |
| `npm run check:fast` | Broad local pre-commit validation. |
| `npm run check:ci` | CI-equivalent fast checks plus browser smoke. |
| `npm run check:all` | Fast checks plus public docs build. |
| `npm run docs:build` | Public docs content or VitePress config changed. |
| `python3 scripts/build.py --check` | Generated outputs might be stale. |

## Verification Ladder

Use the task playbook's ladder when one exists. Each ladder separates quick
local checks from broader release checks:

| Level | Purpose | Typical stopping point |
|---|---|---|
| Minimum | Prove the exact file contract touched by the change still holds. | Small, narrow changes that do not touch release-facing generated files. |
| Recommended | Run the normal product-level safety net for that task. | Most feature, config, device, card, and generated-output changes. |
| Release-grade | Add expensive or broad checks such as `npm run check:fast`, public docs build, browser smoke, or firmware compile. | Publishing, release prep, broad shared behavior changes, or firmware-visible changes. |

For docs-only changes, run `npm run docs:build`, then search the built docs for
internal developer-doc paths such as `dev-docs/` so private guidance does not
leak into the public site.

## Focused Checks

| Command | Covers |
|---|---|
| `npm run check:config` | Shared config formats. |
| `npm run check:model-contract` | Generated web model contract. |
| `npm run check:backup-contract` | Backup import/export compatibility. |
| `npm run check:card-contract-outputs` | Card contract generated output consistency. |
| `npm run check:web-smoke` | Web setup bundle logic without a real browser. |
| `npm run check:web-browser-smoke` | Browser-level setup page smoke test. |
| `npm run check:firmware-parser` | Firmware config parser behavior. |
| `npm run check:firmware-modals` | Modal-related firmware checks. |
| `npm run check:firmware-card-runtime` | Card runtime behavior checks. |
| `npm run check:firmware-display-tokens` | Display token validation. |
| `npm run check:firmware-ha-bindings` | Home Assistant binding validation. |
| `npm run check:device-matrix` | Device matrix consistency. |
| `npm run check:device-profiles` | Public and firmware device profile data. |
| `npm run check:release-confidence` | Release readiness metadata. |
| `npm run check:release-changelog` | Release changelog expectations. |
| `npm run check:public-firmware-script` | Public firmware script behavior. |

## When to Rebuild

Run `python3 scripts/build.py` after changing:

- `common/config/card_contract.json`
- `common/config/entity_names.json`
- `common/assets/icons.json`
- firmware translation strings under `common/config/strings.*.txt`
- `devices/manifest.json`
- `src/webserver/`

Run `python3 scripts/generate_device_slots.py` after changing device font roles
or slot/profile data that affects generated `sensors.yaml`.

## Release-Sensitive Files

Treat these as release-facing:

- `builds/*.yaml`
- `builds/*.factory.yaml`
- `devices/manifest.json`
- `docs/public/device-profiles.json`
- `docs/public/webserver/*/www.js`
- `docs/generated/screens/*.md`
- `docs/generated/cards/capabilities.md`
- `compatibility/fixtures/product_compatibility.json`

Changes here usually deserve `npm run check:product` at minimum.

## Practical Pre-Commit Sequence

For most code or product changes:

```bash
python3 scripts/build.py --check
npm run check:product
```

For broad changes, release preparation, or anything touching generated product
surfaces:

```bash
npm run check:fast
npm run docs:build
```

For firmware changes, also compile the affected device with ESPHome before
publishing.

The ESPHome Docker image version used by firmware compile, nightly firmware, and
release firmware workflows is set in `.github/esphome.env`. Update that one file
when moving to a new ESPHome release.

The `Firmware Compile` GitHub workflow is manual-only. Start it with
`workflow_dispatch` when a PR needs a full firmware compile, especially for
firmware-visible paths such as `common/`, `components/`, `devices/`, `builds/`,
generated web bundles, or the device build scripts.

If generated inputs changed, make sure the regenerated outputs are committed too.
`python3 scripts/build.py --check` is the command that catches stale or missing
generated files before CI does.
