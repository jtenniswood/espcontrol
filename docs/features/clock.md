---
title: "Time Settings"
description: Set EspControl timezone, 12/24-hour format, and optional network time servers under System preferences.
---

# Time Settings

Open **System > Preferences > Time** on your [device setup page](/features/setup).

1. Set **Timezone** to **Auto (Home Assistant)** to follow Home Assistant, or choose a fixed timezone.
2. Choose **Clock Format > 12-hour** or **24-hour**. The 12-hour display has no AM/PM suffix; 24-hour is the default.
3. Leave **Custom NTP Servers** off unless you need your own time servers. If enabled, enter up to three server names or IP addresses.

The format applies to the top-bar clock, setup preview, and displayed sunrise/sunset times. The timezone also affects automatic brightness, automatic theme, schedules, and automatic temperature units.

## If the Time Is Wrong

- Check the selected timezone first.
- The clock normally syncs from NTP, with Home Assistant time as a fallback. Ensure at least one source is reachable.
- With custom networking and no DNS, use IP addresses for NTP servers.
- Turning **Custom NTP Servers** off restores `0.pool.ntp.org`, `1.pool.ntp.org`, and `2.pool.ntp.org`.

To show or hide the time on the panel, edit the Clock item in **Layout**, or control the whole [Clock Bar](/features/appearance#clock-bar) under **Screen > Appearance**.
