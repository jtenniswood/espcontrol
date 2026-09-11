---
title: Local Action Cards
description:
  How to trigger custom on-device callbacks directly from EspControl cards without Home Assistant.
---

# Local Action

Local Action is an **Action** card mode that runs a callback on the ESP32. It works without Home Assistant, which is useful for IR, GPIO, UART, or other device-level jobs.

## Register the Action

Add a unique key, label, and callback to the device's `on_boot` lambda:

```yaml
esphome:
  on_boot:
    - priority: 700
      then:
        - lambda: |-
            register_local_action(
              "tv_off", "TV Off",
              [=]() { id(ir_blaster).transmit_nec(0x04FB, 0x08F7); }
            );
```

## Add the Card

1. Select a card and change its type to **Action**.
2. Choose **Local Action**.
3. Pick the registered action and choose an icon.

When the setup page can reach the panel, it lists registered actions. Otherwise enter the action key exactly as registered. Tapping the card flashes it briefly and runs the callback; a missing key is logged and does nothing.

Use [Trigger](/card-types/buttons) when Home Assistant should react to the tap, or another [Action](/card-types/actions) mode to run a Home Assistant action.
