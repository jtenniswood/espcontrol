---
title: Slider Cards
description:
  How to use slider cards to control Home Assistant light and fan levels or Mac input and output volume.
---

# Slider

A slider card lets you control the brightness of a Home Assistant light, the speed of a Home Assistant fan, or the input and output volume of a connected Mac by dragging a vertical fill bar up or down.

![Slider card showing a lightbulb icon with a brightness fill bar](/images/card-slider.png)

For light-only controls, you may prefer the newer [Lights](/card-types/lights) card. It groups light switching, brightness, and colour temperature in one card type. Use Slider when you want a generic light or fan slider.

## Setting Up a Slider

1. Select a card and change its type to **Slider**.
2. Choose a **Control**:
   - **Home Assistant light or fan** — enter the entity to control, such as `light.living_room` or `fan.office_fan`.
   - **Mac output volume** — controls the speakers or audio device currently selected for Mac output.
   - **Mac input volume** — controls the microphone or audio device currently selected for Mac input.
3. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
4. Choose an **Off Icon** and **On Icon**. Existing sliders that only had one icon keep using that same icon for both states unless you change it.

## How It Works on the Panel

- **Drag** the slider to set the brightness or fan speed from 0 to 100 percent. Releasing the slider sends the new value to Home Assistant.
- For lights, the slider uses Home Assistant's brightness control.
- For fans, the slider uses Home Assistant's percentage speed control.
- A coloured **fill bar** shows the current level in real time as it rises from the bottom of the card.
- When the light or fan changes externally (from Home Assistant or another control), the fill bar updates automatically to reflect the current level.
- Mac volume sliders update when macOS volume changes and are disabled whenever the Companion app is disconnected or the selected audio device does not expose a software volume control.

## On and Off Icons

Slider cards always have separate **Off Icon** and **On Icon** fields. Use the same icon in both fields if you do not want the icon to change while the light or fan is on.
