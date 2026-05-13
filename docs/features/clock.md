---
title: EspControl Time Settings
description:
  How to configure clock sync, timezone, and 12/24-hour format on your EspControl panel.
---

# Time Settings

The panel can display a clock in the top bar, updated every minute from network time. You can choose your timezone, switch between 12-hour and 24-hour format, set custom NTP servers, and adjust the month text used by Date cards.

## Settings

Configured in the **Time Settings** section of the **Settings** tab in [Setup](/features/setup).

- **Timezone** — select your timezone from the dropdown. This also determines sunrise and sunset times used by the [backlight schedule](/features/backlight).
- **Clock Format** — choose **12h** for 12-hour time with AM/PM, or **24h** for 24-hour time. Defaults to 24h.
- **Custom NTP Servers** — turn this on to show and edit the NTP server fields. When it is off, the panel uses the defaults: `0.pool.ntp.org`, `1.pool.ntp.org`, and `2.pool.ntp.org`.
- **NTP Server 1 / 2 / 3** — choose the network time servers used to keep the panel clock accurate when custom NTP servers are enabled.
- **Custom Month Names** — advanced Date card labels. Turn this on to edit the twelve month names used by Date cards. Turning it off resets them to the default English month names.
- **Sunrise / Sunset** — read-only reference values calculated from your timezone, updated daily. Displayed in whichever format you chose.

The **Clock Bar** section controls whether the top bar is shown. It also contains the indoor and outdoor temperature controls shown in that bar, including whether temperatures include the `°C` or `°F` suffix. When the clock bar is enabled, the small network icon at the far right shows WiFi strength or Ethernet status. Tap it to see the device name, IP address, WiFi strength, uptime, and firmware version.

The network status icon is on by default. Turn off **Show Network Status Icon** in the Clock Bar settings if you only want the clock and temperature items in the top bar.

## How It Works

The on-screen clock normally syncs directly from NTP over Wi-Fi. Home Assistant time is still used as a fallback, so the clock can continue to work if NTP is blocked but the panel is connected to Home Assistant.

You can use public NTP server names, such as the defaults, or a local server/IP address on your own network. If your panel uses manual network settings without DNS, use IP addresses for the NTP servers. Turning **Custom NTP Servers** off resets the saved NTP server values back to the defaults.

The clock format setting affects three things:

1. The **top bar clock** on the panel display, when the clock bar is shown.
2. The **sunrise and sunset** times shown in settings.
3. The **clock preview** on the web setup page.

The setting is saved on the device and persists across restarts.

Custom month names are saved as one device setting named **Screen: Month Names**. The setup page shows it as twelve separate fields so it is easier to edit.
