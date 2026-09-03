---
title: Slider Cards
description:
  How to use slider cards to control Home Assistant lights, fans, number entities, or Mac input and output volume.
---

# Slider

A slider card lets you control light brightness, fan speed, a Home Assistant number value, or the input and output volume of a connected Mac by dragging a vertical fill bar up or down.

![Slider card showing a lightbulb icon with a brightness fill bar](/images/card-slider.png)

For light-only controls, you may prefer the newer [Lights](/card-types/lights) card. It groups light switching, brightness, and colour temperature in one card type. Use Slider when you want a simple light, fan, `number`, or `input_number` control.

## Setting Up a Slider

1. Select a card and change its type to **Slider**.
2. Choose a **Control**:
   - **Home Assistant** — enter the entity to control, such as `light.living_room`, `fan.office_fan`, `number.boiler_target`, or `input_number.test_level`.
   - **Mac output volume** — controls the speakers or audio device currently selected for Mac output.
   - **Mac input volume** — controls the microphone or audio device currently selected for Mac input.
3. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
4. Choose an **Off Icon** and **On Icon**. Existing sliders that only had one icon keep using that same icon for both states unless you change it.

## How It Works on the Panel

- **Drag** the slider and release it to send one new value to Home Assistant.
- For lights, the slider uses Home Assistant's brightness control.
- For fans, the slider uses Home Assistant's percentage speed control.
- For `number` entities and `input_number` helpers, the slider uses the minimum, maximum, step, and unit reported by Home Assistant. While you drag, the icon changes to the exact value that will be sent.
- A coloured **fill bar** shows the current level in real time as it rises from the bottom of the card.
- When the entity changes externally (from Home Assistant or another control), the fill bar updates automatically. Number sliders also adapt if Home Assistant changes their range or step.
- Mac volume sliders update when macOS volume changes and are disabled whenever the Companion app is disconnected or the selected audio device does not expose a software volume control.

`number.*` entities normally belong to a device or integration, while `input_number.*` entities are helpers created in Home Assistant. EspControl supports both and sends the matching Home Assistant action automatically.

## On and Off Icons

Slider cards always have separate **Off Icon** and **On Icon** fields. Use the same icon in both fields if you do not want the icon to change while a light or fan is on. Number sliders use the normal icon whenever they are not being dragged.
