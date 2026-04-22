---
title: Screensaver
description:
  How to configure screensaver modes and presence sensor wake on your Espcontrol panel.
---

# Screensaver

The panel can use a screensaver when it's not being used. When active, it can show a dim clock or turn the backlight off so the panel goes dark. Touch the screen to wake it up.

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

### Clock or Display Off

When the screensaver activates, you can choose what happens:

- **Clock** — shows a large drifting clock at reduced brightness (the default). The clock repositions itself periodically to prevent burn-in.
- **Display Off** — switches to a black screen and turns the backlight off completely.

When Clock is selected, you can set separate **Daytime Clock Brightness** and **Nighttime Clock Brightness** values. These use the same sunrise and sunset calculation as the main screen brightness.

## Sensor

Instead of a timer, the screensaver is controlled by a motion or presence sensor (like a mmWave sensor mounted nearby). When someone is in the room, the screen stays on. When nobody is detected, the screen goes to sleep — and wakes up again when someone walks past.

To use this, enter the name of your motion or presence sensor from Home Assistant (for example, `binary_sensor.hallway_presence`).

Below the presence entity, use **Then** to choose whether the panel shows the clock or turns the display off when nobody is detected. This uses the same Clock or Display Off options as Timer mode.

Switching back to Timer keeps the sensor name saved, so you can return to Sensor mode later without typing it in again.

::: tip
Touching the screen always wakes it up, no matter which mode you're using.
:::

## Screen Schedule

The [screen schedule](/features/screen-schedule) is separate from the screensaver. Use it when you want the panel to be fully dark during fixed hours, such as overnight.
