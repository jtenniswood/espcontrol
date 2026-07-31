# Product Source Map

This directory documents EspControl's product sources while the repo keeps the
existing file locations for compatibility with generators, docs, and workflows.

The hard internal edit/rebuild/check contract lives in
[`dev-docs/source-of-truth.md`](../dev-docs/source-of-truth.md). Use that page
when deciding what to edit and what must be regenerated.

## Authored Product Sources

Edit these files when changing product behavior or supported hardware:

- `devices/manifest.json` - supported panels, layout facts, web preview sizing,
  firmware fonts, firmware package substitutions, and public screen metadata.
- `common/config/card_contract.json` - card types, saved config fields, defaults,
  picker metadata, card options, migration aliases, and compact subpage codes.
- `common/config/entity_names.json` - Home Assistant entity names shared by
  firmware YAML and the web setup page.
- `common/assets/icons.json` - icon names, Material Design Icon codepoints, and
  domain defaults.
- `compatibility/fixtures/product_compatibility.json` - saved config, backup,
  layout, and migration fixtures that protect upgrades.

## Generated Outputs

Do not hand-edit generated sections or files. Rebuild them with
`python3 scripts/build.py`, `python3 scripts/generate_device_slots.py`, or
`python3 scripts/check_product_snapshot.py --update`.

- `common/config/entity_names.yaml`
- `src/webserver/generated/entity_catalog.ts`
- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `docs/generated/cards/capabilities.md`
- `docs/generated/screens/*.md`
- `docs/public/device-profiles.json`
- `docs/public/webserver/*/www.js`
- generated blocks inside `devices/*/packages.yaml`
- generated blocks inside `devices/*/device/sensors.yaml`
- `product/product_snapshot.json`

## Checks

Run `npm run check:product` after changing authored product sources. Run
`npm run check:product-snapshot` when the combined product snapshot changes. Run
`npm run check:fast` before committing broader changes.
