---
title: EspControl Backlight Settings
description:
  Choose manual backlight control or automatic day and night brightness using sunrise, sunset, or fixed times.
---

# Backlight

The panel can keep one brightness level under manual control, or automatically use separate daytime and nighttime levels.

## How It Works

The **Brightness Mode** setting gives you three clear choices:

- **Manual** — keeps the brightness level you choose.
- **Sunrise and sunset** — uses your **Daytime Brightness** and **Nighttime Brightness**, changing at locally calculated sunrise and sunset.
- **Fixed times** — uses the same day and night levels, changing at the **Dawn** and **Dusk** times you choose.

Sunrise and sunset are calculated on-device from your selected timezone using a NOAA solar algorithm. The transition is checked every 60 seconds and recalculated at midnight. No internet connection or Home Assistant is required.

## Settings

Configured in the **Brightness** section of the **Settings** tab in [Setup](/features/setup).

- **Brightness Mode** — choose **Manual**, **Sunrise and sunset**, or **Fixed times**.
- **Brightness** — shown in Manual mode and sets the normal screen brightness (1%–100%).
- **Daytime Brightness** — shown in either automatic mode and sets the daytime level (10%–100%, default 100%).
- **Nighttime Brightness** — shown in either automatic mode and sets the nighttime level (10%–100%, default 75%).
- **Dawn / Dusk** — shown in Fixed times mode and decides when the panel switches between the day and night levels.

Sunrise and sunset times are derived from the timezone set in [Time Settings](/features/clock).

## Home Assistant Control

The panel exposes **Screen: Brightness Mode** and **Display Backlight** to Home Assistant, along with the day, night, dawn, and dusk settings. Selecting the mode in Home Assistant behaves the same as selecting it in Setup.

Changing the brightness of **Display Backlight** automatically selects **Manual** mode. Turning the backlight on or off does not change the selected mode, so existing sleep and wake automations continue to work.

For example, select **Fixed times**, then set Dawn to `07:00` and Dusk to `22:00` to keep the panel at daytime brightness for that daily window. Select **Sunrise and sunset** to return to locally calculated times.

## Screensaver

When the screensaver uses **Screen Dimmed**, Manual mode keeps the normal screen visible at its saved dim brightness. Automatic and Timed modes use their own daytime and nighttime dimmed-screen levels and change at the same boundary as the main backlight. When the screensaver clock is active, it can use separate daytime and nighttime clock brightness values. If the screensaver is set to Display Off, the backlight turns off completely. While the backlight is off, EspControl can exercise the LCD pixels in the background to reduce burn-in risk without showing that pattern. On wake, brightness returns to the saved Manual level or the correct automatic level for the current time.

## Screen Schedule

The [screen schedule](/features/screen-schedule) can turn the physical backlight off, keep the panel dimmed, or show a clock at set hours. **Screen Off** uses the schedule's separate **When Woken** brightness during a temporary wake and can run the same invisible burn-in protection while dark. **Screen Dimmed** uses its own overnight brightness setting. **Clock** uses its own clock brightness setting.

## Before Clock Sync

If the panel hasn't synced its clock yet, it defaults to daytime brightness. Once synced, sunrise and sunset are calculated immediately.
