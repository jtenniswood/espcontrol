---
title: Screen Schedule
description:
  How to turn the Espcontrol panel screen on and off automatically at set hours.
---

# Screen Schedule

Screen schedule lets the panel go fully dark during hours when you do not need it, then wake automatically when the schedule says it should be available again.

You will find it in the **Settings** tab on the [Setup](/features/setup) page, under **Screen Schedule**.

## Settings

- **Schedule Screen Off** - turns automatic screen sleep and wake on or off.
- **On Time** - the first hour when the screen should be awake. The default is **6:00 AM**.
- **Off Time** - the first hour when the screen should be asleep. The default is **11:00 PM**.
- **When Woken, Idle Time to Screen Off** - how long the screen stays awake after you tap it during the scheduled-off hours. The default is **1 minute**.
- **When Woken, Screen Brightness** - the brightness used only for a temporary wake during scheduled-off hours. The default is **10%**.

When the schedule is disabled, the on and off time controls are hidden and the panel uses the normal [screensaver](/features/screensaver) and [backlight](/features/backlight) rules.

## How the Times Work

The on time is included, and the off time is not included. For example, **6:00 AM** to **11:00 PM** keeps the screen on from 6:00 AM until just before 11:00 PM. At 11:00 PM, the screen turns off.

Overnight schedules also work. For example, **8:00 PM** to **7:00 AM** keeps the screen on through the night and turns it off during the day.

If the on and off times are the same, the schedule is treated as always on.

## Manual Wake and Sleep

Touching the screen while it is asleep wakes it without changing the saved schedule. If you wake it outside the scheduled hours, it uses the **When Woken** brightness, stays awake for the **When Woken** idle time, then the schedule puts it back to sleep again.

Pressing and holding a button on the touchscreen for 3 seconds puts the screen to sleep manually. This is stronger than the schedule, so it will not immediately wake again just because the current time is inside the scheduled-on window. Tap the screen to wake it.

## Brightness

Screen schedule works alongside the daytime and nighttime brightness settings. When the screen is awake during scheduled-on hours, brightness still follows sunrise and sunset for your selected timezone. A temporary wake during scheduled-off hours uses **When Woken, Screen Brightness** instead. When the screen is asleep, the physical backlight is off.
