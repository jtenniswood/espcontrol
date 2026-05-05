---
title: Action Cards
description:
  How to use action cards on your EspControl panel to run Home Assistant scenes, scripts, automations, buttons, and helpers.
---

# Action

An Action card is a simple one-tap shortcut. It sends a selected Home Assistant action when you tap it, but it does not show an on/off state.

Use Action cards for shortcuts such as running a scene, starting a script, triggering an automation, pressing a Home Assistant button entity, opening a lock latch, or changing a helper.

## Setting Up an Action Card

1. Select a card and change its type to **Action**.
2. Set a **Label** - this is the text shown on the card.
3. Choose an **Action**.
4. Enter the **Entity ID** for the thing you want the action to use.
5. If you choose **Set Number Helper** or **Select Option Helper**, enter the value or option.
6. Choose an **Icon**.

## Run an Existing Home Assistant Script

Use **Run Script** when you already have a Home Assistant script and want a card to run it directly. This avoids creating a separate automation just to connect the panel button to the script.

For example, to run a script called `script.mettre_de_la_musique`:

1. Set the card **Type** to **Action**.
2. Set **Action** to **Run Script**.
3. Set **Entity ID** to `script.mettre_de_la_musique`.
4. Set **Label** to the text you want on the panel, such as `Music`.
5. Choose an icon.

When you tap the card, EspControl sends `script.turn_on` to Home Assistant with that script as the target entity. The label is only what appears on the panel, so it can be different from the script name.

Action cards do not currently pass script variables or extra data. If a script needs inputs, handle those inside the Home Assistant script, or create a small wrapper script in Home Assistant and point the Action card at that wrapper.

## Supported Actions

| Action | Example entity | Extra field |
|---|---|---|
| **Run Scene** | `scene.movie_mode` | None |
| **Run Script** | `script.goodnight` | None |
| **Trigger Automation** | `automation.goodnight` | None |
| **Press Button** | `button.restart_router` | None |
| **Press Input Button** | `input_button.doorbell` | None |
| **Open Lock** | `lock.front_door` | None |
| **Toggle Helper** | `input_boolean.guest_mode` | None |
| **Set Number Helper** | `input_number.target_level` | Value |
| **Select Option Helper** | `input_select.house_mode` | Option |

## How It Works on the Panel

When you tap an Action card:

- The card briefly flashes the highlight colour.
- The selected Home Assistant action is sent with the configured entity.
- The card does not stay highlighted, because Action cards are stateless shortcuts.

## When to Use a Scene or Script

If you want a shortcut that does several things, create a scene or script in Home Assistant first, then point the Action card at that scene or script. This keeps the panel setup simple and makes the behaviour easier to test inside Home Assistant.

Use a script for locks that require a PIN or code. EspControl does not store lock codes on the panel.

Use an [Action](/card-types/actions) card when the panel should directly run something that already exists in Home Assistant. Use a [Trigger](/card-types/buttons) card when you want the panel to fire a custom event that a Home Assistant automation responds to.

Use the dedicated card types for richer controls:

- Use [Cover](/card-types/covers) for blinds, shutters, and covers.
- Use [Lock](/card-types/locks) for locking and unlocking doors.
- Use [Switch](/card-types/switches) or [Slider](/card-types/sliders) for lights.
- Thermostat and climate controls are experimental. They are included in release firmware, but must be enabled from the hidden Developer/Experimental Features setting first.
- Use [Media](/card-types/media) for media playback, volume popups, and track position. Media cards are currently behind **Developer/Experimental Features**.

::: info Requires Home Assistant actions
Action cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
