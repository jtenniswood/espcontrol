---
title: "Backlight Settings"
description: Set a fixed EspControl screen brightness or separate day and night levels using sunrise/sunset or fixed times.
---

# Backlight

Open **Screen > Display > Backlight** on your [device setup page](/features/setup).

1. Choose a mode from the table below.
2. Set the visible brightness sliders (10%–100%).
3. For **Timed**, enter **Dawn** and **Dusk**. For **Automatic**, check the displayed sunrise and sunset times and your [timezone](/features/clock).

| Mode | What to set |
|---|---|
| **Manual** | One **Brightness** level. |
| **Automatic** | **Daytime Brightness** and **Nighttime Brightness**, switching at locally calculated sunrise and sunset. |
| **Timed** | The same day/night levels, switching at your **Dawn** and **Dusk** times. |

For example, Dawn `07:00` and Dusk `22:00` use the daytime level between those times. Before the clock syncs, the panel uses daytime brightness.

## Sleep Brightness

The [screensaver](/settings/sleep#screensaver) and [Night Schedule](/settings/sleep#night-schedule) have their own dimmed-screen and clock brightness settings. **Display Off / Screen Off** turns the backlight off completely. Waking normally restores the appropriate brightness; a temporary wake during Night Schedule uses its saved wake settings.

## Home Assistant Control

Use **Screen: Brightness Mode** and **Display Backlight** in Home Assistant. Its **Sunrise and sunset** and **Fixed times** options correspond to **Automatic** and **Timed** in web setup.

Changing Display Backlight's brightness selects Manual mode. Turning it on or off keeps the selected mode, so sleep/wake automations can still use automatic brightness.
