---
title: EspControl Screen Schedule
description:
  How to control what the EspControl panel does overnight.
---

# Screen Schedule

Screen schedule controls what the panel does overnight, then returns it to normal when the schedule says it should be available again.

You will find it in the **Settings** tab on the [Setup](/features/setup) page, under **Night Schedule**.

## Settings

- **Mode** - choose **Disabled**, **Time**, or **Sensor**. Time uses the daytime and night-time hours. Sensor uses the configured sensor entity: `on` puts the panel in normal mode, while `off` applies the selected night-time action.
- **Daytime** - the first hour when the screen should be awake. The default is **6:00 AM**.
- **Night Time** - the first hour when the night schedule starts. The default is **11:00 PM**.
- **At Night Time** - what the panel should do overnight. **Screen Off** is the default, **Screen Dimmed** keeps the panel usable at a set brightness, and **Clock** shows the clock instead. Screen Off can protect the LCD in the background while the backlight stays off.
- **When Woken, Idle Time to Screen Off** - shown only for **Screen Off**. It controls how long the screen stays awake after you tap it during scheduled-off hours. The default is **1 minute**.
- **When Woken, Screen Brightness** - shown only for **Screen Off**. It controls the brightness used for a temporary wake during scheduled-off hours. The default is **10%**.
- **Dimmed Screen Brightness** - shown only for **Screen Dimmed**. It controls the overnight brightness while the panel stays usable. The default is **10%**.
- **Clock Brightness** - shown only for **Clock**. It controls the backlight level used by the overnight clock. The default is **10%**.
- **Clock Text Color** - shown only for **Clock**. It controls the colour of the overnight schedule clock text.

Time and Sensor modes share the same night-time action and brightness settings. Switching between them keeps those settings. In Sensor mode, you can use a Home Assistant binary sensor directly, or an `input_boolean` controlled by an automation, as the **Sensor Entity**.

When the schedule is disabled, the panel uses the normal [screensaver](/features/screensaver) and [backlight](/features/backlight) rules.

Time-based Night Schedule has priority over screensaver presence wake while night time is active. If someone walks past during scheduled night time, the normal screensaver sensor does not wake the panel; tap wake still works. To make presence control night and daytime behavior, use **Sensor** mode for Night Schedule instead.

## How the Times Work

The on time is included, and the off time is not included. For example, **6:00 AM** to **11:00 PM** keeps the screen in normal use from 6:00 AM until just before 11:00 PM. At 11:00 PM, the selected night mode starts.

Overnight schedules also work. For example, **8:00 PM** to **7:00 AM** keeps the screen on through the night and turns it off during the day.

If the on and off times are the same, the schedule is treated as always on.

## Manual Wake and Sleep

Touching the screen while it is asleep, or while the schedule clock is showing, wakes it without changing the saved schedule. In **Screen Off** and **Clock** modes, a temporary wake outside the scheduled hours uses the saved wake brightness, stays awake for the saved wake idle time, then returns to the selected night mode.

Pressing and holding a button on the touchscreen for 3 seconds puts the screen to sleep manually. This is stronger than the schedule, so it will not immediately wake again just because the current time is inside the scheduled-on window. Tap the screen to wake it.

## Brightness

Screen schedule works alongside the daytime and nighttime brightness settings. When the screen is awake during scheduled-on hours, brightness follows the calculated sunrise and sunset for your selected timezone, or the manual Dawn and Dusk times when **Automatic Brightness** is off. **Screen Dimmed** uses its own overnight brightness setting. **Screen Off** turns the physical backlight off and can run invisible burn-in protection while dark, while **Clock** uses its own clock brightness and text colour settings.
