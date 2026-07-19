---
title: EspControl Backlight Settings
description:
  How the EspControl panel automatically adjusts screen brightness during the day and night based on sunrise and sunset.
---

# Backlight

The panel automatically adjusts screen brightness based on time of day — brighter during daylight, dimmer at night.

## How It Works

Sunrise and sunset times are calculated on-device from your selected timezone using a NOAA solar algorithm. During the day, the panel uses your **Daytime Brightness**; at night, it switches to **Nighttime Brightness**. The transition is checked every 60 seconds, and sunrise/sunset are recalculated at midnight. No internet connection or Home Assistant is required.

## Settings

Configured in the **Brightness** section of the **Settings** tab in [Setup](/features/setup).

- **Daytime Brightness** — screen brightness during the day (1%–100%, default 100%).
- **Nighttime Brightness** — screen brightness at night (1%–100%, default 75%).
- **Automatic Brightness** — when enabled, EspControl uses the calculated sunrise and sunset times. Turn it off to set manual **Dawn** and **Dusk** times for the day/night brightness changeover.
- **Dawn / Dusk** — shown when **Automatic Brightness** is off. These manual times decide when the panel switches between **Daytime Brightness** and **Nighttime Brightness**.

Sunrise and sunset times are derived from the timezone set in [Time Settings](/features/clock).

## Battery Saver

Choose **Settings > System > Power > Battery Saver** when the panel is running from a battery. Battery Saver keeps the panel connected to Home Assistant, but lets the Wi-Fi radio use light/modem sleep and defers image-card downloads and other display-only work while the display is off.

Battery Saver does not change any brightness, screensaver, schedule, voice-service, or relay setting. You remain in control of every brightness level from 1% to 100%. For the largest saving, combine Battery Saver with a short screensaver timeout and **Display Off** rather than a dimmed screen or clock.

Voice processing and an energised relay can use considerably more power than the sleeping Wi-Fi radio, so turn those features off separately when they are not required. Switching back to **Normal** restores the original responsive Wi-Fi behaviour without changing the saved display settings.

## Home Assistant Control

The panel exposes **Screen: Power Mode**, **Screen: Automatic Brightness**, **Screen: Brightness Dawn Time**, and **Screen: Brightness Dusk Time** to Home Assistant. Turn **Screen: Automatic Brightness** off when you want fixed manual dawn and dusk times instead of the calculated sunrise and sunset times.

For example, you can set Dawn to `07:00` and Dusk to `22:00` if you want the panel to stay in daytime brightness for a fixed daily window. Turning automatic brightness back on returns the panel to the calculated sunrise and sunset times for the selected timezone.

## Screensaver

When the screensaver uses **Screen Dimmed**, it keeps the normal screen visible at the saved dim brightness. When the screensaver clock is active, it can use separate daytime and nighttime clock brightness values based on the same automatic or manual day/night times. If the screensaver is set to Display Off, the backlight turns off completely. While the backlight is off, EspControl can exercise the LCD pixels in the background to reduce burn-in risk without showing that pattern. On wake (touch or presence sensor), brightness returns to the correct level for the current time.

## Screen Schedule

The [screen schedule](/features/screen-schedule) can turn the physical backlight off, keep the panel dimmed, or show a clock at set hours. **Screen Off** uses the schedule's separate **When Woken** brightness during a temporary wake and can run the same invisible burn-in protection while dark. **Screen Dimmed** uses its own overnight brightness setting. **Clock** uses its own clock brightness setting.

## Before Clock Sync

If the panel hasn't synced its clock yet, it defaults to daytime brightness. Once synced, sunrise and sunset are calculated immediately.
