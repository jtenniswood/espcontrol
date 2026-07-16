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

Migrated card definitions also contain a `normalization` section. It records a
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

The top-level `runtime` section gives every contract card a generated behaviour
identity without putting executable behaviour in JSON. Each entry selects a
permitted handwritten `driver` and declares all capability flags:

- `informationOnly`
- `subscriptions`
- `actions`
- `numericControl`
- `modal`
- `runtimeAllocation`
- `subpage`

Mode-aware cards also declare `modeField`, `defaultDriver`, and an exhaustive
`modes` mapping. Cover and Media currently use this to resolve their canonical
`sensor` mode to a more specific driver. The validator requires one runtime
entry per card, exact capability coverage, permitted and used driver names,
subpage agreement, and a mapping for every declared mode value.

Generated consumers include:

- `src/webserver/generated/card_contract.ts`
- `src/webserver/generated/saved_config_vacuum.ts`
- `src/webserver/generated/saved_config_sensor.ts`
- `src/webserver/generated/saved_config_action.ts`
- `src/webserver/generated/saved_config_media.ts`
- `src/webserver/generated/saved_config_static.ts`
- `src/webserver/generated/saved_config_fan.ts`
- `src/webserver/generated/saved_config_date_time.ts`
- `src/webserver/generated/saved_config_mower.ts`
- `src/webserver/generated/saved_config_occupancy.ts`
- `src/webserver/generated/saved_config_access.ts`
- `components/espcontrol/button_grid_contract_generated.h`
- `components/espcontrol/button_grid_saved_config_vacuum_generated.h`
- `components/espcontrol/button_grid_saved_config_sensor_generated.h`
- `components/espcontrol/button_grid_saved_config_action_generated.h`
- `components/espcontrol/button_grid_saved_config_media_generated.h`
- `components/espcontrol/button_grid_saved_config_static_generated.h`
- `components/espcontrol/button_grid_saved_config_fan_generated.h`
- `components/espcontrol/button_grid_saved_config_date_time_generated.h`
- `components/espcontrol/button_grid_saved_config_mower_generated.h`
- `components/espcontrol/button_grid_saved_config_occupancy_generated.h`
- `components/espcontrol/button_grid_saved_config_access_generated.h`
- `docs/generated/cards/capabilities.md`

Firmware card types then cross the shared runtime registry in
`components/espcontrol/button_grid_card_registry.h`. The main grid and
subpages both resolve the same `Family` before choosing their surface-specific
widget and lifecycle adapter. The registry test covers every authored contract
type and checks that subpage capability still matches the contract.

The generated web `CARD_RUNTIME_SPECS` registry is attached to matching
`BUTTON_TYPES` registrations as `runtimeSpec`. Firmware receives matching
`CardTypeId`, `CardDriverId`, capability flags, and a canonical-config resolver
in `button_grid_contract_generated.h`. Door/Window and Presence cards now use
the shared handwritten `STATUS_ENTITY` lifecycle driver for main-grid and
subpage visual setup, data binding, passive interaction, layout refresh, and
cleanup. Calendar, Clock, and Timezone use the shared handwritten `DATE_TIME`
lifecycle driver for the same stages; Calendar keeps its Home Assistant date
subscription, while Clock and Timezone share one local-time update registry.
Sensor and its `local_sensor`/`text_sensor` compatibility forms use the shared
`SENSOR` driver, including numeric, text, icon, duration, and local ESPHome data
binding. Weather and forecast compatibility use the shared `WEATHER` driver
while retaining the specialised forecast request registry. All of these paths
are shared by the main grid and subpages. Switch, Light Switch, Fan Switch,
Push, Internal, Webhook, Screen Lock, Action, local-action compatibility, and
Alarm Action use the shared `BASIC_ACTION` lifecycle boundary while retaining
their existing confirmation, service, local relay, HTTP, PIN, and state-display
implementations. Slider, Light Brightness, Light Temperature, Fan Speed, Fan
Oscillate, Fan Direction, Fan Preset, canonical Option Select, and Action
option-select compatibility use the shared `NUMERIC_SELECTABLE` lifecycle
boundary while retaining their existing slider, fan-action, and selection-modal
behaviour. Cover slider modes and the combined Fan Control card remain on the
existing `Family` dispatch for their later reviewed migrations. Other families
also remain on that dispatch until their reviewed migration PR switches them
one family at a time.

The pre-driver-migration runtime baseline is authored in
`common/config/card_runtime_inventory.json`. It classifies contract and
runtime-only types, lists meaningful modes, and records the broad subscription,
action, and modal responsibilities that later driver migrations must preserve.
Run `npm run check:card-runtime-coverage` to check the generated parser fixtures,
web picker/preview fingerprints, firmware-family coverage, and
`docs/generated/cards/runtime-coverage.md` report. Regenerate them with
`node scripts/generate_card_runtime_coverage.js` only when a reviewed behaviour
change intentionally updates the baseline.

Vacuum's routine saved-field policies and legacy migration actions are
generated for both browser and firmware. Its mode-specific unit and icon
decisions remain in the reviewed `normalize_vacuum_fields` hook.

Sensor's legacy `local_sensor` and `text_sensor` type migrations and routine
orchestration are generated for browser and firmware. The generated routine
invokes the named `normalize_sensor_fields` hook before
`normalize_sensor_options`, keeping Sensor-specific decisions explicit and
reviewed in those hooks.

Action's legacy `local` and `option_select` type migrations and routine
orchestration are generated for browser and firmware. The generated routine
invokes the reviewed `normalize_action_fields` hook before
`normalize_action_options`.

Media routine orchestration is generated for browser and firmware. The
generated routine invokes the reviewed `normalize_media_fields` hook before
`normalize_media_options`.

Trigger, Internal, Screen Lock, basic Light Switch, Slider, Light Brightness,
and Light Temperature cards use the shared static-card generator. Their rules
are entirely declarative, so these families need no custom normalization hooks
in either browser or firmware.

All six Fan card types use generated routing for routine field cleanup and
option handling. The reviewed `normalize_fan_fields` hook retains only default
icon decisions, while `normalize_fan_options` keeps Fan Control tab handling in
its established option helper.

Calendar, Clock, and Timezone cards use generated routing for routine field
cleanup and option handling. Their reviewed field hook only supplies the
established default entities for Calendar and Timezone, while the named option
hook preserves the existing Large Numbers behaviour.

Lawn Mower uses generated routing for routine cleanup. Its reviewed field hook
only normalizes the mower mode and selects the established mode-specific
default icon.

Door/Window and Presence cards use generated occupancy routing for routine
cleanup. Their reviewed field hook retains subtype-aware and presence default
icons, while the named option hook preserves only the Active Color flag.

Cover, Garage, Gate, and Lock cards use generated access routing for routine
cleanup. Their reviewed field hook retains mode-sensitive unit and active-icon
decisions, while the named option hook preserves the established modal-tab and
status-label settings.

An `allowed` field policy may declare `aliases` whose targets are in its
allowed-value list. This preserves renamed legacy values before applying the
fallback; Vacuum uses it for the old service-style Start and Dock modes.

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

Malformed UTF-8 percent runs are preserved as literal saved text, matching the browser's safe decoder. Valid percent-encoded UTF-8 and delimiters continue to decode normally.

When a partly migrated Sensor translation contains both a current state output and a legacy high/low label, the current output wins; the legacy value supplies only the missing input/output parts.

Action state, script-field, and confirmation text values also ignore leading and trailing whitespace during normalization. Padded confirmation defaults are omitted so a second normalization pass is identical to the first.

## Where Card Logic Lives

| Concern | Typical path |
|---|---|
| Type metadata and defaults | `common/config/card_contract.json` |
| Web settings and preview | `src/webserver/cards/<type>.ts` |
| Web parsing/serialization | `src/webserver/application/config_codec.ts` |
| Firmware parsing | `components/espcontrol/button_grid_config.h` |
| Firmware rendering/runtime | `components/espcontrol/button_grid_<type>.h` |
| Shared firmware family registry | `components/espcontrol/button_grid_card_registry.h` |
| Grid setup/runtime wiring | `components/espcontrol/button_grid_grid.h` |
| Shared generated constants | `button_grid_contract_generated.h` and `src/webserver/generated/card_contract.ts` |

## Adding or Fixing a Card Type

A card type usually spans the contract, setup page, and firmware. Work in this
order:

1. Register the card and its runtime driver/capabilities in
   `common/config/card_contract.json`.
2. Add web settings and preview behavior in `src/webserver/cards/<type>.ts`.
3. If it stores options, update web parsing and option preservation in
   `src/webserver/application/config_codec.ts`.
4. Add firmware rendering/runtime behavior in
   `components/espcontrol/button_grid_<type>.h`.
5. Include the new header from `components/espcontrol/button_grid.h`.
6. While legacy dispatch remains active, assign the type to a `Family` in
   `button_grid_card_registry.h`. When migrating it, add a shared lifecycle
   driver that owns visual setup, data binding, interaction, layout, cleanup,
   and main/subpage click dispatch. Surface-specific ownership or navigation
   can remain in that driver's environment adapter.
7. If firmware parsing must understand new fields or options, update
   `components/espcontrol/button_grid_config.h`.
8. If the card opens a full-screen modal, add a `ControlModalKind` value and use
   the shared `control_modal_open_shell(...)` helper.
9. Add compatibility fixtures plus runtime inventory modes and lifecycle
   expectations for subscriptions, actions, modal ownership, and cleanup.
10. Rebuild, run checks, flash a `dev.yaml` build, and verify both main-grid and
    subpage use, reload persistence, reconnect behaviour, actions, and cleanup.

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

Saved-configuration normalization moves to generated helpers one card family at a time. The generated shadow catalogue currently covers Action, Sensor, Media, and Vacuum policies. All four pilot families now have complete comparisons: tests feed the same raw saved values to the existing browser normalizer, the generated browser helper, and compiled generated C++ code, then require identical complete configurations.

Shadow helpers are deliberately not included by production firmware yet, so this stage adds 0 bytes of device flash and 0 bytes of RAM. The shadow check fails if the generated C++ header is included from another firmware header; this preserves the 8 KiB flash guard until a later PR deliberately switches a production family.

Production rollout proceeds in focused card-family groups. Vacuum, Sensor,
Action, and Media now use generated production routing, with reviewed hooks
retained for their genuinely card-specific decisions. Trigger, Internal,
Screen Lock, basic Light Switch, Slider, Light Brightness, and Light Temperature
form the fully declarative static group; the remaining card families stay on
their established production paths until their focused migration steps.
The six Fan types also use generated production routing, with only their named
icon and Fan Control option hooks kept by hand.
Calendar, Clock, and Timezone now use generated production routing too, with
only their named default-entity and Large Numbers option hooks kept by hand.
Lawn Mower also uses generated production routing, with only its named mode and
default-icon hook kept by hand.
Door/Window and Presence also use generated production routing, with only their
named default-icon and Active Color option hooks kept by hand.
Cover, Garage, Gate, and Lock also use generated production routing, with only
their named mode-sensitive field and option hooks kept by hand.
Alarm and Alarm Action also use generated production routing, with only their
named action/default-icon and option hooks kept by hand.
Weather and its legacy Weather Forecast alias also use generated production
routing, with only their named supported-mode and Large Numbers hooks kept by hand.
Image also uses generated production routing, with only its named label/icon
visibility and modal-option hooks kept by hand.
Climate and Climate Control also use generated production routing, with only
their named icon/precision and climate-option hooks kept by hand; the legacy
`climate` saved type remains compatible and normalizes to `climate_control`.
Light Control also uses generated production routing, with only its named
visible-tab option hook kept by hand.
Webhook also uses generated production routing, with only its named HTTP-method,
request-body, empty-icon, and Headers option hooks kept by hand.
Subpage also uses generated production routing, with only its named preset-field
and supported state-display option hooks kept by hand.
Basic Switch also uses generated production routing, with only its named
confirmation, active-pattern, and Large Numbers option hook kept by hand.
