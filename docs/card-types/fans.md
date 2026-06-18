---
title: Fan Cards
description:
  How to use fan cards on your EspControl panel to control Home Assistant fan entities.
---

# Fans

Fan cards are grouped controls for Home Assistant `fan` entities. They can show a fan switch, speed slider, oscillation toggle, direction control, or preset picker.

Use Fan cards when you want fan-specific controls. Use [Slider](/card-types/sliders) when you only need a simple fan speed slider, or [Switch](/card-types/switches) when you only need basic on/off fan control.

## Setting Up a Fan Card

1. Select a card and change its type to **Fans**.
2. Choose the fan **Type**:
   - **Switch** turns the fan on or off.
   - **Speed** lets you drag a vertical slider from 0 to 100 percent.
   - **Oscillation** toggles fan oscillation when the integration supports it.
   - **Direction** changes fan direction when the integration supports it.
   - **Preset** opens the fan's preset mode list when presets are available.
3. Enter the **Fan Entity**, for example `fan.bedroom`.
4. Set a **Label** if you want custom text. If left blank, the friendly name from Home Assistant is used when available.
5. Choose the icon fields shown for the selected type.

## Fan Types

| Type | What it does | Home Assistant support needed |
|---|---|---|
| **Switch** | Turns the fan on or off and shows active state. | Basic `fan.turn_on` and `fan.turn_off` support |
| **Speed** | Sets fan percentage from 0 to 100. | Percentage speed support |
| **Oscillation** | Toggles oscillation. | Oscillating support |
| **Direction** | Switches fan direction. | Direction support |
| **Preset** | Shows available preset modes and sends the selected preset. | `preset_modes` support |

## How It Works on the Panel

- **Switch** behaves like a fan-specific Switch card with separate off and on icons.
- **Speed** behaves like a vertical slider and follows fan speed changes made elsewhere in Home Assistant.
- **Oscillation** shows and toggles the current oscillation state.
- **Direction** sends the supported direction change for the fan integration.
- **Preset** opens a picker using the preset modes Home Assistant reports for the entity.

Not every Home Assistant fan integration supports every feature. If a control does not appear to do anything, check the fan entity in Home Assistant and confirm the integration exposes that capability.

::: info Requires Home Assistant actions
Fan cards send Home Assistant fan actions from the panel. If tapping or dragging a card does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
