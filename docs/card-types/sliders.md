---
title: Sliders
description:
  How to use slider cards on your Espcontrol panel to control light brightness from Home Assistant.
---

# Sliders

A slider card lets you control the brightness of a Home Assistant light entity by dragging a fill bar up or down (or left to right). 

<!-- ![Slider card showing a lightbulb icon with a brightness fill bar](/images/card-slider.png) -->

## Setting up a slider

1. Select a card and change its type to **Slider**.
2. Enter an **Entity ID** — the Home Assistant light entity you want to control (for example, `light.living_room`).
3. Choose an **Icon** (defaults to a lightbulb when set to **Auto**).
4. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
5. Pick a **Direction** — **Vertical** (default) or **Horizontal**.

## How it works on the panel

- **Drag** the slider to set the brightness from 0 to 100 percent. Releasing the slider sends the new brightness to Home Assistant.
- A coloured **fill bar** shows the current brightness level in real time — it rises from the bottom in vertical mode or extends from the left in horizontal mode.
- When the light turns on externally (from Home Assistant or another control), the fill bar updates automatically to reflect the current brightness.

## Direction

You can orient the slider in two directions:

- **Vertical** (default) — the fill bar grows upward from the bottom of the card.
- **Horizontal** — the fill bar grows from left to right.

Choose the direction that best fits your grid layout. Double-height cards work well with vertical sliders, while wide cards suit horizontal sliders.

## Change Icon When On

Enable **Change Icon When On** to show a different icon while the light is on. For example, you could use an outline lightbulb when off and a filled lightbulb when on.

When the light is off, the card reverts to the default icon.