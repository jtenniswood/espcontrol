# Card Contract

`common/config/card_contract.json` is the source of truth for card type metadata.
It keeps the web setup page and firmware aligned.

`contractVersion` versions the authored contract language. It is deliberately
separate from backup envelope versions and the legacy/compact saved-string
encoding. Changing the contract version does not make users resave cards.

## What the Contract Defines

Each card entry can define:

- `label` - display label used by the setup page.
- `allowInSubpage` - whether the card can be used inside a subpage.
- `domains` - allowed Home Assistant entity domains.
- `options` - typed settings stored in the compact `options` field.
- `default` - default saved config for a new card.
- aliases or picker metadata where supported by the schema.

Pilot card definitions also contain a `normalization` section. It records a
policy for every saved field, the canonical stored-option order, the current
unknown-option policy, named migration actions, and any reviewed custom hook.
Hooks must be listed in `normalizationHooks`; arbitrary executable expressions
are not accepted in JSON.

Option `applicability` conditions use only:

- equality (`equals`)
- membership (`in`)
- presence (`present`)
- optional negation (`negate`)

The validator rejects missing field policies, unknown hooks, duplicate storage
names, missing choice/number defaults, aliases to missing cards, invalid
conditions, duplicate compact codes, and reuse of a code listed in
`retiredSubpageTypeCodes`.

Generated consumers include:

- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `docs/generated/cards/capabilities.md`

## Saved Button Config

The setup page stores button configuration in ESPHome text entities, usually
named `Button N Config`. Firmware reads those strings and parses them into
`ParsedCfg` in `components/espcontrol/button_grid_config.h`.

The web-side equivalent lives in `src/webserver/application/config_codec.ts`.

When saved config changes, update both sides and keep old config readable when
possible.

## Option Persistence

Several places intentionally clear unknown `options` to prevent stale settings
from leaking across card types. If a card uses `options`, make sure it is
preserved in all relevant places:

- `src/webserver/application/config_codec.ts`
  - normalization while editing
  - serialization before writing back to the device
- `components/espcontrol/button_grid_config.h`
  - firmware parsing after the compact string is read

This means there are three wipe points:

1. `normalizeButtonConfig` in `src/webserver/application/config_codec.ts`.
2. `buttonConfigFields` in `src/webserver/application/config_codec.ts`.
3. `parse_cfg` in `components/espcontrol/button_grid_config.h`.

If an option appears to save in the setup page but disappears after reload, or
applies in the editor but shows defaults on the device, one of these exclusions
is usually missing.

## Contract Change Checklist

After editing `common/config/card_contract.json`:

```bash
python3 scripts/build.py
npm run check:card-contract-outputs
npm run check:model-contract
npm run check:backup-contract
npm run check:product
```

Expected generated files commonly include:

- `src/webserver/generated/card_contract.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `docs/generated/cards/capabilities.md`
- `docs/public/webserver/*/www.js`

## Compatibility Notes

Treat saved card config as durable user data.

- Do not rename card types without an alias or migration path.
- Do not remove an option parser before existing values have a fallback.
- Keep backup import/export working for older backups.
- Add fixtures in `compatibility/fixtures/product_compatibility.json` when the
  saved shape changes.

Baseline migration decision: leading and trailing whitespace in saved Media playlist text values is not meaningful. Browser and firmware normalization trim it, and a padded `playlist` content type is treated as the default and omitted. Existing stored strings are still read without a new format and are not rewritten until an existing save or backup-import action persists the normalized value.

## Where Card Logic Lives

| Concern | Typical path |
|---|---|
| Type metadata and defaults | `common/config/card_contract.json` |
| Web settings and preview | `src/webserver/cards/<type>.ts` |
| Web parsing/serialization | `src/webserver/application/config_codec.ts` |
| Firmware parsing | `components/espcontrol/button_grid_config.h` |
| Firmware rendering/runtime | `components/espcontrol/button_grid_<type>.h` |
| Grid setup/runtime wiring | `components/espcontrol/button_grid_grid.h` |
| Shared generated constants | `button_grid_contract_generated.h` and `src/webserver/generated/card_contract.ts` |

## Adding or Fixing a Card Type

A card type usually spans the contract, setup page, and firmware. Work in this
order:

1. Register the card in `common/config/card_contract.json`.
2. Add web settings and preview behavior in `src/webserver/cards/<type>.ts`.
3. If it stores options, update web parsing and option preservation in
   `src/webserver/application/config_codec.ts`.
4. Add firmware rendering/runtime behavior in
   `components/espcontrol/button_grid_<type>.h`.
5. Include the new header from `components/espcontrol/button_grid.h`.
6. Wire visual setup and runtime/subscription behavior in
   `components/espcontrol/button_grid_grid.h`. There are usually main-grid and
   subpage call sites.
7. If firmware parsing must understand new fields or options, update
   `components/espcontrol/button_grid_config.h`.
8. If the card opens a full-screen modal, add a `ControlModalKind` value and use
   the shared `control_modal_open_shell(...)` helper.
9. Rebuild, run checks, flash a `dev.yaml` build, and verify the setting survives
   a setup-page reload.

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

Also add `"hello"` to all option-preservation exclusions listed above.

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

export function registerHelloCardTypes(): void {
registerButtonType("hello", {
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

## Saved-configuration shadow rollout

Saved-configuration normalization moves to generated helpers one card family at a time. The generated shadow catalogue currently covers Action, Sensor, Media, and Vacuum policies. Vacuum, Sensor, and Action now have complete comparisons: tests feed the same raw saved values to the existing browser normalizer, the generated browser helper, and compiled generated C++ code, then require identical complete configurations.

Shadow helpers are deliberately not included by production firmware yet, so this stage adds 0 bytes of device flash and 0 bytes of RAM. The shadow check fails if the generated C++ header is included from another firmware header; this preserves the 8 KiB flash guard until a later PR deliberately switches a production family.
