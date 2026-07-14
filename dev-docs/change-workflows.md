# Change Workflows

Use these workflows as a starting point. The exact files can vary, but these are
the usual paths touched by each kind of change. For checklist-driven task
recipes, use [Task Playbooks](playbooks/README.md).

## Add or Change a Card Type

Playbook: [Add or change a card type](playbooks/add-card-type.md).

Start with the contract, then wire both UI surfaces.

1. Edit `common/config/card_contract.json`.
2. If the card has web settings or a preview, add or update
   `src/webserver/cards/<type>.ts`.
3. If the card stores options, update option parsing/preservation in
   `src/webserver/application/config_codec.ts`.
4. Add or update firmware behavior in `components/espcontrol/button_grid_<type>.h`.
5. Include the card header from `components/espcontrol/button_grid.h`.
6. Wire setup and runtime behavior in `components/espcontrol/button_grid_grid.h`.
7. If firmware parsing must understand new fields or options, update
   `components/espcontrol/button_grid_config.h`.
8. Run `python3 scripts/build.py`.
9. Run at least `npm run check:product`. For broader card behavior, run
   `npm run check:fast`.

Watch for the option wipe points documented in [Card Contract](card-contract.md).

## Change Saved Config Format

Playbook: [Change saved config](playbooks/change-saved-config.md).

Saved config is upgrade-sensitive because existing devices and backups may
already contain older strings.

1. Update parsing and serialization together.
2. Keep older values readable unless there is an intentional migration.
3. Add or update compatibility fixtures in
   `compatibility/fixtures/product_compatibility.json`.
4. Run:

```bash
npm run check:backup-contract
npm run check:model-contract
npm run check:config
npm run check:product
```

## Add a Home Assistant Entity Name

1. Edit `common/config/entity_names.json`.
2. Run `python3 scripts/build.py`.
3. Confirm these generated files changed as expected:
   - `common/config/entity_names.yaml`
   - `src/webserver/generated/entity_catalog.ts`
4. Run:

```bash
npm run check:config
npm run check:product
```

## Add or Change a Supported Device

Playbook: [Add or change a supported device](playbooks/add-supported-device.md).

1. Add or update the device entry in `devices/manifest.json`.
2. Add or update `devices/<slug>/packages.yaml`, `dev.yaml`, `esphome.yaml`, and
   files under `devices/<slug>/device/`.
3. Confirm the device has the required font roles in `firmware.fonts`.
4. Run:

```bash
python3 scripts/build.py
python3 scripts/generate_device_slots.py
npm run check:device-matrix
npm run check:device-profiles
npm run check:product
```

If the new device has different layout behavior, also review web preview sizing
in the manifest `web` section.

## Change Fonts or Icons

Playbook: [Change fonts or icons](playbooks/change-fonts-or-icons.md).

Fonts and icons are memory-sensitive on embedded displays.

1. Add icons to `common/assets/icons.json` when the name should be available in
   the setup page.
2. Add needed glyphs to the relevant `common/assets/*glyphs.yaml` file.
3. Add or adjust per-device font definitions in
   `devices/<slug>/device/fonts.yaml`.
4. If exposing a new firmware font role, update:
   - `devices/manifest.json`
   - `scripts/device_profiles.py`
   - `scripts/generate_device_slots.py`
   - the consuming C++ config structures
5. Run:

```bash
python3 scripts/build.py icons
python3 scripts/generate_device_slots.py
npm run check:product
```

## Change the Web Setup Page

1. Edit `src/webserver/application/` for shared behavior or `src/webserver/cards/`
   for card-specific UI.
2. Import and call a new shared module installer in the deliberate order in
   `src/webserver/entry.ts`.
3. Run:

```bash
python3 scripts/build.py www
npm run check:web-smoke
npm run check:web-browser-smoke
```

Commit the rebuilt `docs/public/webserver/*/www.js` bundles when they change.
The Python command prepares device configuration and delegates bundling to the
single esbuild API pipeline in `scripts/build_web_bundle.js`; tests use its
temporary-output mode to avoid relying on tracked bundles.

## Change Firmware UI Behavior

1. Edit the relevant `components/espcontrol/button_grid_*.h` file.
2. If the card is new, wire it through `button_grid.h` and `button_grid_grid.h`.
3. If parsing changes, update `button_grid_config.h`.
4. Compile the relevant device with ESPHome before release.
5. Run parser and runtime checks:

```bash
npm run check:firmware-parser
npm run check:firmware-modals
npm run check:firmware-card-runtime
npm run check:firmware-display-tokens
npm run check:firmware-ha-bindings
```
