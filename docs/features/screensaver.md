---
title: Screensaver
description:
  How to configure screensaver modes and presence sensor wake on your Espcontrol panel.
---

# Screensaver

To save power and protect the display, the panel activates a screensaver when it's not being used. The screen shows a gentle snow animation and the backlight turns off. Touch the screen to wake it up.

There are two ways to control the screensaver, configured in the **Settings** tab on the [Setup](/features/setup) page:

## Timer

The screensaver turns on after the panel hasn't been touched for a set amount of time. Choose from:

- 1 minute
- **5 minutes** (the default)
- 10, 15, 20, 30, or 45 minutes
- 1 hour

### Clock or Display Off

When the screensaver activates, you can choose what happens:

- **Clock** — shows a large drifting clock at reduced brightness (the default). The clock repositions itself periodically to prevent burn-in.
- **Display Off** — turns the backlight off completely.

## Sensor

Instead of a timer, the screensaver is controlled by a motion or presence sensor (like a mmWave sensor mounted nearby). When someone is in the room, the screen stays on. When nobody is detected, the screen goes to sleep — and wakes up again when someone walks past.

To use this, enter the name of your motion or presence sensor from Home Assistant (for example, `binary_sensor.hallway_presence`).

::: tip
Touching the screen always wakes it up, no matter which mode you're using.
:::
