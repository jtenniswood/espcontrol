---
title: Internal Switches Cards
description:
  How to control built-in panel relays locally from EspControl cards without relying on Home Assistant.
---

# Internal Switches

Internal Switches cards control the panel's built-in relay hardware directly from the touchscreen. They do not send a command through Home Assistant, so they can still work when Home Assistant is offline.

This card type appears on devices that define built-in relays, such as the 4848S040 relay variant and the Waveshare ESP32-P4-86-Panel-ETH-2RO.

## Setting Up an Internal Switches Card

1. Select a card and change its type to **Internal Switches**.
2. Set a **Label** if you want custom text. If left blank, the selected relay name is used.
3. Choose the **Internal Relay** to control.
4. Choose a **Mode**.
5. Choose the icon fields for that mode.

## Modes

**Switch** mode behaves like a normal Switch card. Tapping it turns the selected relay on or off, and the card shows the current relay state.

Switch mode shows two icon fields:

- **On Icon** defaults to **Lightbulb**.
- **Off Icon** defaults to **Lightbulb Outline**.

**Push Button** mode behaves like a Trigger card. Tapping it pulses the selected relay for 200 ms, then turns it off again.

Push Button mode shows one **Icon** field, because it does not have a separate on/off state.

## When to Use It

Use Internal Switches cards when the panel itself should operate the connected circuit, even during a Home Assistant outage. Use normal [Switch](/card-types/switches) cards with Home Assistant relay entities when you specifically want the command to go through Home Assistant automations or permissions.
