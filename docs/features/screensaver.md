---
title: EspControl Screensaver
description:
  How to configure screensaver modes and presence sensor wake on your EspControl panel.
---

# Screensaver

The panel can use a screensaver when it's not being used. When active, it can dim the normal screen, show a dim clock, or turn the backlight off so the panel goes dark. Touch the screen to wake it up.

There are three screen saver modes, configured in the **Settings** tab on the [Setup](/features/setup) page:

## Disabled

The screensaver does not run automatically. This is the default setting.

## Timer

The screensaver turns on after the panel hasn't been touched for a set amount of time. Choose from:

- 10 seconds
- 30 seconds
- 1 minute
- **5 minutes** (the default)
- 10, 15, 20, 30, or 45 minutes
- 1 hour

If the 10 or 30 second choices are not shown, update the panel firmware first. The web page checks what range the installed firmware supports before showing the shorter timer values.

### What Happens

When the screensaver activates, you can choose what happens:

- **Screen Dimmed** — keeps the normal screen visible, but lowers the backlight. The first tap wakes the screen instead of pressing a card.
- **Clock** — shows a large drifting clock at reduced brightness (the default). The clock repositions itself periodically to prevent burn-in.
- **Display Off** — switches to a black screen and turns the backlight off completely. While the backlight is off, EspControl can exercise the LCD pixels in the background to reduce burn-in risk; this should not be visible.

When Screen Dimmed is selected, set **Dimmed Screen Brightness**. When Clock is selected, set separate **Daytime Clock Brightness** and **Nighttime Clock Brightness** values. Clock brightness uses the same sunrise and sunset calculation as the main screen brightness.

## PIN Protection After Wake

In **Setup → Settings → Screensaver**, turn on **Require PIN after wake** and enter a numeric PIN. The PIN is stored on that panel. It is not configured in YAML or through a Home Assistant helper.

After the panel wakes from **Display Off**, **Screen Dimmed**, **Clock**, presence-controlled sleep, or manual sleep, a numeric keypad appears with the digits in a new random order. Normal touchscreen controls remain blocked until the correct PIN is entered. A wrong PIN clears the entry and leaves the panel locked.

Clearing the PIN disables wake locking even if the switch remains on. The PIN itself is not included in normal configuration backups; after importing a backup, set a new PIN on the destination panel.

::: warning Security limit
PIN protection blocks local touchscreen controls. It does not protect the device's web setup page, network access, Home Assistant access, or other administration paths. Secure those separately on your network.
:::

## Start the Screensaver from a Card

Add a local [Screensaver card](/card-types/screensaver) to start the configured screensaver immediately from the home screen or a subpage. The card follows the same **Display Off**, **Screen Dimmed**, or **Clock** choice used by automatic screensaver modes and needs no Home Assistant entity.

## Sensor

Instead of a timer, the screensaver is controlled by a motion or presence sensor (like a mmWave sensor mounted nearby). When someone is in the room, the screen stays on. When nobody is detected, the screen goes to sleep — and wakes up again when someone walks past.

To use this, enter the name of your motion or presence sensor from Home Assistant (for example, `binary_sensor.hallway_presence`).

Below the presence entity, use **Then** to choose whether the panel dims the screen, shows the clock, or turns the display off when nobody is detected. This uses the same options as Timer mode.

Presence wakes the panel from those dimmed, clock, or display-off states. When the normal cards or media cover art are already visible, presence does not change the page or restart the cover-art timer.

Switching back to Timer keeps the sensor name saved, so you can return to Sensor mode later without typing it in again.

::: tip
Touching the screen always wakes it up, no matter which mode you're using.
:::

## Screen Schedule

The [screen schedule](/features/screen-schedule) is separate from the screensaver. Use it when you want the panel to be fully dark, dimmed, or showing a clock overnight.

When Night Schedule is using fixed **Time** hours, it has priority over screensaver sensor wake during night time. The screensaver presence sensor still keeps the panel awake and wakes it during normal daytime operation, but it does not override scheduled night time. Tap wake still works, and uses the temporary wake settings from the screen schedule.

If you want presence to control when the panel is in night mode, set Night Schedule to **Sensor** mode instead of **Time** mode.
