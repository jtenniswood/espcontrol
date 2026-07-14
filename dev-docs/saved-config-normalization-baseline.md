# Saved-Configuration Normalization Baseline

This inventory records the behaviour that must remain stable while routine card
normalization moves into the versioned contract. It describes the current
browser and compiled-firmware implementations; it is not a proposal for a new
saved format.

The shared corpus is `common/config/*_card_normalization_fixtures.json`.
`npm run check:saved-config-parity` sends every fixture through the built browser
codec and the production C++ parser, compares all nine normalized fields, and
checks browser parse/serialize/parse idempotence. The normal fast check runs the
same parity test as part of `firmware-parser`.

## Classification

- **Declarative**: allowed values, defaults, field clearing, option
  applicability/order, and default omission. These are candidates for contract
  generation.
- **Migration**: an old type, mode, label, icon, or option is translated to its
  current meaning. These remain explicit migration actions.
- **Hook**: a transformation is clearer as reviewed named code than as JSON
  conditions.
- **Platform**: framing, encoding, storage length, or runtime behaviour that is
  intentionally kept outside the generated normalizer.

## Framing and Shared Rules

| Rule | Class | Browser | Firmware |
|---|---|---|---|
| Read nine legacy semicolon fields and compact `~` comma fields | Platform | `EspControlModel.parseRawButtonConfig` | `parse_cfg` |
| Percent-decode compact delimiters and UTF-8; leave malformed escapes literal | Platform | `EspControlModel.decodeConfigField` | `decode_compact_field` |
| Write the existing safe legacy form when possible, otherwise the existing compact form | Platform | `serializeButtonConfig` | Firmware reads but does not rewrite stored card data |
| Treat missing icons as `Auto` at the model boundary | Declarative | `buttonShape`/model defaults and generated card normalization | Generated card normalization before firmware runtime use |
| Parse comma-separated options, preserve canonical order, percent-encode option values, and omit defaults | Declarative | `config_option_core.ts` and family option modules | `cfg_option_*`, `*_card_options_normalized` |
| Remove unrecognized or inapplicable options | Declarative | final preservation guard in `normalizeButtonConfig` | final option guard in `normalize_parsed_cfg` |
| Allow `large_numbers` only for supported type/mode/precision combinations | Declarative + Hook | `cardLargeNumbersSupported` | `card_large_numbers_supported` |
| Keep 255-byte storage chunks lossless when joining subpage/card strings | Platform | saved/subpage model helpers | compiled subpage parser test |

## Card-Family Rules

| Family or saved type | Fields, options, defaults, and migrations | Class | Browser implementation | Firmware implementation |
|---|---|---|---|---|
| Switch (`""`) | Keep ordinary fields; normalize confirmation flags/text and `on_pattern`; keep `large_numbers` only when a state sensor and numeric precision make it applicable; drop unknown options | Declarative | `config_confirmation_options.ts`, `normalizeButtonConfig` | `switch_card_options_normalized`, final guard |
| Action | Canonicalize local and option-select actions; clear remote-only fields for those modes; migrate legacy vacuum actions; normalize script fields, confirmation text, state display options, and applicable `large_numbers` | Declarative + Migration + Hook | `cards/action.ts`, `config_confirmation_options.ts`, `normalizeButtonConfig` | action branches and `action_card_options_normalized` |
| Vacuum / lawn mower | Validate the command mode; supply mode-specific default icon; clear unit, precision, and unsupported options; keep vacuum clean-area identifier in `unit`; migrate legacy vacuum actions | Declarative + Migration | `cards/vacuum.ts`, `cards/lawn_mower.ts` | vacuum/mower branches in `normalize_parsed_cfg` |
| Alarm / alarm action | Clear unused sensor/unit/precision; validate action and visible-action list; cap and deduplicate actions; omit default PIN/display/action values; replace legacy action icons and labels | Declarative + Migration + Hook | `config_access_climate_alarm_options.ts`, `cards/alarm.ts` | `alarm_card_options_normalized`, alarm branches |
| Calendar / clock / timezone | Force family-owned entity/icon/label/sensor/unit fields; validate calendar date/datetime mode; preserve applicable `large_numbers`; fill contract entity defaults | Declarative | `config_sensor_options.ts`, `normalizeButtonConfig` | date/time branches and `date_time_card_options_normalized` |
| Climate / climate control | Alias `climate` to `climate_control`; clear sensor/unit; validate precision; normalize label/number display, temperature step, modal tabs, and applicable `large_numbers`; omit defaults | Declarative + Migration + Hook | `config_access_climate_alarm_options.ts`, `config_modal_tab_options.ts` | climate helpers and climate branch |
| Cover | Validate toggle/open/close/stop/modal mode; normalize position and modal tabs; tabs only apply to modal mode; omit default tabs | Declarative | `cards/slider.ts`, `config_modal_tab_options.ts` | cover helpers and cover branch |
| Door/window / presence | Clear unused entity/unit fields; validate door/window subtype; supply subtype-specific icon pairs; normalize `active_color`; drop numeric-only options | Declarative | `config_sensor_options.ts` | door/window and presence helpers/branches |
| Fan switch/speed/preset/oscillation/direction/control | Clear sensor/unit/precision; supply per-mode icons; only switch keeps an active icon; only control keeps validated/deduplicated tabs; omit default tabs | Declarative | `cards/fan.ts`, `config_modal_tab_options.ts` | fan branch and `fan_control_card_options_normalized` |
| Garage / gate / lock | Validate status/open/close/stop/lock/unlock modes; clear unit/precision; command modes force `icon_on=Auto`; normalize status-label option for garage/gate; lock drops all options | Declarative | card modules and `config_access_climate_alarm_options.ts` | family branches and option helpers |
| Internal / push / screen lock | Validate internal switch/push mode and icons; push drops unsupported options; screen lock owns all fields and forces its icon pair | Declarative | `cards/internal.ts`, `normalizeButtonConfig` | internal/push and screen-lock branches |
| Slider / light brightness | Remove retired slider-direction value from `sensor`; drop unsupported options | Migration + Declarative | brightness-slider branch | `brightness_slider_type` branch |
| Light switch / temperature / control | Switch clears sensor/unit/precision/options; temperature retains display fields but drops unsupported options; control clears display-only fields and normalizes tabs | Declarative | `cards/light_temperature.ts`, `config_modal_tab_options.ts` | light branches and `light_control_card_options_normalized` |
| Media | Validate mode; migrate `controls`; update legacy labels/icons; apply mode-specific precision; normalize volume limit, label/number display, playlist fields, and applicable `large_numbers`; omit defaults and inapplicable options | Declarative + Migration + Hook | `config_media_options.ts`, `cards/media.ts` | media helpers and media branch |
| Sensor / local sensor | Local mode clears remote-only options; validate numeric/icon/text precision; keep `large_numbers` only for numeric mode; normalize active colour and text-state mappings; migrate old high/low label keys | Declarative + Migration + Hook | `config_sensor_options.ts` | `sensor_card_options_normalized`, sensor branches |
| Subpage | Apply named visual preset; validate kind; keep `large_numbers` only for a suitable non-indicator sensor and numeric precision; drop unknown options | Declarative + Hook | `config_subpage_options.ts`, card preset helper | `subpage_card_options_normalized` and runtime preset handling |
| Weather | Alias `weather_forecast` to tomorrow; clear sensor; validate current/today/tomorrow mode; only forecast modes keep `large_numbers`; clear legacy default label | Declarative + Migration | `cards/weather.ts`, `normalizeButtonConfig` | weather branches and `weather_card_options_normalized` |
| Image | Clear sensor/unit/precision and active icon; label/icon only survive when enabled; validate modal mode; retired refresh options are ignored; omit defaults | Declarative + Migration | `config_image_options.ts` | image helpers and image branch |
| Webhook | Normalize HTTP method; only body-capable methods keep body; preserve encoded headers; clear precision and unknown options | Declarative + Hook | `cards/webhook.ts` | webhook helpers and webhook branch |
| Unknown type | Preserve the nine ordinary fields so the card remains inspectable, but remove unknown options under the current no-forward-preservation policy | Declarative | final preservation guard | final preservation guard |

## Baseline Decisions

- The browser's normalized structure is the intended result when an existing
  fixture exposed an `icon_on` representation difference. Firmware now uses
  `Auto` for non-switch fan modes and canonical option-select actions, matching
  the browser without changing the saved wire format.
- Backup envelope versions (1 and 2), compact/legacy saved-string selection,
  and runtime-only behaviour remain separate from the normalization contract.
- A future generated implementation must match this corpus before it can replace
  any production family branch.
