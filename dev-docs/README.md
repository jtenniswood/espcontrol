# EspControl Developer Reference

These notes are for contributors and maintainers working on EspControl firmware,
the web configurator, generated files, or release outputs. End-user install and
usage instructions live in `README.md` and the public documentation under
`docs/`.

This folder is intentionally outside the public VitePress documentation site. It
is the source of truth for repo-internal developer guidance.

## Quick Start

Install local dependencies before running generators or checks:

```bash
npm ci
```

Developer tooling expects:

- Node.js for the web bundle and JavaScript checks.
- Python 3 for `scripts/build.py` and Python validators.
- ESPHome CLI for compiling, flashing, and logging firmware.

Common starting commands:

```bash
python3 scripts/build.py            # run generators
python3 scripts/build.py --check    # confirm generated output is current
npm run check:dev-docs              # confirm internal docs tables and links
npm run check:product               # product-level safety net
npm run check:fast                  # broader pre-commit check
```

For broad review scans, prefer:

```bash
rg --files
```

It respects ignore rules and avoids noisy local folders such as `.cache/`,
`.esphome/`, `.pio-core/`, `.uv-cache/`, and `.worktrees/`. If `find` is
needed, prune those folders explicitly so temporary build output does not hide
source files.

## Most Common Tasks

| If you need to... | Start here |
|---|---|
| Choose the right path for a broad request | [Task Router](task-router.md) |
| Add or change a card | [Add or Change a Card Type](playbooks/add-card-type.md) and [Card Type Map](card-type-map.md) |
| Change saved settings, backups, or compact config strings | [Change Saved Config](playbooks/change-saved-config.md) and [Compatibility Contract](compatibility-contract.md) |
| Add or change supported hardware | [Add or Change a Supported Device](playbooks/add-supported-device.md) |
| Change icons, glyphs, or firmware font roles | [Change Fonts or Icons](playbooks/change-fonts-or-icons.md) and [Font Guidelines](font-guidelines.md) |
| Work out which check to run | [Check Matrix](check-matrix.md) |
| Diagnose a broken behavior | [Failure Cookbook](failure-cookbook.md) |
| Understand why the repo is shaped this way | [Architecture Decision Records](adr/README.md) |

## Fast Orientation

- The hard ownership map is [Source of Truth Contract](source-of-truth.md):
  edit authored sources, never generated outputs.
- Product metadata starts in `devices/manifest.json`.
- Card behavior starts in `common/config/card_contract.json`.
- Shared Home Assistant entity names start in `common/config/entity_names.json`.
- Web setup code lives under `src/webserver/`.
- Firmware UI code lives mostly in `components/espcontrol/*.h`.
- Device entry points live under `devices/<device-slug>/`.
- Generated public web bundles are written to `docs/public/webserver/<slug>/www.js`.
- Generated public docs are written under `docs/generated/`.

## Repository Layout

| Path | What lives here |
|---|---|
| `common/` | Shared ESPHome YAML, theme, screens, addons, config, assets, icon lists, and glyph sets. |
| `common/config/card_contract.json` | Source of truth for card metadata, options, defaults, generated web constants, and generated firmware constants. |
| `components/espcontrol/*.h` | Header-only C++ for the on-device LVGL UI, grid, card faces, modals, parser, and Home Assistant bindings. |
| `src/webserver/` | Web configurator source. `types/<card>.js` holds card-specific panels; `modules/` holds shared setup-page logic. |
| `devices/<slug>/` | Per-device ESPHome entry points, package manifests, fonts, display drivers, pins, and local development config. |
| `docs/public/webserver/<slug>/www.js` | Generated configurator bundles served to devices at runtime. |
| `scripts/` | Build scripts, generators, validators, smoke checks, and release helpers. |

The firmware and web configurator share card facts through generated files, so a
card's labels, domains, options, defaults, and subpage behavior should normally
start in the contract and flow outward from there.

## Reference Map

- [Source of Truth Contract](source-of-truth.md) - which files are authored,
  which files are generated, how to rebuild them, and what check proves they are
  current.
- [Working Tree Rules](working-tree-rules.md) - how to handle dirty worktrees,
  unrelated local changes, staging, commits, and pushes.
- [Task Router](task-router.md) - decision trees for common broad requests.
- [Architecture](architecture.md) - how firmware, the web setup page, generated
  files, and device profiles fit together.
- [Compatibility Contract](compatibility-contract.md) - what must keep working
  across upgrades and releases.
- [Failure Cookbook](failure-cookbook.md) - first files and checks for common
  breakages.
- [Change Workflows](change-workflows.md) - common edits and the files/checks
  they usually require.
- [Task Playbooks](playbooks/README.md) - short, checklist-driven recipes for
  common changes with edit-first paths, generated-file expectations, and stop
  rules.
- [Check Matrix](check-matrix.md) - generated path-to-check routing table.
- [Architecture Decision Records](adr/README.md) - accepted structural decisions
  that should not be undone casually.
- [Card Contract](card-contract.md) - how card metadata moves from JSON into the
  web UI and firmware.
- [Card Type Map](card-type-map.md) - per-card starting points for web files,
  firmware headers, option storage, modals, Home Assistant subscriptions, and
  checks.
- [Web Configurator](web-configurator.md) - structure of the browser setup page
  served by the device.
- [Firmware](firmware.md) - the on-device LVGL grid, card runtime, modals, fonts,
  and parser notes.
- [Font Guidelines](font-guidelines.md) - how to reuse existing firmware font
  roles and avoid unnecessary new font sizes.
- [Devices and Builds](devices-and-builds.md) - device profiles, generated
  package files, firmware bundles, and release-facing outputs.
- [Checks and Releases](checks-and-releases.md) - local verification commands and
  release-sensitive files.

## Keep This Folder Internal

The public docs site is built with:

```bash
npm run docs:build
```

That script runs `vitepress build docs`, so only Markdown under `docs/` is treated
as site content. Keep this folder at the repository root unless there is an
intentional decision to publish it.

## Editing Rules

- Prefer source files over generated files. If a file says it is generated, find
  the generator and update its input.
- Generated tables in this folder are updated with
  `python3 scripts/check_dev_docs.py --update`; verify them with
  `npm run check:dev-docs`.
- After changing contract, device, entity, icon, or model inputs, run the
  relevant generator and commit the regenerated output.
- Keep developer notes factual and repo-specific. Do not duplicate user-facing
  instructions from the public docs unless the developer context changes the
  action.
