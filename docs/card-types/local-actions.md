---
title: Local Action Cards
description:
  How to trigger custom on-device callbacks directly from EspControl cards without going through Home Assistant.
---

# Local Action

A local action card calls a callback registered directly on the device when tapped. The action runs immediately on the ESP32 — no Home Assistant involved, no network round-trip.

Local action cards are useful for anything that needs to run at the device level: pulsing GPIO pins, transmitting IR codes, sending UART commands, or calling any custom ESPHome component.

::: tip Works without Home Assistant
Because the action runs on the device itself, local action cards work even when Home Assistant is offline or unreachable.
:::

## Setting Up a Local Action Card

1. Select a card and change its type to **Local Action**.
2. Set a **Label** — this is shown on the card.
3. Set an **Action Key** — this must match the key you register in your device config.
4. Choose an **Icon** (defaults to a tap gesture icon).

## Registering the Action on the Device

Local actions are registered in your device's `on_boot` lambda using `register_local_action()`. The first argument is the key that must match the **Action Key** set in the web UI.

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

You can register as many actions as you need:

```yaml
        - lambda: |-
            register_local_action(
              "tv_off",      "TV Off",      [=]() { id(ir_blaster).transmit_nec(0x04FB, 0x08F7); });
            register_local_action(
              "tv_vol_up",   "Volume Up",   [=]() { id(ir_blaster).transmit_nec(0x04FB, 0x58A7); });
            register_local_action(
              "unlock_door", "Unlock Door", [=]() { id(door_relay).turn_on(); });
```

## How It Works on the Panel

When you tap a local action card:

- The card flashes the highlight colour and fades back, the same visual behaviour as a Trigger card.
- The registered callback runs immediately on the device.
- If no action is registered for the key, a warning is logged and nothing else happens.

## Action Key Tips

- Keys are case-sensitive strings matched at runtime — `tv_off` and `TV_Off` are different keys.
- If a card's key doesn't match any registered action, the device logs a warning but does not crash.
- You can re-register an action with the same key to replace it — the latest registration wins.

## Example Use Cases

| Action Key | What it does |
|---|---|
| `tv_off` | IR blaster command to turn off a TV |
| `unlock_door` | Pulse a GPIO relay to trigger a door latch |
| `projector_on` | UART command to a projector or AV receiver |
| `wake_pc` | Wake-on-LAN magic packet to a computer |
| `night_mode` | Run an ESPHome script that sets multiple things at once |

## When to Use It

Use local action cards when the action is handled entirely by the device and does not need Home Assistant. Use [Trigger](/card-types/buttons) cards when you want Home Assistant to react to the button press, or [Action](/card-types/actions) cards when you want to call a Home Assistant service directly.
