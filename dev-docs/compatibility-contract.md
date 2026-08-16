# Compatibility Contract

Treat these as product promises. Changes can still happen, but they need an
explicit migration path, compatibility fixture, release note, and rollback plan.

## Must Keep Working

| Surface | Promise | Protected by |
|---|---|---|
| Saved card strings | Existing `Button N Config` and subpage config strings keep parsing after firmware or web updates. | `npm run check:firmware-parser`, `npm run check:web-smoke`, `npm run check:backup-contract` |
| Backup import/export | Backups created by older supported releases import without users recreating layouts by hand. | `product/v2/product_compatibility.json`, `npm run check:backup-contract` |
| Card type names and aliases | Saved card `type` values keep their meaning, including hidden and legacy types. | `product/v2/card_contract.json`, `npm run check:card-contract-outputs` |
| Option values | Existing option keys and values either keep working or have a fallback parser. | `src/webserver/application/config_codec.ts`, `components/espcontrol/button_grid_config.h` |
| Device slugs | Published device slugs remain valid for firmware bundles, web bundles, docs, manifests, and release assets. | `devices/manifest.json`, `npm run check:device-profiles`, `npm run check:firmware-release` |
| Public firmware URLs | Installed panels can still find update manifests and OTA assets at the expected public paths. | `npm run check:firmware-release`, `npm run check:public-firmware-script` |
| Public web bundles | Older firmware can still load `docs/public/webserver/<slug>/www.js` for its slug; new build entrypoints bundle the same generated file locally. | `python3 scripts/build.py www`, `npm run check:web-smoke` |
| Generated device YAML | Generated package and sensor blocks stay reproducible from manifest data. | `python3 scripts/generate_device_slots.py --check` |
| Home Assistant actions | Existing card controls keep calling the same Home Assistant services unless a migration is deliberate. | `npm run check:firmware-ha-bindings`, `npm run check:firmware-card-runtime` |

## Compatibility Rules

- Do not rename a saved card type without a migration alias.
- Do not reuse an old compact type code for a different meaning.
- Do not remove option parsing while existing configs can still contain that
  option.
- Do not make backup export produce a shape that backup import cannot read.
- Do not change a device slug, firmware manifest slug, release asset name, or
  public web bundle path without a compatibility plan.
- Do not require users to resave cards after updating unless the release notes
  clearly call out the migration.

### PanelConfig release lifecycle

`product/release_contract.json` declares the PanelConfig compatibility phase,
and `configuration_release_policy.h` is the checked firmware projection of that
choice. Keep `dual-write` for two stable releases. The release preflight reads
the immutable `release-manifest.json` assets from published stable releases and
rejects a move to `read-import-only` until two consecutive releases with the
same PanelConfig document version prove that window. For the following stable
release, change both declarations to `read-import-only`: new native saves stop
mirroring legacy text entities, while an older panel's text configuration can
still be imported once. Retire legacy reads only after that read/import-only
release has shipped and its upgrade coverage has been confirmed.

Live grid application uses a separate native runtime adapter. The legacy
adapter is limited to one-time import and release-controlled downgrade
mirroring, so disabling legacy writes cannot disable live card updates.

## Required Migration Shape

Every compatibility-affecting change needs:

1. Old input example: compact card string, backup fixture, device slug, URL, or
   generated output being preserved.
2. New parser behavior: where the old shape is accepted and normalized.
3. Fixture or check: the command that proves the old shape still works.
4. Generated output review: which generated files are expected to change.
5. User-facing note: only when behavior or upgrade expectations change.

## Stop Conditions

Stop and redesign the change if:

- A fixture fails and the only fix is to tell users to recreate cards.
- Web import accepts a backup that firmware cannot render.
- Firmware renders a card differently from the web preview for the same saved
  config.
- A published device slug or release asset disappears from generated metadata.
- A generated output changed and no authored source explains why.
