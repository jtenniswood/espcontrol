---
title: Action Cards
description:
  How to use action cards on your EspControl panel to run Home Assistant scenes, scripts, automations, buttons, and helpers.
---

# Action

An Action card is a simple one-tap shortcut. It sends a selected Home Assistant action, opens an Option Select picker, or runs a registered local device action when you tap it.

Use one for shortcuts such as a scene, script, automation, Home Assistant button, helper, or a custom action registered on the panel.

## Setting Up an Action Card

1. Select a card and change its type to **Action**.
2. Set a **Label** - this is the text shown on the card.
3. Choose an **Action**.
4. Enter the matching **Entity** (not needed for Local Action).
5. Enter a value for **Set Number** or **Set Number Helper**, if used, and choose an icon.
6. For scripts, you can add fields and ask for confirmation before running it.

## Supported Actions

| Action | Example entity | Extra field |
|---|---|---|
| **Run Scene** | `scene.movie_mode` | None |
| **Run Script** | `script.goodnight` | Fields |
| **Trigger Automation** | `automation.goodnight` | None |
| **Press Button** | `button.restart_router` | None |
| **Press Input Button** | `input_button.doorbell` | None |
| **Toggle Helper** | `input_boolean.guest_mode` | None |
| **Set Number** | `number.target_level` | Value |
| **Set Number Helper** | `input_number.target_level` | Value |
| **Option Select** | `select.wled_preset` or `input_select.house_mode` | Opens option list |
| **Local Action** | Registered local action key | Runs on the panel |

Use **Set Number** for a `number.*` entity supplied by a device or integration. Use **Set Number Helper** for an `input_number.*` helper created in Home Assistant. The setup page checks that the selected action and entity match, and corrects older saved cards that used the other numeric action.

For more about choosing from a live list, see [Option Select](/card-types/option-select). For an action that runs on the panel without Home Assistant, see [Local Action](/card-types/local-actions).

## Show State

Action cards are normally stateless: they flash when tapped, then return to their normal colour.

Turn on **Show State** when an action should behave like a shortcut but still show whether something is active. For example, an Action card might run a scene called `scene.movie_mode`, while **State Entity** watches `input_boolean.movie_mode`.

Show State has three display modes:

- **Icon** — keeps the normal action icon, and can show a separate **On Icon** while the state entity is active.
- **Numeric** — shows a live sensor value, with optional **Unit**, **Unit Precision**, and **Large State Numbers** on larger cards.
- **Text** — shows the live state text where the card label normally appears.

When the state entity is active, Icon mode highlights the card. Numeric mode highlights the card when the live value is greater than zero, which is useful for counters and count-based indicators. If the state entity is unavailable, the card keeps its normal appearance and clears or falls back from the live state display until Home Assistant reports a usable value again.

The card flashes when pressed. Home Assistant actions use the entity you selected; Local Actions run on the panel. Use the dedicated card types for richer controls such as lights, covers, media, climate, locks, and vacuums.

::: info Requires Home Assistant actions
Home Assistant-backed Action cards send Home Assistant actions from the panel. If tapping one of those cards does nothing, check [Enable Actions](/getting-started/home-assistant-actions). Local Action mode runs on the panel and does not need Home Assistant.
:::
