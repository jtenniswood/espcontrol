---
title: Climate Control Reimplementation Spec
description: Technical specification for rebuilding EspControl climate cards and thermostat detail controls without using the existing source code.
---

# Climate Control Reimplementation Spec

This document describes the climate control feature as a product and integration contract. It is intended to be enough to build a new implementation without copying or referring to the existing climate control source code.

The feature has two surfaces:

- A compact **Climate** card on the home grid or inside a subpage.
- A full-screen thermostat detail view opened by tapping the card.

The feature controls a Home Assistant `climate` entity. It depends on Home Assistant state subscriptions for display data and Home Assistant service calls for user actions.

## Feature Status

Climate cards are treated as an experimental feature. A firmware or app build may hide, ignore, or render them as a default card unless experimental features are enabled.

## Configuration Model

Each climate card needs these saved fields:

| Field | Required | Purpose | Example |
|---|---:|---|---|
| `entity` | Yes | Home Assistant climate entity ID. | `climate.living_room` |
| `label` | No | User-facing name. If empty, use Home Assistant `friendly_name`, then the entity ID, then `Climate`. | `Living Room` |
| `type` | Yes | Card type marker. | `climate` |
| `precision` | No | Display decimal places for temperatures. Empty or `0` means whole numbers. Climate cards may append a custom arc range as `precision:min:max`. | `1:16:30` |

The card should ignore generic sensor/unit fields used by other card types. It should force or default its icon to a thermostat in configuration previews.

Supported precision values:

| Saved value | Display |
|---|---|
| empty or `0` | Whole numbers |
| `1` | 1 decimal place |
| `2` | 2 decimal places |
| `3` | 3 decimal places, allowed by firmware-level parsing even if the setup UI only offers up to 2 |

When the setup UI advanced range fields are used, save the value in `precision` as `precision:min:max`.
For example, `0:16:30` means whole-number display with an arc limited to 16-30, and `1:16:30` means one decimal place with the same range.
If no custom range is saved, use Home Assistant `min_temp` and `max_temp`.

Temperature units come from the global panel temperature setting, not from a per-card field. The implementation should display the panel's current unit symbol, such as `°C` or `°F`.

## Home Assistant Data Contract

Subscribe to the climate entity state and the following attributes.

| Source | Meaning | Required behavior |
|---|---|---|
| entity state | Current HVAC mode. Usually `off`, `heat`, `cool`, `heat_cool`, `auto`, `dry`, or `fan_only`. | Determines availability and whether low/high dual target editing applies. |
| `friendly_name` | Home Assistant display name. | Used as fallback label when no custom label is configured. |
| `hvac_action` | What the device is currently doing. Usually `idle`, `heating`, `cooling`, `drying`, `fan`, `off`, `unknown`, or `unavailable`. | Drives status text, active color, and checked/active card state. |
| `hvac_modes` | List of selectable HVAC modes. | Drives the mode menu. |
| `temperature` | Single target temperature. | Used when the entity has one target. |
| `target_temp_low` | Lower target temperature. | Used for heat/cool range mode. |
| `target_temp_high` | Upper target temperature. | Used for heat/cool range mode. |
| `min_temp` | Minimum allowed target. | Lower bound for arc and minus button. |
| `max_temp` | Maximum allowed target. | Upper bound for arc and plus button. |
| `target_temp_step` | Supported increment. | Step size for plus/minus and rounding arc changes. |
| `fan_mode` | Current fan mode. | Shown on the fan control if fan modes exist. |
| `fan_modes` | List of selectable fan modes. | Determines whether fan control appears and what options it contains. |
| `swing_mode` | Current swing mode. | Shown on the swing control if swing modes exist. |
| `swing_modes` | List of selectable swing modes. | Determines whether swing control appears and what options it contains. |
| `preset_mode` | Current preset mode. | Store for future preset UI support. |
| `preset_modes` | List of selectable preset modes. | Store for future preset UI support. |

Treat the entity as unavailable when the entity state is empty, `unknown`, or `unavailable`. In that case, show placeholder target text and avoid showing the card as active.

If an optional attribute is missing or cannot be parsed:

- For target temperatures, mark that target as absent.
- For `min_temp`, keep the default `5`.
- For `max_temp`, keep the default `35`.
- For `target_temp_step`, keep the default `0.5`.
- For mode lists, show no related control.

## Option List Parsing

Home Assistant may expose list attributes in different textual forms depending on the API path. A robust implementation should accept at least:

- quoted list-like text, such as `['off', 'heat', 'cool']`
- comma-separated or bracketed text, such as `[off, heat, cool]`
- mixed case enum-like text

Normalize options by:

- trimming whitespace
- discarding empty entries
- discarding accidental header names such as `hvacMode`, `fanMode`, `swingMode`, and `presetMode`
- de-duplicating case-insensitively
- converting all-uppercase enum tokens to lower case

Display option labels in friendly text:

| Raw value | Label |
|---|---|
| `off` | Off |
| `heat` | Heat |
| `cool` | Cool |
| `heat_cool` | Heat/Cool |
| `auto` | Auto |
| `dry` | Dry |
| `fan_only` | Fan |
| anything else | Sentence-style text, replacing separators where appropriate |

## Compact Card Behavior

The compact climate card is a status and entry point. It should be clickable when a climate context exists.

### Compact Card Data

Show a large target value:

| Available data | Card value |
|---|---|
| Entity unavailable | `--` |
| Both `target_temp_low` and `target_temp_high` present | `low-high`, for example `18-22` |
| `temperature` present | Single target, for example `20` |
| Only `target_temp_low` present | Low target |
| Only `target_temp_high` present | High target |
| No target data | `--` |

Show the temperature unit beside the value unless the value is `--`.

Show the card label using this priority:

1. Active action label, when the climate is actively doing something.
2. Configured card label.
3. Home Assistant `friendly_name`.
4. Entity.
5. `Climate`.

Active action labels:

| Data | Label |
|---|---|
| unavailable | Unavailable |
| `hvac_action = heating` | Heating |
| `hvac_action = cooling` | Cooling |
| `hvac_action = drying` | Drying |
| `hvac_action = fan` | Fan |
| `hvac_mode = off` | Off |
| otherwise | Idle |

Treat an action as active only when:

- the entity is available
- `hvac_mode` is not `off`
- `hvac_action` is not empty, `idle`, `off`, `unknown`, or `unavailable`

### Compact Card Color

Use the panel's configured active/on color while the card is active. Use the panel's configured inactive/off color otherwise. If those colors are not configured, use sensible defaults:

| Role | Default |
|---|---|
| Active/on color | `#FF8C00` |
| Inactive/off color | `#313131` |

The card should also expose an active/checked state when active so the visual style is consistent with other control cards.

### Tap Behavior

Tapping a compact climate card opens the thermostat detail view. Tapping should not directly toggle the climate entity.

## Detail View Layout

The detail view should be a dedicated screen/page, not a modal over the grid. It should include:

- dark full-screen background
- main card/panel area
- circular back button in the top-left
- circular mode/menu button in the top-right, using a three-dots icon
- large circular temperature arc
- large target temperature in the center of the arc
- unit symbol aligned near the target number
- current status text below the target number
- minus and plus buttons near the bottom of the arc
- optional low/high selector for dual target mode
- optional fan and swing chips near the bottom
- popup option menus for HVAC, fan, and swing selections

The layout must scale by display size. Use the shorter display side as the main scaling input. On small displays, keep buttons at least large enough for touch, roughly `56-66px` minimum. On large displays, cap very large round buttons around `116px`.

The detail view should return to the exact page from which it was opened. For example, if opened from a subpage, the back button returns to that subpage, not always to the home page.

## Detail View Data

The center target value shows the selected editable target:

- In `heat_cool` mode with both low and high targets, show the currently selected low or high value.
- In normal single-target mode, show `temperature`.
- If `temperature` is absent, fall back to low, then high, then a default of `20` clamped to the supported range.

Show the global temperature unit beside the target value.

Show the current action label below the target value using the same action mapping as the compact card.

In dual target mode:

- Show a small hint: `Low target` or `High target`.
- Show `Low` and `High` chips.
- Highlight the selected chip.
- Editing low must not allow low to become greater than or equal to high.
- Editing high must not allow high to become less than or equal to low.
- The minimum gap between low and high is one `target_temp_step`.

In single target mode:

- Hide the low/high hint and chips.

## Temperature Arc Behavior

The arc represents the editable target temperature.

Use tenths internally for the arc range if the UI toolkit only supports integer arc values:

- arc minimum = `min_temp * 10`
- arc maximum = `max_temp * 10`
- arc value = selected target * `10`

Clamp the selected target to `[min_temp, max_temp]`.

When the user drags the arc:

1. Convert the arc value back to a temperature.
2. Round it to the nearest `target_temp_step`.
3. Clamp it to the supported range.
4. Update the screen immediately.
5. Send `climate.set_temperature` when the user releases the arc.

When the user presses plus or minus:

1. Add or subtract `target_temp_step`.
2. Round and clamp.
3. Update the screen immediately.
4. Send `climate.set_temperature` using a short debounce, around `450ms`, so repeated taps do not flood Home Assistant.

## Service Calls

All service calls target the configured climate entity.

### Set Single Temperature

Use this call when not in dual target mode:

```yaml
service: climate.set_temperature
data:
  entity_id: climate.living_room
  temperature: "20.0"
```

Send temperature values with one decimal place for service calls, even when display precision is whole numbers. This avoids losing half-degree steps.

### Set Low/High Temperature

Use this call in dual target mode:

```yaml
service: climate.set_temperature
data:
  entity_id: climate.living_room
  target_temp_low: "18.0"
  target_temp_high: "22.0"
```

Always send both low and high together in dual target mode.

### Set HVAC Mode

```yaml
service: climate.set_hvac_mode
data:
  entity_id: climate.living_room
  hvac_mode: heat
```

After the user chooses a mode, update local state immediately so the UI feels responsive. Home Assistant state updates should later confirm or correct the value.

### Set Fan Mode

```yaml
service: climate.set_fan_mode
data:
  entity_id: climate.living_room
  fan_mode: auto
```

Only show fan controls when `fan_modes` has at least one option.

### Set Swing Mode

```yaml
service: climate.set_swing_mode
data:
  entity_id: climate.living_room
  swing_mode: vertical
```

Only show swing controls when `swing_modes` has at least one option.

### Set Preset Mode

Preset values should be tracked because Home Assistant climate entities often expose them. A future UI can call:

```yaml
service: climate.set_preset_mode
data:
  entity_id: climate.living_room
  preset_mode: eco
```

The current UI contract does not require a visible preset selector.

## Menus and Controls

### HVAC Mode Menu

Open from the top-right three-dots button. Show all `hvac_modes` as selectable options. Selecting an option sends `climate.set_hvac_mode`.

The menu should appear near the top-right button on compact screens and may be centered on larger screens. Use a dim overlay behind the menu. Tapping outside the menu dismisses it.

### Fan Menu

Show a `Fan` chip only when `fan_modes` exists. The chip text should be two lines:

```text
Fan
Auto
```

If no current fan mode is known, show `None` on the second line. Tapping the chip opens a menu of `fan_modes`. Selecting an option sends `climate.set_fan_mode`.

### Swing Menu

Show a `Swing` chip only when `swing_modes` exists. The chip text should be two lines:

```text
Swing
Vertical
```

If no current swing mode is known, show `None` on the second line. Tapping the chip opens a menu of `swing_modes`. Selecting an option sends `climate.set_swing_mode`.

## Colors

Use a restrained dark thermostat UI.

Recommended defaults:

| Role | Color |
|---|---|
| Screen background | `#111111` |
| Detail panel | `#252525` |
| Popup | `#242424` |
| Main text | `#D8D8D8` |
| Inactive arc | Inactive/off card color, default `#313131` |
| Neutral accent | `#BDBDBD` |
| Heating accent | `#A44A1C` |
| Cooling accent | `#1565C0` |
| Other active accent | Panel active/on color, default `#FF8C00` |

Use heating accent when `hvac_action` is `heating`, cooling accent when `hvac_action` is `cooling`, and the panel active/on color for other active actions.

## Temperature Unit Refresh

When the global temperature unit changes, update:

- compact card unit labels
- detail view unit label
- all visible target values that depend on precision/unit formatting

The climate card should not store its own unit.

## Error and Edge Cases

| Situation | Expected behavior |
|---|---|
| Entity missing | Setup UI should require an entity before saving. Runtime should show placeholder if somehow missing. |
| Home Assistant API unavailable | Do not send actions. Keep UI responsive and avoid crashes. |
| Entity unavailable | Compact card shows `--`, no unit, label falls back to configured/friendly label or unavailable state as appropriate. |
| Missing target values | Use `--` on compact card. Detail view may use a clamped default target only for editing. |
| Bad `min_temp`/`max_temp` | Keep defaults. If max is lower than min, force max to at least min plus a small range. |
| Bad or missing step | Use `0.5`. Ignore absurd steps such as zero or very large values. |
| Low/high targets cross | Clamp the edited value so low remains below high by at least one step. |
| User taps quickly | Debounce temperature service calls for plus/minus. |
| User drags arc | Update locally during drag, send once on release. |
| Mode list missing | Hide the related control. |

## Web Configurator Preview

The setup page preview does not need live Home Assistant data. It should render:

- sample value `20` formatted with selected precision
- global temperature unit
- configured label, or entity ID, or `Climate`
- thermostat type badge/icon

The setup UI should make clear that the entity must be a Home Assistant climate entity.

## Acceptance Checklist

A new implementation is compatible when all of these pass:

- A configured `climate.living_room` card shows `--` before Home Assistant data arrives.
- When Home Assistant reports `temperature = 20`, the compact card shows `20°C` or the configured global unit.
- With precision `1`, the compact card shows `20.0`.
- With `target_temp_low = 18` and `target_temp_high = 22`, the compact card shows `18-22`.
- When `hvac_action = heating`, the compact card label shows `Heating` and uses the active/on color.
- When `hvac_action = cooling`, the detail arc uses the cooling accent.
- When `hvac_mode = off`, the status label shows `Off` and the card is inactive.
- Tapping the compact card opens a detail view and the back button returns to the original page.
- Plus and minus change the selected target by `target_temp_step`.
- Dragging the arc updates the displayed target and sends one service call when released.
- In `heat_cool` mode, `Low` and `High` selectors appear and prevent crossed targets.
- HVAC mode options are generated from `hvac_modes` and send `climate.set_hvac_mode`.
- Fan controls appear only when `fan_modes` exists and send `climate.set_fan_mode`.
- Swing controls appear only when `swing_modes` exists and send `climate.set_swing_mode`.
- Changing the global panel temperature unit updates both compact and detail views.

## Suggested Internal State

A source-independent implementation can use this internal state shape:

```ts
type ClimateControlState = {
  entityId: string
  configuredLabel?: string
  friendlyName?: string
  available: boolean
  hvacMode: string
  hvacAction: string
  hvacModes: string[]
  fanMode?: string
  fanModes: string[]
  swingMode?: string
  swingModes: string[]
  presetMode?: string
  presetModes: string[]
  hasTarget: boolean
  target: number
  hasLow: boolean
  low: number
  hasHigh: boolean
  high: number
  editHigh: boolean
  minTemp: number
  maxTemp: number
  step: number
  precision: number
}
```

Default values:

```ts
const defaults = {
  available: false,
  hvacMode: 'off',
  hvacAction: 'idle',
  target: 20,
  low: 18,
  high: 22,
  minTemp: 5,
  maxTemp: 35,
  step: 0.5,
  precision: 0,
}
```

This state shape is not mandatory, but it captures the behavior required by the feature.
