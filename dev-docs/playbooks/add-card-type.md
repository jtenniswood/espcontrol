# Add or Change a Card Type

Use this when adding a new button/card type or changing how an existing card is
configured, rendered, previewed, or saved.

## Edit First

- `product/v2/card_contract.json`
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

- [ ] Add or update the card entry in `product/v2/card_contract.json`.
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
      `product/v2/product_compatibility.json`.
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
- generated files under `docs/public/webserver/`

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

## Worked Example: Hello Card

This minimal card type stores one option, `name`, and shows `Hello <name>`.

Add the contract entry:

```json
"hello": {
  "label": "Hello",
  "allowInSubpage": true,
  "domains": [],
  "options": [
    { "name": "name", "label": "Name", "kind": "text", "defaultValue": "" }
  ],
  "default": {
    "entity": "", "label": "", "icon": "Auto", "icon_on": "Auto",
    "sensor": "", "unit": "", "type": "hello", "precision": "", "options": ""
  }
}
```

Also add `hello` to `runtime.drivers` and declare its runtime spec. A static
card has no subscriptions, actions, numeric control, modal, or allocation:

```json
"hello": {
  "driver": "hello",
  "capabilities": {
    "informationOnly": true,
    "subscriptions": false,
    "actions": false,
    "numericControl": false,
    "modal": false,
    "runtimeAllocation": false,
    "subpage": true
  }
}
```

Add a matching `card_runtime_inventory.json` case and compatibility fixture so
normalisation, picker visibility, preview, lifecycle, and both runtime surfaces
are protected before implementing the card.

Regenerate the shared outputs:

```bash
python3 scripts/build.py
```

Add option helpers in `src/webserver/application/config_codec.ts`:

```js
function helloName(b) {
  return configOptionValue(b && b.options, "name");
}

function setHelloName(b, name) {
  if (!b) return "";
  b.options = setConfigOptionValue(b.options || "", "name", String(name || "").trim());
  return b.options;
}
```

Also add `"hello"` to all option-preservation exclusions listed in the
[Card Contract](../card-contract.md#option-persistence).

Create `src/webserver/cards/hello.ts` and export its registration function:

```js
var HELLO_CARD_METADATA = {
  nameField: {
    label: "Name",
    idSuffix: "name",
    placeholder: "world",
    bindName: null,
    value: function (b) { return helloName(b); },
  },
  preview: { badge: "hand-wave" },
};

export function registerHelloCardTypes(registry: CardRegistry): void {
registry.register("hello", {
  label: function () { return cardContractCardLabel("hello"); },
  allowInSubpage: function () { return cardContractAllowInSubpage("hello"); },
  pickerKey: function () { return cardContractPickerKey("hello"); },
  hidden: function () { return cardContractHidden("hello"); },
  hideLabel: true,
  defaultConfig: function () { return cardContractDefaultConfig("hello"); },
  cardMetadata: HELLO_CARD_METADATA,

  renderPreview: function (b, helpers) {
    var greeting = "Hello " + (helloName(b) || "world");
    return {
      labelHtml: cardBadgeLabelHtml(helpers, greeting, HELLO_CARD_METADATA.preview.badge),
    };
  },

  onSelect: function (b) {
    b.entity = "";
    b.label = "";
    b.sensor = "";
    b.unit = "";
    b.precision = "";
    b.icon = "Auto";
    b.icon_on = "Auto";
    b.options = "";
  },

  renderSettings: function (panel, b, helpers) {
    var field = helpers.renderCardTextField(panel, b, helpers, HELLO_CARD_METADATA.nameField);
    field.input.maxLength = 32;

    function save() {
      setHelloName(b, field.input.value);
      helpers.saveField("options", b.options);
      scheduleRender();
    }

    field.input.addEventListener("input", save);
    field.input.addEventListener("change", save);
    field.input.addEventListener("blur", save);
  },
});
}
```

Add the firmware tile in `components/espcontrol/button_grid_hello.h`:

```cpp
#pragma once
#include <string>
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "button_grid_config.h"

inline std::string hello_greeting(const ParsedCfg &p) {
  std::string name = cfg_option_value(p.options, "name");
  if (name.empty()) name = "world";
  return "Hello " + name;
}

inline void setup_hello_card(BtnSlot &s, const ParsedCfg &p) {
  if (s.icon_lbl) lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (s.sensor_container) lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (s.text_lbl) lv_label_set_text(s.text_lbl, hello_greeting(p).c_str());
}
```

Include that header from `components/espcontrol/button_grid.h`, then wire the
visual setup pass in `components/espcontrol/button_grid_grid.h`:

```cpp
if (p.type == "hello") {
  setup_hello_card(s, p);
  return;
}
```

That is enough for a static card. For live Home Assistant data, model the
runtime pass and subscriptions on an existing data-driven card such as sensor or
media.

After rebuilding and flashing, add a Hello card in the configurator, save it,
reload the setup page, and read the stored config back from the device:

```bash
curl -s "http://<device-ip>/text/Button%20N%20Config?detail=all"
```
