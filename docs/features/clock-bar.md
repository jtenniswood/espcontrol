---
title: EspControl Clock Bar
description:
  How to configure the clock bar shown at the top of your EspControl panel.
---

# Clock Bar

The clock bar is the narrow status area at the top of the panel. It uses a fixed layout: one temperature reading on the left, the current time in the middle, and the connectivity icon on the right.

You will find these controls in **Settings > Display > Clock Bar** on the panel web page.

## Settings

- **Show Clock Bar** - turns the whole top bar on or off.
- **Temperature** - select the temperature item in the screen preview, choose **Edit**, then choose the Home Assistant sensor and whether to show the degree symbol.
- **Clock** - select the clock item in the screen preview and choose **Hide** or **Show**.
- **Connectivity** - select the connectivity item in the screen preview and choose **Hide** or **Show**.

The clock bar layout is not customizable. Hidden items stay greyed in the web preview so you can select and show them again, but they are hidden on the device screen. Extra saved temperature entries, weather settings, and older saved layout strings are ignored by current firmware.

Tap the network status icon on the panel to see device details, including the device name, IP address, WiFi strength, uptime, and firmware version.

On firmware builds with local voice controls, turn on **Voice Services** to enable wake-word listening and show the microphone shortcut in the clock bar. Voice Services is off by default. When it is off, wake-word listening is stopped and the microphone/speaker shortcut is hidden. Tap the shortcut to adjust the device volume and access the microphone mute control. A microphone-off icon means voice listening is muted; a speaker-off icon means speaker output is muted. See [Voice Control](/features/voice-control) for the ESP32-P4 86 voice setup.

The time format and timezone are configured separately in [Time Settings](/features/clock). The temperature unit is configured in [Temperature Settings](/features/temperature).
