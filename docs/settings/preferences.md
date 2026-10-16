---
title: "Preferences Settings"
description: Configure EspControl language, timezone, clock format, and temperature units in one guide.
---

# Preferences

Set the panel’s language, local time, and temperature units. Open **System > Preferences** on your [device setup page](/features/setup).

| Setting | What to do |
|---|---|
| [Language](/settings/preferences#language) | Choose the language used by built-in touchscreen labels. |
| [Time](/settings/preferences#time) | Set timezone, clock format, and optional NTP servers. |
| [Temperature](/settings/preferences#temperature) | Choose Auto, °C, or °F for temperature labels. |

Set **Time > Timezone** first: automatic theme, brightness, schedules, and temperature units depend on it. Choose **Apply Configuration** after editing, then check the panel’s clock and labels.

## Language

1. Open **System > Preferences > Language** on your [device setup page](/features/setup).
2. Choose a language from the list included in your firmware.
3. Choose **Apply Configuration**, then check the panel's built-in labels.

This changes text built into the touchscreen. Entity names, custom labels, media titles, and other Home Assistant content keep their supplied text. The browser setup page remains in English.

The choice is saved on the device and included in backups. Home Assistant can also change it through **Screen: Language**.

For a missing language or incorrect translation, [open a GitHub issue](https://github.com/jtenniswood/espcontrol/issues/new).

## Time

Open **System > Preferences > Time** on your [device setup page](/features/setup).

1. Set **Timezone** to **Auto (Home Assistant)** to follow Home Assistant, or choose a fixed timezone.
2. Choose **Clock Format > 12-hour** or **24-hour**. The 12-hour display has no AM/PM suffix; 24-hour is the default.
3. Leave **Custom NTP Servers** off unless you need your own time servers. If enabled, enter up to three server names or IP addresses.

The format applies to the top-bar clock, setup preview, and displayed sunrise/sunset times. The timezone also affects automatic brightness, automatic theme, schedules, and automatic temperature units.

### If the Time Is Wrong

- Check the selected timezone first.
- The clock normally syncs from NTP, with Home Assistant time as a fallback. Ensure at least one source is reachable.
- With custom networking and no DNS, use IP addresses for NTP servers.
- Turning **Custom NTP Servers** off restores `0.pool.ntp.org`, `1.pool.ntp.org`, and `2.pool.ntp.org`.

To show or hide the time on the panel, edit the Clock item in **Layout**, or control the whole [Clock Bar](/features/appearance#clock-bar) under **Screen > Appearance**.

## Temperature

Open **System > Preferences > Temperature** on your [device setup page](/features/setup). Set **Temperature Unit** to **Auto**, **°C**, or **°F**. Auto selects °F for US and US-territory timezones, otherwise °C.

### Choose the Clock-Bar Sensor

1. Open **Layout** and select **Temperature** in the top-bar preview.
2. Choose **Edit** and enter the Home Assistant temperature sensor.
3. Turn **Show Degree Symbol** off if you want numbers without the `°C` or `°F` suffix.

The bar shows one temperature sensor. Hiding its preview item keeps the sensor saved. **Screen > Appearance > Clock Bar > Show Clock Bar** controls visibility of the entire bar.
