---
title: Switch Cards
description:
  How to use switch cards on your EspControl panel to control Home Assistant entities.
---

# Switch

A Switch card is the default on/off card. It controls one Home Assistant entity from the touchscreen and shows whether that entity is currently active.

Use Switch cards for common Home Assistant entities such as lights, switches, fans, media players, covers, and button entities. The entity needs to support a Home Assistant toggle or button press action to respond when tapped.

Use a [Lock](/card-types/locks) card for door locks. Lock cards use Home Assistant's lock and unlock actions directly and avoid unlocking when the current state is unknown.

![Switch card showing a Heater icon](/images/card-toggle.png)

## Setting Up a Switch Card

1. Select a card and change its type to **Switch**. New cards use **Switch** by default.
2. Enter an **Entity ID** - the Home Assistant entity you want to control, for example `light.kitchen` or `switch.garden_lights`.
3. Set a **Label** if you want custom text on the card. If left blank, the friendly name from Home Assistant is used.
4. Choose an **Off Icon**, or leave it as **Auto** so the panel picks an icon from the entity type.
5. Choose an **On Icon** if you want a different icon while the entity is active.
6. Optionally turn on **Show sensor data when on** if the card should show a live sensor value or text state while active.

## How It Works on the Panel

- Tapping most Switch cards sends a Home Assistant toggle action for the entity.
- If the entity starts with `button.`, tapping the card sends a button press instead.
- The card lights up when Home Assistant reports an active state such as `on`, `open`, `opening`, `closing`, `playing`, `home`, or `unlocked`.
- If the entity is changed somewhere else, such as in Home Assistant or by an automation, the card updates to match.

## Active State Display

Switch cards always have separate **Off Icon** and **On Icon** settings. The on icon is used while Home Assistant reports the entity as active.

Switch cards can also show sensor data while the entity is active:

- **Numeric** - show a live sensor value instead of the icon. You can set the sensor entity, unit, and decimal precision.
- **Text** - show a live text state instead of the card label, preserving capital letters and line breaks.

When the entity is not active, the card goes back to its normal off icon and label.

::: info Requires Home Assistant actions
Switch cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
