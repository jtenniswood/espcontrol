---
title: Date & Time Cards
description:
  How to show the date or local date and time on your EspControl panel.
---

# Date & Time

A date card can show either just the date, or the local date and time. In date-only mode, the large number shows the day and the label underneath shows the month. In date-and-time mode, the large number shows the local time and the label underneath shows the day and month.

Date cards are read-only — tapping them does nothing.

## Setting Up a Date Card

1. Select a card and change its type to **Date & Time**.
2. Choose **Date** or **Time & Date** from the **Type** dropdown.
3. Apply the configuration so the panel restarts with the new card.
4. On a **Large** card, turn on **Large Date / Time Numbers** if you want the main value scaled much larger.

## How It Works on the Panel

- In **Date only** mode, the card reads `sensor.date`, and it also falls back to the panel's own time source.
- In **Date & time** mode, the large time display follows the timezone and 12/24-hour setting selected in [Time Settings](/features/clock).
- The label underneath follows the same local timezone, so it stays matched to the time shown above.
- Month text comes from **Custom Month Names** in [Time Settings](/features/clock), if that advanced option is enabled.
- The panel publishes a **Screen: Date** diagnostic value, so you can check whether the device currently knows the date.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like Sensor, World Clock, and Weather cards.
- If the panel has not synced time yet, the card shows `--` until time becomes available.

The same **Date & Time** card settings also include **World Clock**. Choosing that changes the card into a [World Clock](/card-types/timezones) card for another city or timezone.
