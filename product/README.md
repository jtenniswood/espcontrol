# Product Source Map

This directory is the Product Model ownership boundary. The initial reset stage
mapped established source locations into one validated model. The current
generated-pilot stage moves the Sensor card and Guition JC8012P4A1 (10-inch V1)
device entry into [`v2/`](v2/) while preserving the existing generated output.
Generators and validators resolve their card and device inputs through this
model, so later migrations change one controlled entry point.

The hard internal edit/rebuild/check contract lives in
[`dev-docs/source-of-truth.md`](../dev-docs/source-of-truth.md). Use that page
when deciding what to edit and what must be regenerated.

## Authored Product Sources

Edit these files when changing product behavior or supported hardware:

- `product/v2/device_catalog.json` - supported panels, layout facts, web preview sizing,
  firmware fonts, firmware package substitutions, and public screen metadata.
- `product/v2/card_contract.json` - card types, saved config fields, defaults,
  picker metadata, card options, migration aliases, and compact subpage codes.
- `product/v2/entity_names.json` - Home Assistant entity names shared by
  firmware YAML and the web setup page.
- `product/v2/icons.json` - icon names, Material Design Icon codepoints, and
  domain defaults.
- `product/v2/product_compatibility.json` - saved config, backup,
  layout, and migration fixtures that protect upgrades.

`model_v2.json` records the complete card-contract and device-catalogue sources
in `product/v2/`. The selected pilot overlays below prove the per-card and
per-device composition path without changing the generated output.

## Product Model v2 Pilot

- `v2/cards/sensor.json` is the authoritative Sensor card definition.
- `v2/devices/guition-esp32-p4-jc8012p4a1.json` is the authoritative 10-inch
  V1 device entry; shared hardware profiles remain in `product/v2/device_catalog.json`.

`python3 scripts/check_product_model_v2.py` proves that the composed model is
byte-for-byte equivalent to the legacy card contract and device catalogue.
Until another family receives a per-item overlay, edit its shared definition in
`product/v2/card_contract.json` or `product/v2/device_catalog.json`.

## Generated Outputs

Do not hand-edit generated sections or files. Rebuild them with
`python3 scripts/build.py`, `python3 scripts/generate_device_slots.py`, or
`python3 scripts/check_product_snapshot.py --update`.

- `common/config/entity_names.yaml`
- `devices/manifest.json`
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
`npm run check:product-model-v2` when changing the ownership adapter, and
`npm run check:product-snapshot` when the combined product snapshot changes.
Run `npm run check:fast` before committing broader changes.
