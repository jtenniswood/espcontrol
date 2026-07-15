# Add or Change a Card Type

Use this when adding a new button/card type or changing how an existing card is
configured, rendered, previewed, or saved.

## Edit First

- `common/config/card_contract.json`
- `src/webserver/cards/<type>.ts`
- `components/espcontrol/button_grid_<type>.h`

Only edit these first. Add parser or wiring files after the contract, web type,
and firmware behavior show the real shape of the change.

## Ask Before

- Renaming or removing an existing card type.
- Removing an existing option or changing what an existing saved option means.
- Adding a new top-level saved-config field instead of using `options`.
- Adding a new firmware font role for the card.

## Checklist

- [ ] Add or update the card entry in `common/config/card_contract.json`.
- [ ] Add or update its `runtime.specs` entry with a permitted driver and every
      capability flag. Add an exhaustive mode mapping when a saved field selects
      different behaviour.
- [ ] Add or update the web settings and preview in
      `src/webserver/cards/<type>.ts`.
- [ ] If options are saved, preserve them in
      `src/webserver/application/config_codec.ts`.
- [ ] Add or update firmware rendering/runtime behavior in
      `components/espcontrol/button_grid_<type>.h`.
- [ ] Include the card header from `components/espcontrol/button_grid.h`.
- [ ] Wire setup and runtime behavior in
      `components/espcontrol/button_grid_grid.h`.
- [ ] If firmware parsing needs new fields or options, update
      `components/espcontrol/button_grid_config.h`.
- [ ] Add or update compatibility fixtures when the saved shape changes:
      `compatibility/fixtures/product_compatibility.json`.
- [ ] Add every meaningful mode to `common/config/card_runtime_inventory.json`,
      including expected subscriptions, actions, and modal ownership.
- [ ] Cover normalisation, picker visibility, preview, reload persistence,
      main-grid/subpage wiring, reconnect subscriptions, actions, runtime
      allocation, modal dismissal, and cleanup as applicable.

## Regenerate

```bash
python3 scripts/build.py
```

Do not edit generated files directly. The source-to-generated mapping is in
[Source of Truth Contract](../source-of-truth.md).

Expected generated files commonly include:

- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `docs/generated/cards/capabilities.md`
- `docs/public/webserver/*/www.js`

## Stop If

- Generated files outside the expected list changed.
- Existing button config strings would no longer load.
- Web settings save correctly but disappear after reload.
- The firmware parser and web codec no longer describe the same saved fields.

## Verify

| Level | Run | Stop when |
|---|---|---|
| Minimum | `npm run check:card-contract-outputs`<br>`npm run check:card-runtime-coverage`<br>`npm run check:model-contract`<br>`npm run check:backup-contract`<br>`npm run check:firmware-parser` | The change only affects the card contract, generated runtime metadata, web model, saved options, or compatibility shape, and no release-facing generated files changed unexpectedly. |
| Recommended | `npm run check:product` | Most card changes can stop here after generated card outputs, backup compatibility, web smoke, firmware card runtime, and release-facing metadata checks pass. |
| Release-grade | `npm run check:fast` plus all supported-display compiles | Use before publishing, or when the card change touches shared firmware runtime, lifecycle/registry code, broad web setup behavior, generated product surfaces, or multiple card types. Keep physical device testing separate from automated compile results. |
