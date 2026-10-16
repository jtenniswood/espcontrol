---
title: "Night Schedule"
description: Set EspControl overnight screen behaviour using fixed hours or a Home Assistant sensor, with temporary touch wake.
---

# Night Schedule

Open **Screen > Sleep & Schedule > Night Schedule** on your [device setup page](/features/setup).

## Set the Schedule

1. Choose **Mode > Time** for fixed hours, or **Sensor** to follow a Home Assistant sensor. **Disabled** leaves normal screensaver and backlight rules in charge.
2. In **Time**, set **Daytime** (normal use starts) and **Night Time** (night behaviour starts). For example, `06:00` and `23:00` keep normal use from 6 AM until 11 PM. The window can cross midnight. Equal times keep normal mode on all day.
3. In **Sensor**, enter **Sensor Entity** and choose **Activate Night Schedule When > Sensor Is Off** or **Sensor Is On**. The opposite state returns to normal use.
4. Choose **At Night Time** from the table below.
5. Choose **Apply Configuration**, then test the boundary or change the sensor state.

| Night action | What to set |
|---|---|
| **Screen Off** | **When Woken, Idle Time to Screen Off** and **When Woken, Screen Brightness** for temporary wakes. |
| **Screen Dimmed** | **Dimmed Screen Brightness** for overnight use. |
| **Clock** | **Clock Brightness** and **Clock Text Colour**. |

## How It Works with Other Settings

The schedule's sensor is separate from the Screensaver **Presence Entity**. Time-based night hours take priority over screensaver presence wake and media cover art. If presence should control night behaviour, use **Sensor** mode here.

Normal-use hours follow your [Backlight](/features/backlight) and [Screensaver](/features/screensaver) settings. When a scheduled Clock period ends, an idle panel goes directly to its normal screensaver action.

Enable **Screen > Appearance > Clock Bar > Show Night Mode Icon** to see a moon while night mode is active.

## Manual Wake and Sleep

Touch the sleeping panel or press its **Screen: Wake** button in Home Assistant. Screen Off and Clock modes temporarily wake using the saved wake brightness and idle time, then return to night behaviour. Waking does not change the schedule. [PIN protection](/features/screensaver#pin-protection-after-wake) still applies.

Press and hold a touchscreen button for three seconds to sleep manually. It stays asleep even during normal-use hours until you wake it. See [Wake from Home Assistant](/features/screensaver#wake-from-home-assistant) for an automation example.
