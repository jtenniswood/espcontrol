---
title: Slider Cards
description:
  How to use slider cards on your EspControl panel to control lights, fans, and number entities from Home Assistant.
---

# Slider

A slider card lets you control light brightness, fan speed, or a Home Assistant number value by dragging a vertical fill bar up or down.

![Slider card showing a lightbulb icon with a brightness fill bar](/images/card-slider.png)

For light-only controls, you may prefer the newer [Lights](/card-types/lights) card. It groups light switching, brightness, and colour temperature in one card type. Use Slider when you want a simple light, fan, `number`, or `input_number` control.

## Setting Up a Slider

1. Select a card and change its type to **Slider**.
2. Enter an **Entity** — for example, `light.living_room`, `fan.office_fan`, `number.boiler_target`, or `input_number.test_level`.
3. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
4. Choose an **Off Icon** and **On Icon**. Existing sliders that only had one icon keep using that same icon for both states unless you change it.

## How It Works on the Panel

- **Drag** the slider and release it to send one new value to Home Assistant.
- For lights, the slider uses Home Assistant's brightness control.
- For fans, the slider uses Home Assistant's percentage speed control.
- For `number` entities and `input_number` helpers, the slider uses the minimum, maximum, step, and unit reported by Home Assistant. While you drag, the icon changes to the exact value that will be sent.
- A coloured **fill bar** shows the current level in real time as it rises from the bottom of the card.
- When the entity changes externally (from Home Assistant or another control), the fill bar updates automatically. Number sliders also adapt if Home Assistant changes their range or step.

`number.*` entities normally belong to a device or integration, while `input_number.*` entities are helpers created in Home Assistant. EspControl supports both and sends the matching Home Assistant action automatically.

## On and Off Icons

Slider cards always have separate **Off Icon** and **On Icon** fields. Use the same icon in both fields if you do not want the icon to change while a light or fan is on. Number sliders use the normal icon whenever they are not being dragged.
