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
| Product profiles | `product/v2/device_catalog.json` | Supported displays, reusable profiles, slot counts, layout, firmware substitutions, font roles, and public device facts. |
| Card metadata | `product/v2/card_contract.json` | Card type names, defaults, allowed domains, options, aliases, and subpage codes. |
| Entity names | `product/v2/entity_names.json` | Shared Home Assistant entity names used by firmware and the setup page. |
| Icons | `product/v2/icons.json` and `common/assets/*glyphs.yaml` | Icon names, glyphs, and font glyph sets. |
| Firmware UI | `components/espcontrol/*.h` | LVGL card grid, card renderers, modals, config parsing, Home Assistant bindings. |
| Configuration service | `components/espcontrol/configuration_service.*` and `configuration_store.*` | One-time legacy import, compatibility dual-write, atomic two-slot storage, and generation-matched saves for browser conflict protection. It is the live production persistence path. |
| Native panel document | `components/espcontrol/panel_config_document.h`, `panel_config_service_validator.h`, and `src/webserver/model/panel_config.ts` | Bounded, versioned `PanelConfig` binary codec shared by firmware and the browser. Its validator protects atomic service saves and loads; V1 carries current compact card strings unchanged while live storage and API migration run through the core-owned configuration service. |
| Native configuration discovery | `components/espcontrol/panel_config_capabilities*.h` | `GET /api/v1/capabilities` advertises the native document, read/write API, and web-asset versions so the browser can select the compatible configuration path. |
| Web setup page | `src/webserver/` | Browser UI for configuring cards, settings, backup/restore, and previews. |
| Typed web state | `src/webserver/state/` | Device configuration and application state types, isolated state creation, direct module-owned state access, event aliases, and event parsing. Application state must not be published as a browser global. |
| Typed device API | `src/webserver/api/` | Injectable HTTP transport and ordered request queue; UI modules retain user-facing reactions. |
| Device config | `devices/<slug>/` | ESPHome entry points and per-device display/font/pin config. |
| Build scripts | `scripts/` | Generators, validators, smoke checks, and release helpers. |

## Generated Outputs

Do not hand-edit these unless the generator has been intentionally retired. The
full source-to-output ownership table lives in
[`source-of-truth.md`](source-of-truth.md).

- `common/config/entity_names.yaml`
- `src/webserver/generated/entity_catalog.ts`
- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `components/espcontrol/i18n_generated.h`
- `docs/generated/cards/capabilities.md`
- `docs/generated/screens/*.md`
- `docs/public/device-profiles.json`
- `docs/public/webserver/www.js`
- `docs/public/webserver/web-assets.json`
- `docs/public/webserver/embedded/www.js`
- `docs/public/webserver/bundles/*/www.js`
- `docs/public/webserver/*/www.js`
- generated blocks inside `devices/*/packages.yaml`
- generated blocks inside `devices/*/device/sensors.yaml`

The central generator is:

```bash
python3 scripts/build.py
```

Use `python3 scripts/build.py --check` to confirm generated files are current.

Generated source files remain tracked because firmware and web compilation
consume them. Publishable binaries are different: release jobs assemble those
only under the ignored `dist/` directory. Nothing under `dist/` is an authored
source or a committed generated input.

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

`EspControlAppCore` owns the long-lived configuration, card runtime, Home
Assistant binding and callback, display lifecycle, grid navigation, and
modal-state services. The navigation and modal-state slots have fixed capacity so their
LVGL-specific types remain in the UI layer with a bounded, reviewable memory
budget. Firmware UI accesses those services only after the core starts; the
standalone host-test fallbacks are excluded from firmware images.
`EspControlApp` starts the core before WiFi so the 250-priority boot automations
also use the core-owned Home Assistant binding.

## Build-Time Flow

```text
product/v2/card_contract.json
  -> src/webserver/generated/card_contract.ts
  -> components/espcontrol/button_grid_contract_generated.h
  -> docs/generated/cards/capabilities.md

product/v2/entity_names.json
  -> common/config/entity_names.yaml
  -> src/webserver/generated/entity_catalog.ts

product/v2/device_catalog.json
  -> devices/manifest.json

devices/manifest.json
  -> docs/public/device-profiles.json
  -> docs/generated/screens/*.md
  -> generated package and sensor blocks

src/webserver/**
  -> docs/public/webserver/www.js
  -> docs/public/webserver/web-assets.json
  -> docs/public/webserver/embedded/www.js
  -> docs/public/webserver/bundles/<sha256>/www.js
  -> docs/public/webserver/<slug>/www.js
```

## Public Docs Boundary

The public docs site is built from `docs/` with `vitepress build docs`.

This `dev-docs/` folder is intentionally outside that tree. It can be linked from
root-level contributor material if needed, but should not be added to the
VitePress sidebar unless the publication decision changes.
