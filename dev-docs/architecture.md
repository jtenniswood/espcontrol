# Architecture

EspControl has three main surfaces:

1. Firmware that runs on the ESP32 display.
2. A browser setup page served by the device.
3. Public install and reference docs built with VitePress.

The firmware and setup page share product facts through generated files. Most
changes should start from the source JSON/YAML files, then rebuild generated
outputs. For the hard edit/rebuild/check contract, use
[`source-of-truth.md`](source-of-truth.md).

## Main Source Areas

| Area | Path | Purpose |
|---|---|---|
| Product profiles | `devices/catalog.json` | Supported displays, reusable profiles, slot counts, layout, firmware substitutions, font roles, and public device facts. |
| Card metadata | `common/config/card_contract.json` | Card type names, defaults, allowed domains, options, aliases, and subpage codes. |
| Entity names | `common/config/entity_names.json` | Shared Home Assistant entity names used by firmware and the setup page. |
| Icons | `common/assets/icons.json` and `common/assets/*glyphs.yaml` | Icon names, glyphs, and font glyph sets. |
| Firmware UI | `components/espcontrol/*.h` | LVGL card grid, card renderers, modals, config parsing, Home Assistant bindings. |
| Web setup page | `src/webserver/` | Browser UI for configuring cards, settings, backup/restore, and previews. |
| Typed web state | `src/webserver/state/` | Device configuration and application state types, isolated state creation, event aliases, and event parsing. |
| Device config | `devices/<slug>/` | ESPHome entry points and per-device display/font/pin config. |
| Build scripts | `scripts/` | Generators, validators, smoke checks, and release helpers. |

## Generated Outputs

Do not hand-edit these unless the generator has been intentionally retired. The
full source-to-output ownership table lives in
[`source-of-truth.md`](source-of-truth.md).

- `common/config/entity_names.yaml`
- `src/webserver/modules/entity_catalog.js`
- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `components/espcontrol/i18n_generated.h`
- `docs/generated/cards/capabilities.md`
- `docs/generated/screens/*.md`
- `docs/public/device-profiles.json`
- `docs/public/webserver/*/www.js`
- generated blocks inside `devices/*/packages.yaml`
- generated blocks inside `devices/*/device/sensors.yaml`

The central generator is:

```bash
python3 scripts/build.py
```

Use `python3 scripts/build.py --check` to confirm generated files are current.

## Runtime Flow

1. The device boots ESPHome firmware from `devices/<slug>/dev.yaml` or
   `devices/<slug>/esphome.yaml`.
2. Firmware builds the LVGL display from shared YAML in `common/device/`,
   theme/config YAML in `common/config/`, and C++ components in
   `components/espcontrol/`.
3. The device exposes a web server.
4. The browser setup page loads a per-device `www.js` bundle. New build
   entrypoints bundle it into firmware; older installed firmware can still fetch
   `docs/public/webserver/<slug>/www.js` from GitHub Pages.
5. The setup page reads and writes ESPHome entities exposed by the device, such
   as `Button N Config` text entities.
6. Firmware parses the saved compact config string and updates the on-device
   cards.

## Build-Time Flow

```text
common/config/card_contract.json
  -> src/webserver/generated/card_contract.ts
  -> components/espcontrol/button_grid_contract_generated.h
  -> docs/generated/cards/capabilities.md

common/config/entity_names.json
  -> common/config/entity_names.yaml
  -> src/webserver/modules/entity_catalog.js

devices/manifest.json
  -> docs/public/device-profiles.json
  -> docs/generated/screens/*.md
  -> generated package and sensor blocks

src/webserver/**
  -> docs/public/webserver/<slug>/www.js
```

## Public Docs Boundary

The public docs site is built from `docs/` with `vitepress build docs`.

This `dev-docs/` folder is intentionally outside that tree. It can be linked from
root-level contributor material if needed, but should not be added to the
VitePress sidebar unless the publication decision changes.
