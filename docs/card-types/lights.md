---
title: Light Cards
description:
  How to use light cards on your EspControl panel for light switching, brightness, colour temperature, and colour presets.
---

# Lights

The Lights card is a guided card type for Home Assistant `light` entities. It keeps the common light controls together, so you can choose whether a light should behave like a switch, a brightness slider, a colour temperature slider, or a full light control popup.

![Light brightness card showing a lightbulb icon and dimmer fill bar](/images/card-light-brightness.png)

## Setting Up a Light Card

1. Select a card and change its type to **Lights**.
2. Choose the light **Type**:
   - **All Controls** opens a full-screen light control with brightness, colour temperature, and colour preset tabs.
   - **Switch** turns the light on or off with a tap.
   - **Brightness** lets you drag a vertical slider from 0 to 100 percent.
   - **Colour Temperature** lets you drag a vertical slider between warm and cool white.
3. Enter the Home Assistant light entity, for example `light.living_room`.
4. Set a **Label** if you want custom text. If left blank, the friendly name from Home Assistant is used.
5. Choose the icon fields shown for the selected type.

## All Controls

All Controls gives one card a full-screen popup for the main light controls.

- Tapping the card opens tabs for brightness, colour temperature, and colour presets.
- The power tab toggles the light when you tap anywhere on its on/off control.
- The brightness tab lets you drag from 0 to 100 percent.
- The colour temperature tab lets you move between warm and cool white.
- The colour tab provides preset colour swatches for RGB-capable lights.
- The card uses separate **Off Icon** and **On Icon** settings and lights up while Home Assistant reports the light as on.

Use this when one light should be adjustable without needing separate brightness, colour temperature, and colour buttons on the home screen.

## Switch

Switch mode is a light-specific version of a normal [Switch](/card-types/switches) card.

- Tapping the card toggles the light through Home Assistant.
- The card uses separate **Off Icon** and **On Icon** settings.
- The card lights up while Home Assistant reports the light as on.

Use this when you only need quick on/off control.

## Brightness

Brightness mode gives the card a vertical fill bar.

- Drag the fill bar to set brightness from 0 to 100 percent.
- Releasing the slider sends `light.turn_on` with `brightness_pct`.
- Dragging to 0 turns the light off.
- The fill bar follows brightness changes made elsewhere in Home Assistant.

Use this when you want direct dimmer-style control from the panel.

## Colour Temperature

Colour Temperature mode controls the white temperature of a light.

- Drag the fill bar to move between the configured minimum and maximum Kelvin values.
- The default range is **2000 K** to **6500 K**.
- The bottom of the slider is warmer; the top is cooler.
- Releasing the slider sends `light.turn_on` with `color_temp_kelvin`.

This mode needs a Home Assistant light that supports colour temperature. If a light only supports on/off or brightness, use **Switch**, **Brightness**, or **All Controls** without relying on the colour temperature tab.

## Lights vs Slider

Use **Lights** for light-specific controls. Use [Slider](/card-types/sliders) when you want a generic slider for a light or fan, especially fan speed control.

::: info Requires Home Assistant actions
Light cards send Home Assistant actions from the panel. If tapping or dragging a card does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
