---
title: Date & Time Cards
description:
  How to show the local clock, date, or date and time on your EspControl panel.
---

# Date & Time

A date and time card can show the local clock, just the date, or the local date and time. In clock mode, only the time is shown. In date-only mode, the large number shows the day and the label underneath shows the month. In date-and-time mode, the large number shows the local time and the label underneath shows the day and month.

Date and clock cards are read-only — tapping them does nothing.

## Setting Up a Date or Clock Card

1. Select a card and change its type to **Date & Time**.
2. Choose **Clock**, **Date**, or **Time & Date** from the **Type** dropdown.
3. On a **Wide** or **Large** card, turn on **Large Clock** if you want the main value scaled much larger. On a **Wide** card, the label underneath is hidden so the larger value has room.
4. Apply the configuration so the panel restarts with the new card.

## How It Works on the Panel

- In **Clock** mode, the card uses the panel's own synced time source and shows no date label.
- In **Date only** mode, the card reads `sensor.date`, and it also falls back to the panel's own time source.
- In **Date & time** mode, the large time display follows the timezone and 12/24-hour setting selected in [Time Settings](/features/clock).
- The label underneath follows the same local timezone, so it stays matched to the time shown above.
- Month text comes from **Custom Month Names** in [Time Settings](/features/clock), if that advanced option is enabled.
- The panel publishes a **Screen: Date** diagnostic value, so you can check whether the device currently knows the date.
- The card uses the fixed **tertiary** background colour, like Sensor, World Clock, and Weather cards.
- If the panel has not synced time yet, the card shows `--` until time becomes available.

The same **Date & Time** card settings also include **World Clock**. Choosing that changes the card into a [World Clock](/card-types/timezones) card for another city or timezone.

## Agenda Card

The Agenda card lists your next upcoming calendar event, fetched from one or
more Home Assistant calendars. Enter one calendar entity, or several separated
by commas (for example `calendar.family, calendar.work`). The tile shows the
next event's time (or day, for events beyond today) and its title, refreshing
every ten minutes.

Pick each calendar from the list in the card's settings and give it a colour;
its events carry that colour on the panel. **Days Ahead** sets how far forward
the card looks (1 to 30 days, 14 by default), and **Hide Empty Days** leaves
days with nothing scheduled out of the full view.

Tapping the card opens that full view: every day in the window, one after
another, scrolling.

The same upcoming-events list can also be shown on the [photo
screensaver](/features/screensaver) as an overlay.
