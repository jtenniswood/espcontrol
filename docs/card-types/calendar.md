---
title: Date Cards
description:
  How to show today's date on your EspControl panel.
---

# Date

A date card can show either just the date, or the local date and time. In date-only mode, the large number shows the day and the label underneath uses the date text from the date source. In date-and-time mode, the large number shows the local time and the label underneath uses the same date source text.

Date cards are read-only — tapping them does nothing.

## Setting Up a Date Card

1. Select a card and change its type to **Date**.
2. Choose **Date only** or **Date & time** from the **Display** dropdown.
3. Apply the configuration so the panel restarts with the new card.

## How It Works on the Panel

- In **Date only** mode, the card reads `sensor.date`, and it also falls back to the panel's own time source.
- In **Date & time** mode, the large time display follows the timezone and 12/24-hour setting selected in [Time Settings](/features/clock).
- The label underneath comes from the date source text, such as `sensor.date` or the panel's own **Screen: Date** diagnostic value.
- The panel publishes a **Screen: Date** diagnostic value, so you can check whether the device currently knows the date.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like Sensor, World Clock, and Weather cards.
- If the panel has not synced time yet, the card shows `--` until time becomes available.
