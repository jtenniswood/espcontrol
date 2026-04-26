---
title: Clock
description:
  How to configure clock sync, timezone, and 12/24-hour format on your Espcontrol panel.
---

# Clock

The panel can display a clock in the top bar, updated every minute from network time. You can choose your timezone, switch between 12-hour and 24-hour format, set custom NTP servers, or hide the top bar completely.

## Settings

Configured in the **Clock** section of the **Settings** tab in [Setup](/features/setup).

- **Timezone** — select your timezone from the dropdown. This also determines sunrise and sunset times used by the [backlight schedule](/features/backlight).
- **Clock Format** — choose **12h** for 12-hour time with AM/PM, or **24h** for 24-hour time. Defaults to 24h.
- **NTP Server 1 / 2 / 3** — choose the network time servers used to keep the panel clock accurate. Defaults to `0.pool.ntp.org`, `1.pool.ntp.org`, and `2.pool.ntp.org`.
- **Show Clock Bar** — turn this off to hide the top clock/temperature bar and give the card grid more screen space.
- **Sunrise / Sunset** — read-only reference values calculated from your timezone, updated daily. Displayed in whichever format you chose.

## How It Works

The on-screen clock normally syncs directly from NTP over Wi-Fi. Home Assistant time is still used as a fallback, so the clock can continue to work if NTP is blocked but the panel is connected to Home Assistant.

You can use public NTP server names, such as the defaults, or a local server/IP address on your own network. If your panel uses manual network settings without DNS, use IP addresses for the NTP servers.

The clock format setting affects three things:

1. The **top bar clock** on the panel display, when the clock bar is shown.
2. The **sunrise and sunset** times shown in settings.
3. The **clock preview** on the web setup page.

The setting is saved on the device and persists across restarts.
