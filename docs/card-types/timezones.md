---
title: World Clock Cards
description:
  How to show the current time for another city on your EspControl panel.
---

# World Clock

A world clock card shows the current local time for a selected city. The large value is the time, and the label underneath is the city name.

World clock cards are read-only — tapping them does nothing. They do not need a Home Assistant entity; the panel uses its own synced time source.

## Setting Up a World Clock Card

1. Select a card and change its type to **Date & Time**.
2. Set **Type** to **World Clock**.
3. Choose the city/timezone from the **City / Timezone** dropdown.
4. Apply the configuration so the panel restarts with the new card.

## How It Works on the Panel

- The dropdown uses the same city/timezone list as the main [Time Settings](/features/clock) setting.
- The card follows the panel's 12-hour or 24-hour clock format.
- The time updates once per minute from the panel's own synced time source.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like Sensor, Date, and Weather cards.
- If the panel has not synced time yet, the card shows `--:--` until time becomes available.
- Older backups or logs may still refer to this as `timezone`. That is the internal saved name for the same **World Clock** card, kept so existing cards continue to load.

::: tip Firmware support
The setup page and the panel firmware both need to know about this card type. If the setup page lets you create a **World Clock** card but the panel itself does not show the clock after applying the configuration, install the latest [firmware update](/features/firmware-updates).
:::
