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
- **Show Night Mode Icon** - shows a moon beside the connectivity icon while the night schedule is active. Off by default.
- **Temperature** - select the temperature item in the screen preview, choose **Edit**, then choose the Home Assistant sensor and whether to show the degree symbol.
- **Clock** - select the clock item in the screen preview and choose **Hide** or **Show**.
- **Connectivity** - select the connectivity item in the screen preview and choose **Hide** or **Show**.

Opening a card control, such as climate or media, shows the card’s label on the left of the clock bar in the same style as subpage titles. Closing the control restores the subpage title or temperature reading.

The clock bar layout is not customizable. Hidden items stay greyed in the web preview so you can select and show them again, but they are hidden on the device screen. Extra saved temperature entries, weather settings, and older saved layout strings are ignored by current firmware.

Tap the network status icon on the panel to open Settings. The page keeps the clock bar visible, shows **Settings** to the left of the clock, and uses the device's normal grid—for example, 5×3 on a 7-inch display. The cards are ordered **Back**, **IP address**, **Wi-Fi quality** (Wi-Fi builds only), then **Build**, flowing left to right and then top to bottom. **IP address** shows the current address, **Wi-Fi quality** shows the signal percentage or **Disconnected** while unavailable, and **Build** shows the installed firmware version. Ethernet builds omit the Wi-Fi card. Tap **Back** to return to the page you were using and restore its temperature display.

The night mode moon appears whenever the [Night Schedule](/features/screen-schedule) is in its night period, in both **Time** and **Sensor** mode - in Sensor mode it follows the sensor entity and the activation state you chose. It is visible in practice when the schedule keeps the screen awake or dimmed rather than turning it off, and it disappears again when normal mode resumes.

On firmware builds with local voice controls, turn on **Voice Services** to enable wake-word listening and show the microphone shortcut in the clock bar. Voice Services is off by default. When it is off, wake-word listening is stopped and the microphone/speaker shortcut is hidden. Tap the shortcut to adjust the device volume and access the microphone mute control. The clock bar shows **Voice** while these controls are open, then returns to its previous label or temperature when you close them. A microphone-off icon means voice listening is muted; a speaker-off icon means speaker output is muted. See [Voice Control](/features/voice-control) for the ESP32-P4 86 voice setup.

The time format and timezone are configured separately in [Time Settings](/features/clock). The temperature unit is configured in [Temperature Settings](/features/temperature).
