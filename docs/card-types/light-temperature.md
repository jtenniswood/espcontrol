---
title: Light Temperature Slider Cards
description:
  How to use light temperature slider cards on your EspControl panel to control the colour temperature of a Home Assistant light entity.
---

# Light Temperature Slider

::: warning Experimental
Light temperature slider cards are currently behind **Developer/Experimental Features**. Open the setup page with `?developer=experimental`, then enable **Developer/Experimental Features** in the Developer settings section.
:::

A light temperature slider card lets you control the colour temperature of a Home Assistant light entity by dragging a vertical fill bar. The bottom of the slider is the warmest (lowest kelvin) setting and the top is the coolest (highest kelvin).

## Setting Up a Light Temperature Slider Card

1. Select a card and change its type to **Light Temperature Slider**.
2. Enter an **Entity ID** — the Home Assistant light entity you want to control (for example, `light.living_room`).
3. Set **Min Color Temp (K)** and **Max Color Temp (K)** to match the range supported by your light. The defaults (2000 K–6500 K) cover most tunable white bulbs.
4. Choose an **Icon** (optional). If left as Auto, the domain default icon is used.
5. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.

## Options

### Label

Choose **Label** to show your configured label on the card. Choose **Setting** to replace the card's label with the live colour-temperature reading (for example, `2500K`) while the light is on. The value updates as the slider is dragged and as the light's `color_temp_kelvin` attribute changes in Home Assistant. When the light is off, the label reverts to your configured label (or the entity's friendly name).

## How It Works on the Panel

- **Drag** the fill bar to set the colour temperature. Releasing the slider sends the new value to Home Assistant using `light.turn_on` with `color_temp_kelvin` (which will also turn the light on if it is currently off).
- Tapping the card without dragging does nothing — only a drag-and-release sends a command.
- When the light is **off**, the slider renders empty regardless of the last colour temperature reported by Home Assistant.
- When the light's colour temperature changes externally (from Home Assistant or another control), the fill bar updates automatically.
- The slider covers only the kelvin range you configured — values outside that range are clamped.
