# Product Source Map

This directory is the Product Model ownership boundary. The initial reset stage
mapped established source locations into one validated model. The current
generated-source stage owns every supported card-contract entry and device
profile in [`v2/`](v2/) while preserving the existing generated output.
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
- `product/v2/companion_capabilities.json` - the Companion protocol, security
  policy, card modes, Mac actions, media controls, and system metrics.
- `product/v2/product_compatibility.json` - saved config, backup,
  layout, and migration fixtures that protect upgrades.

`model_v2.json` records the complete card-contract and device-catalogue sources
in `product/v2/`. The per-card and per-device sources below are required to
cover every supported entry and prove the composed model has not changed the
generated output.

## Product Model v2 Sources

- `v2/cards/sensor.json`, `media.json`, and `image.json` are the original
  sensor, media, and image pilots.
- `v2/cards/light_*.json` is the complete light-card family: switch,
  brightness, full control, and temperature.
- `v2/cards/fan_*.json` is the complete fan-card family: switch, speed,
  direction, oscillation, preset, and full control.
- `v2/cards/clock.json`, `calendar.json`, and `timezone.json` are the
  date-and-time family.
- `v2/cards/weather.json` and `weather_forecast.json` are the weather family.
- `v2/cards/internal.json`, `local_sensor.json`, and `screen_lock.json` are
  the system-card family.
- `v2/cards/cover.json`, `door_window.json`, `garage.json`, `gate.json`, and
  `lock.json` are the access-card family.
- `v2/cards/action.json`, `option_select.json`, `push.json`, `slider.json`,
  and `subpage.json` are the interaction-card family.
- `v2/cards/climate.json` and `climate_control.json` are the climate-card
  family.
- `v2/cards/alarm.json`, `alarm_action.json`, and `presence.json` are the
  alert-card family.
- `v2/cards/lawn_mower.json`, `vacuum.json`, and `webhook.json` are the
  appliance-and-integration-card family.
- `v2/cards/default_switch.json` owns the contract's unnamed default-switch
  fallback entry.
- `v2/devices/guition-esp32-p4-jc8012p4a1.json` and
  `guition-esp32-p4-jc8012p4a1-v2.json` are the 10-inch V1 and V2 device
  entries; shared hardware profiles remain in `product/v2/device_catalog.json`.
- `v2/devices/guition-esp32-p4-jc1060p470.json` and
  `guition-esp32-p4-jc1060p470-v2.json` are the 7-inch original-panel and
  new-panel device entries; shared hardware profiles remain in
  `product/v2/device_catalog.json`.
- `v2/devices/guition-esp32-p4-jc4880p443.json` is the authoritative 4.3-inch
  P4 device entry.
- `v2/devices/esp32-p4-86.json` is the authoritative square P4-86 device
  entry, including its local-voice and relay capabilities.
- `v2/devices/guition-esp32-s3-4848s040.json` is the authoritative compact
  S3 device entry.

`python3 scripts/check_product_model_v2.py` proves that the composed model is
byte-for-byte equivalent to the generated card contract and device catalogue,
and rejects an incomplete card or device source set.

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
- `src/webserver/generated/companion_capabilities.ts`
- `components/espcontrol/companion_capabilities_generated.h`
- `macos/Companion/Sources/Companion/CompanionCapabilities.generated.swift`
- `product/generated/companion_manifest.json`

## Checks

Run `npm run check:product` after changing authored product sources. Run
`npm run check:product-model-v2` when changing the ownership adapter, and
`npm run check:product-snapshot` when the combined product snapshot changes.
Run `npm run check:fast` before committing broader changes.
Run `npm run check:companion-contract` after any Companion protocol, pairing,
capability, or generated-contract change.
