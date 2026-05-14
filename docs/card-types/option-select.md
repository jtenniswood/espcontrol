---
title: Option Select Cards
description:
  How to use Option Select cards on your EspControl panel to choose Home Assistant select and input_select options.
---

# Option Select

An Option Select card shows the current value of a Home Assistant `select` or `input_select` entity. When you tap the card, EspControl opens a simple list of available options and sends the selected option back to Home Assistant.

Use this for things like WLED presets, lighting scenes exposed as a select entity, room modes, house modes, and other helpers where the choice needs to be made from the panel.

## Setting Up an Option Select Card

1. Select a card and change its type to **Option Select**.
2. Enter the **Select Entity**, for example `select.wled_preset` or `input_select.house_mode`.
3. Set a **Label** if you want the card to use custom text.

The panel automatically reads the entity's current value and its available options from Home Assistant.

## Supported Entities

| Home Assistant domain | Example |
|---|---|
| `select` | `select.wled_preset` |
| `input_select` | `input_select.house_mode` |

## How It Works on the Panel

When Home Assistant reports the entity state, the card shows that value on the card. When you tap the card, EspControl opens the option list from the entity's `options` attribute.

Choosing an option sends:

| Entity domain | Home Assistant action |
|---|---|
| `select` | `select.select_option` |
| `input_select` | `input_select.select_option` |

::: info Requires Home Assistant actions
Option Select cards send Home Assistant actions from the panel. If selecting an option does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
