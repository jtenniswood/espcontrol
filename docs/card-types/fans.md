---
title: Fan Cards
description:
  How to use experimental Fan cards on your EspControl panel for Home Assistant fan entities.
---

# Fans

::: warning Experimental
Fan cards are currently behind **Developer/Experimental Features**. To add one, open the setup page with `?developer=experimental`, enable **Developer/Experimental Features** in Settings, then choose **Fans** from the card type list.
:::

The Fans card is a guided card type for Home Assistant `fan` entities. It keeps the common fan controls together, so one picker entry can create a switch, speed slider, oscillation button, direction button, or preset selector.

## Setting Up a Fan Card

1. Select a card and change its type to **Fans**.
2. Choose the fan **Type**:
   - **Switch** turns the fan on or off with a tap.
   - **Speed** lets you drag a vertical slider from 0 to 100 percent.
   - **Oscillation** toggles oscillation on supported fans.
   - **Direction** switches between forward and reverse on supported fans.
   - **Preset** opens a list of preset modes from Home Assistant.
3. Enter the Home Assistant fan entity, for example `fan.bedroom`.
4. Set a **Label** if you want custom text. If left blank, the friendly name from Home Assistant is used.
5. Choose the icon fields shown for the selected type.

## How It Works on the Panel

- **Switch** sends `fan.turn_on` or `fan.turn_off`.
- **Speed** sends `fan.turn_off` at 0 percent, or `fan.turn_on` with `percentage` from 1 to 100.
- **Oscillation** sends `fan.oscillate` with the opposite of the current oscillating state.
- **Direction** sends `fan.set_direction` with `forward` or `reverse`.
- **Preset** reads `preset_modes`, opens a full-screen picker, and sends `fan.set_preset_mode`.

If Home Assistant does not expose the attribute needed by a fan type, that card is disabled on the panel until the attribute is available.

## Fans vs Slider

Use **Fans** when you want fan-specific setup and actions. The older [Slider](/card-types/sliders) card can still control a `fan` entity's percentage speed and remains unchanged.

::: info Requires Home Assistant actions
Fan cards send Home Assistant actions from the panel. If tapping or dragging a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
