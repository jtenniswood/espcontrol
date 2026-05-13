---
title: Light Modal UX Plan
description: UX plan for a small-screen Home Assistant light control modal covering brightness, colour temperature, RGB, presets, and effects.
---

# Light Modal UX Plan

Source requirement: [GitHub issue #188](https://github.com/jtenniswood/espcontrol/issues/188).

Home Assistant references:

- [Light entity developer docs](https://developers.home-assistant.io/docs/core/entity/light/)
- [Light integration service docs](https://www.home-assistant.io/integrations/light/)

## Goal

Build a full light control that feels as quick as Home Assistant's light modal, but is shaped for EspControl's wall panels. The main job is simple: walk up to the screen, adjust the light, and leave. Advanced colour and effect controls should be available only when the light supports them, and should not make the everyday brightness/on-off path harder.

The smallest target screens are the 4-inch square displays and the 4.3-inch portrait display, so the design should start there and scale up.

## Current Starting Point

EspControl already has separate light controls:

| Current control | What it does well | Limitation for this issue |
|---|---|---|
| Light switch | Very fast on/off control | No brightness, colour temperature, RGB, presets, or effects |
| Light brightness | Direct slider for brightness | No colour temperature, presets, RGB, or effects |
| Light temperature | Direct slider for colour temperature | Separate from brightness and switch control |

The firmware also already has reusable full-screen control patterns for media volume and climate. The light modal should reuse the same overall shell: full-screen panel, back button, consistent scaling, and capability-aware menus.

## Product Decision

Add a dedicated full light-control mode instead of changing existing light cards by default.

Reason: existing users may rely on the current direct switch, brightness, and colour temperature cards. A new full-control mode lets users choose the richer interaction without breaking current layouts.

Recommended setup wording:

| Setup option | Device behaviour |
|---|---|
| Switch | Tap toggles the light |
| Brightness | Drag sets brightness, tap toggles |
| Colour Temperature | Drag sets colour temperature |
| Full Light Control | Tap opens the light modal |

The existing options stay useful for one-action controls. The new full control is for rooms or important lights where richer control is worth one extra tap.

## Compact Card

The compact card is an entry point, not a dense control surface.

It should show:

- the configured label, falling back to Home Assistant `friendly_name`
- a light icon
- active/inactive background colour using the existing on/off palette
- optional small colour or temperature swatch only if it remains visually clean

Tap opens the modal. The card should not try to show brightness readouts, colour temperature values, or last-changed timers. Those were listed as probably not required and would add clutter on small displays.

## Modal Layout

The modal should use a full-screen panel, visually matching the current media and climate controls.

Core layout:

| Area | Contents |
|---|---|
| Top-left | circular back button |
| Top-centre | light name, short enough to fit |
| Top-right | circular power button |
| Centre | one active control: brightness, colour temperature, presets, or RGB |
| Bottom | compact mode rail with icons for available modes |

The modal should not show all controls at once. It should show one clear control at a time, because the smallest screens do not have enough room for a good brightness slider, colour slider, preset grid, RGB picker, and effects menu together.

## Primary Flow

Default to the most common action:

1. User taps the light card.
2. The modal opens on brightness.
3. User drags the large brightness control or taps the power button.
4. User taps back, or the modal closes through the normal back control.

Brightness is the default because it is useful for almost every dimmable light. For on/off-only lights, the modal should simplify down to the power control plus any supported effects.

## Mode Rail

The bottom mode rail should show only the modes supported by the selected light.

| Mode | Show when | Control |
|---|---|---|
| Brightness | light supports brightness, or brightness is present | large vertical brightness slider |
| Colour temperature | `supported_color_modes` includes `color_temp` | warm-to-cool slider |
| Presets | presets are configured and light supports colour temperature or colour | swatch grid |
| RGB | light supports `hs`, `rgb`, `rgbw`, `rgbww`, or `xy` style colour | colour wheel |
| Effects | `effect_list` has usable entries | option menu |

Use icons in the rail rather than long labels. A selected mode can show a short title near the top or centre if there is enough space, but the control itself should be understandable without instructions.

## Brightness Control

Use a large rounded vertical slider, inspired by the examples in the issue.

Behaviour:

- fill height represents brightness
- dragging sends `light.turn_on` with `brightness_pct`
- power button sends `light.turn_off` or `light.turn_on`
- while dragging, a temporary percentage readout may appear inside the slider
- after release, return to the clean label-free view

The temporary readout is useful feedback without permanently cluttering the modal.

## Colour Temperature Control

Use a warm-to-cool slider. Keep the same physical interaction as brightness so users do not have to learn a new gesture.

Behaviour:

- slider bottom is warm, slider top is cool
- use Home Assistant `min_color_temp_kelvin` and `max_color_temp_kelvin` when available
- fall back to the existing configured range used by the current colour temperature card
- send `light.turn_on` with `color_temp_kelvin`
- while dragging, a temporary Kelvin value may appear, then disappear after release

This mirrors the current light temperature card while bringing it into the full modal.

## Presets

Presets should be configured from the web setup page, not edited on the device. The device should only apply them.

Recommended MVP:

- one global preset palette shared by all light modal cards
- up to 8 visible presets on the device
- each preset stores a swatch colour plus the Home Assistant command values
- presets that the selected light cannot support should be hidden or disabled

Why global first: it is simpler to understand and avoids repeating the same warm, cool, night, relax, and colour presets on every light. Per-light overrides can come later if real users need room-specific palettes.

Preset examples:

| Preset type | Command shape |
|---|---|
| Warm white | `color_temp_kelvin` plus optional `brightness_pct` |
| Cool white | `color_temp_kelvin` plus optional `brightness_pct` |
| RGB colour | `rgb_color` or `hs_color` plus optional `brightness_pct` |

On the device, show presets as circular swatches. Do not show preset editing controls on the touchscreen.

## RGB Picker

RGB is useful but more advanced. Put it behind the RGB mode rather than making it part of the default brightness flow.

Recommended control:

- a large colour wheel for capable lights
- selected colour marker large enough to see on 4-inch displays
- optional brightness remains in the brightness mode, not squeezed beside the wheel

If a full colour wheel proves too heavy for firmware performance, a practical fallback is a fixed colour palette with more swatches. That fallback should still live in the RGB mode so the overall flow stays the same.

## Effects

Effects should use a simple menu, not a large always-visible list.

Behaviour:

- show the Effects rail icon only when `effect_list` has usable options
- tap opens an option menu
- current effect is highlighted
- selecting an effect sends `light.turn_on` with `effect`
- if the effect list is long, use a paged list rather than shrinking rows below comfortable touch size

Avoid putting effects in the main default flow. Effects are less common than brightness and colour temperature.

## Capability Rules

The modal should adapt to each Home Assistant light. Unsupported controls should disappear.

Use these Home Assistant values where available:

| Data | Purpose |
|---|---|
| entity state | on/off/unavailable |
| `friendly_name` | label fallback |
| `brightness` | brightness state, 0 to 255 |
| `color_mode` | current active colour mode |
| `supported_color_modes` | decides which controls appear |
| `color_temp_kelvin` | current colour temperature |
| `min_color_temp_kelvin` | warm/cool range lower bound |
| `max_color_temp_kelvin` | warm/cool range upper bound |
| `rgb_color` or `hs_color` | current colour |
| `effect` | current effect |
| `effect_list` | available effects |

If the light is `unknown` or `unavailable`, dim the modal controls and avoid sending commands until the entity becomes available again.

## Small Screen Rules

Start design checks at 480 px short side.

Rules:

- keep touch targets at least 56 px where possible
- never show more than one main control at a time
- keep the bottom rail to 4 visible icons if possible
- move Effects into a menu rather than adding another large panel
- avoid permanent numeric readouts
- truncate long light names cleanly
- preserve the same back behaviour used by climate and media modals, including returning to the subpage that opened the control

## Web Setup Plan

The setup page needs two changes:

1. Add the Full Light Control option under Lights.
2. Add a global Light Presets settings area.

Suggested preset editor fields:

| Field | Purpose |
|---|---|
| Preset name | shown in setup, not necessarily on device |
| Swatch colour | what appears on the device |
| Type | colour temperature or RGB |
| Brightness | optional brightness percentage |
| Kelvin | for colour temperature presets |
| RGB/HS colour | for RGB presets |

The web preview should show a light card entry point, not try to fully simulate the modal.

## Implementation Phases

### Phase 1 - UX and Data Contract

- Agree this plan.
- Confirm whether Full Light Control is a new setup option.
- Decide the first global preset defaults.

### Phase 2 - Modal Shell

- Create a light modal context using the same layout scaling approach as media volume and climate.
- Add subscriptions for on/off, friendly name, brightness, colour temperature, supported colour modes, RGB/HS colour, and effects.
- Add unavailable handling.

### Phase 3 - Brightness MVP

- Add compact card entry point.
- Add modal open/close.
- Add power button.
- Add brightness slider.
- Verify 4-inch square and 4.3-inch portrait layouts first.

### Phase 4 - Colour Temperature and Presets

- Add capability-driven colour temperature mode.
- Add global preset storage in setup.
- Add preset swatch mode on device.

### Phase 5 - RGB and Effects

- Add RGB picker or palette fallback.
- Add effect option menu.
- Add current selection highlighting where reliable.

### Phase 6 - Polish and Documentation

- Add card type docs.
- Add screenshots after implementation.
- Run firmware compile checks across supported devices before release.

## Acceptance Criteria

- Existing light switch, brightness, and colour temperature cards keep their current behaviours.
- A new full light-control card can open the modal.
- Unsupported controls are hidden for each light.
- Brightness can be changed without needing to enter an advanced screen.
- Colour temperature appears only for capable lights.
- RGB appears only for capable lights.
- Effects appear only for lights with effects.
- Presets are configured in the web setup UI and applied from the device.
- The modal works cleanly on 480x480 and 480x800 displays.
- The modal returns to the same home page or subpage it was opened from.

## Open Questions

| Question | Recommendation |
|---|---|
| Should the modal replace existing brightness cards? | No. Add Full Light Control first to avoid changing existing setups. |
| Should presets be global or per light? | Global for MVP, per-light overrides later if needed. |
| How many presets should fit on-device? | Up to 8 visible swatches. |
| Should RGB be a primary mode? | No. Keep it available, but secondary to brightness and colour temperature. |
| Should brightness and colour temperature readouts be permanent? | No. Show temporary readouts only while dragging. |
