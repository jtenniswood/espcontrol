# Change Saved Config

Use this when changing how button configs, backups, model data, or compatibility
fixtures are parsed or serialized.

## Edit First

- `src/webserver/application/config_codec.ts`
- `components/espcontrol/button_grid_config.h`
- `compatibility/fixtures/product_compatibility.json`

Only edit card UI or runtime files after the saved shape and compatibility rule
are clear.

## Ask Before

- Making old saved configs unreadable.
- Renaming a saved card type without an alias or migration path.
- Removing backup import/export support for an existing shape.
- Changing compact config syntax in a way that requires users to resave cards.

## Checklist

- [ ] Update web parsing and serialization together.
- [ ] Update firmware parsing for the same fields.
- [ ] Keep older values readable unless there is an intentional migration.
- [ ] Preserve unknown-but-valid options where the card contract expects them.
- [ ] Add or update compatibility fixtures in
      `compatibility/fixtures/product_compatibility.json`.
- [ ] Check backup import/export behavior for the changed shape.

## Regenerate

Run this when the change also touches contract, model, web, or generated inputs:

```bash
python3 scripts/build.py
```

Do not edit generated contract or web bundle outputs directly. The TypeScript
model is authored source. The
source-to-generated mapping is in
[Source of Truth Contract](../source-of-truth.md).

Expected generated files depend on the source touched. Common examples include:

- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `docs/public/webserver/*/www.js`
- `docs/generated/cards/capabilities.md`

## Stop If

- Existing compatibility fixtures fail and no migration was agreed.
- Web and firmware parsers accept different field names or option values.
- Backup export can create a shape that backup import cannot read.
- Generated files change even though no generator input changed.

## Verify

| Level | Run | Stop when |
|---|---|---|
| Minimum | `npm run check:backup-contract`<br>`npm run check:model-contract`<br>`npm run check:config`<br>`npm run check:firmware-parser` | The change is limited to parsing, serialization, or compatibility fixtures, and older saved examples still load through both web and firmware paths. |
| Recommended | `npm run check:product` | Most saved-config changes can stop here after backup compatibility, model contract, generated outputs, web smoke, firmware runtime, and release-facing metadata checks pass. |
| Release-grade | `npm run check:fast` | Use before release, when compact config syntax changes, when multiple card types are affected, or when backup import/export behavior changes broadly. |
