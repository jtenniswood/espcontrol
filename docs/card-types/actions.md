---
title: Action Cards
description:
  How to use action cards on your EspControl panel to run Home Assistant scenes, scripts, automations, buttons, and helpers.
---

# Action

An Action card is a simple one-tap shortcut. It sends a selected Home Assistant action, opens an Option Select picker, or runs a registered local device action when you tap it.

Use Action cards for shortcuts such as running a scene, starting a script, triggering an automation, pressing a Home Assistant button entity, changing a helper, or calling a custom action registered on the panel itself.

## Setting Up an Action Card

1. Select a card and change its type to **Action**.
2. Set a **Label** - this is the text shown on the card.
3. Choose an **Action**.
4. Enter the **Entity** for the thing you want the action to use.
5. If you choose **Set Number Helper**, enter the value.
6. Choose an **Icon**.
7. If you choose **Run Script**, optionally turn on **Confirmation Required** if accidental taps would be a problem.
8. Optionally turn on **Show State** if a Home Assistant-backed Action card should show a separate Home Assistant state.

## Run an Existing Home Assistant Script

Use **Run Script** when you already have a Home Assistant script and want a card to run it directly. This avoids creating a separate automation just to connect the panel button to the script.

For example, to run a script called `script.mettre_de_la_musique`:

1. Set the card **Type** to **Action**.
2. Set **Action** to **Run Script**.
3. Set **Entity** to `script.mettre_de_la_musique`.
4. Set **Label** to the text you want on the panel, such as `Music`.
5. Choose an icon.

When you tap the card, EspControl sends `script.turn_on` to Home Assistant with that script as the target entity. The label is only what appears on the panel, so it can be different from the script name.

For safety-sensitive scripts, turn on **Confirmation Required**. The card will open the same confirmation popup used by Switch cards before it sends `script.turn_on`. You can customise the popup message and the confirm/cancel button text.

Action cards do not currently pass script variables or extra data. If a script needs inputs, handle those inside the Home Assistant script, or create a small wrapper script in Home Assistant and point the Action card at that wrapper.

## Supported Actions

| Action | Example entity | Extra field |
|---|---|---|
| **Run Scene** | `scene.movie_mode` | None |
| **Run Script** | `script.goodnight` | None |
| **Trigger Automation** | `automation.goodnight` | None |
| **Press Button** | `button.restart_router` | None |
| **Press Input Button** | `input_button.doorbell` | None |
| **Toggle Helper** | `input_boolean.guest_mode` | None |
| **Set Number Helper** | `input_number.target_level` | Value |
| **Option Select** | `select.wled_preset` or `input_select.house_mode` | Opens option list |
| **Local Action** | Registered local action key | Runs on the panel |

## Option Select

Choose **Option Select** inside the Action card when you want the panel to show and change a live `select` or `input_select` value.

When you tap the card, EspControl opens the option list reported by Home Assistant. Choosing an option sends `select.select_option` for `select` entities or `input_select.select_option` for `input_select` helpers.

This is useful for WLED presets, room modes, house modes, and similar helpers where you want to pick from the current list rather than hard-code one option into the card.

## Local Action

Choose **Local Action** inside the Action card when the action should run directly on the ESP32 instead of going through Home Assistant.

Local actions are registered in your device's `on_boot` lambda using `register_local_action()`. The key must be unique per device and is used to match the card to the callback.

```yaml
esphome:
  on_boot:
    - priority: 700
      then:
        - lambda: |-
            register_local_action(
              "tv_off",
              "TV Off",
              [=]() { id(ir_blaster).transmit_nec(0x04FB, 0x08F7); }
            );
```

When the setup page can reach the device, it shows a dropdown of registered local actions. If the device cannot be reached, enter the **Action Key** manually.

## Show State

Action cards are normally stateless: they flash when tapped, then return to their normal colour.

Turn on **Show State** when an action should behave like a shortcut but still show whether something is active. For example, an Action card might run a scene called `scene.movie_mode`, while **State Entity** watches `input_boolean.movie_mode`.

Show State has three display modes:

- **Icon** — keeps the normal action icon, and can show a separate **On Icon** while the state entity is active.
- **Numeric** — shows a live sensor value, with optional **Unit**, **Unit Precision**, and **Large State Numbers** on larger cards.
- **Text** — shows the live state text where the card label normally appears.

When the state entity is active, Icon mode highlights the card. If the state entity is unavailable, the card is disabled until Home Assistant reports it as available again. The action target itself stays tappable because many Home Assistant command-only entities, including `button.*` entities, do not report a useful on/off state.

## How It Works on the Panel

When you tap an Action card:

- The card briefly flashes the highlight colour.
- The selected Home Assistant action is sent with the configured entity. Run Script cards with **Confirmation Required** ask first.
- Local Action cards run the registered callback on the panel itself.
- If **Show State** is off, the card does not stay highlighted.
- If **Show State** is on, the card display follows the state entity you chose.

## When to Use a Scene or Script

If you want a shortcut that does several things, create a scene or script in Home Assistant first, then point the Action card at that scene or script. This keeps the panel setup simple and makes the behaviour easier to test inside Home Assistant.

Use a script for locks that require a PIN or code. EspControl does not store lock codes on the panel.

Use an [Action](/card-types/actions) card when the panel should directly run something that already exists in Home Assistant, or when it should run a registered local panel action. Use a [Webhook](/card-types/webhooks) card when the panel should call a URL directly without Home Assistant. Use a [Trigger](/card-types/buttons) card when you want the panel to fire a custom event that a Home Assistant automation responds to.

Use the dedicated card types for richer controls:

- Use [Cover](/card-types/covers) for blinds, shutters, and covers.
- Use [Lock](/card-types/locks) for locking and unlocking doors.
- Use [Lights](/card-types/lights) for light switching, brightness, and colour temperature.
- Use [Media](/card-types/media) for media player playback, volume, and now-playing controls.
- Use [Climate](/card-types/climate) for thermostat and HVAC controls.
- Use [Vacuum](/card-types/vacuum) for robot vacuum status and cleaning controls.

::: info Requires Home Assistant actions
Home Assistant-backed Action cards send Home Assistant actions from the panel. If tapping one of those cards does nothing, check [Enable Actions](/getting-started/home-assistant-actions). Local Action mode runs on the panel and does not need Home Assistant.
:::
