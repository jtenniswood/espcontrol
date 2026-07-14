# Task Router

Start here when the request is broad and you need the shortest safe path through
the repo. Each route gives the first files to inspect, the generator boundary,
the first check to run, and the condition that should stop the change.

## I Want to Change a Card

Use this for a new card type, a card setting, card preview behavior, on-device
rendering, Home Assistant actions, or subpage card behavior.

1. Read [Card Type Map](card-type-map.md) for the saved type and related files.
2. Edit first:
   - `common/config/card_contract.json`
   - `src/webserver/cards/<type>.ts`
   - `components/espcontrol/button_grid_<type>.h`
3. Regenerate with `python3 scripts/build.py` when the contract or web bundle
   changes.
4. Verify first with `npm run check:card-contract-outputs`,
   `npm run check:web-smoke`, and the relevant firmware check from
   [Check Matrix](check-matrix.md).
5. Stop if saved config would become unreadable, if web and firmware parse the
   same option differently, or if generated outputs change outside the expected
   list.

Use [Add or Change a Card Type](playbooks/add-card-type.md) for the full
checklist.

## I Want to Change a Saved Setting

Use this for button config strings, subpage config chunks, backup shape, import
or export behavior, model fields, aliases, or compatibility fixtures.

1. Read [Compatibility Contract](compatibility-contract.md).
2. Edit first:
   - `src/webserver/application/config_codec.ts`
   - `components/espcontrol/button_grid_config.h`
   - `compatibility/fixtures/product_compatibility.json`
3. Regenerate only if contract, model, or web generated inputs also changed.
4. Verify first with `npm run check:backup-contract`,
   `npm run check:model-contract`, and `npm run check:firmware-parser`.
5. Stop if old backups fail, if an old compact config string no longer parses,
   or if a user would need to manually recreate cards after updating.

Use [Change Saved Config](playbooks/change-saved-config.md) for the full
checklist.

## I Want to Add or Change Hardware

Use this for a new display, slot count, orientation, device profile, firmware
package, release build, font mapping, Ethernet/WiFi variant, or install docs.

1. Edit first:
   - `devices/manifest.json`
   - `devices/<slug>/packages.yaml`
   - `devices/<slug>/dev.yaml`
   - `devices/<slug>/esphome.yaml`
   - `devices/<slug>/device/*.yaml`
2. Regenerate with `python3 scripts/build.py` and
   `python3 scripts/generate_device_slots.py`.
3. Verify first with `npm run check:device-profiles` and
   `npm run check:device-matrix`.
4. Stop if a generated block was edited by hand, release-facing build names
   changed unexpectedly, or the new hardware needs a one-off exception in shared
   code that could be manifest data instead.

Use [Add or Change a Supported Device](playbooks/add-supported-device.md) for
the full checklist.

## I Want to Change Firmware UI

Use this for LVGL layout, modal behavior, card runtime state, Home Assistant
bindings, font roles, image download limits, or device-specific UI sizing.

1. Find the relevant runtime path in [Firmware](firmware.md) and
   [Card Type Map](card-type-map.md).
2. Edit the smallest card/runtime header first. Avoid changing generated device
   YAML directly.
3. If a new font role or device profile value is needed, update
   `devices/manifest.json`, then regenerate slot files.
4. Verify first with the focused firmware check from [Check Matrix](check-matrix.md).
5. Stop if the smallest supported screen has not been considered, if a modal can
   remain open across navigation unexpectedly, or if Home Assistant subscriptions
   are not cleaned up on reconfigure.

## I Want to Change Public Docs

Use this for files under `docs/`, generated public docs, screenshots, VitePress
navigation, or card type documentation.

1. Edit handwritten docs under `docs/`.
2. If generated public docs are stale, update the source in
   `devices/manifest.json` or `common/config/card_contract.json`, then regenerate.
3. Verify with `npm run docs:build` and `npm run check:dev-docs`.
4. Stop if a generated docs page was edited directly or if a public card page no
   longer maps to a saved card type.

## I Want to Change Release Behavior

Use this for firmware manifests, public firmware URLs, release confidence,
GitHub Pages assets, update scripts, or release changelog checks.

1. Read [Checks and Releases](checks-and-releases.md) and
   [Compatibility Contract](compatibility-contract.md).
2. Keep public URLs, device slugs, and existing release asset names stable unless
   the migration is explicit.
3. Verify with `npm run check:release-preflight` before publishing.
4. Stop if an existing installed panel would look for a firmware bundle, manifest
   URL, or web bundle that no longer exists.
