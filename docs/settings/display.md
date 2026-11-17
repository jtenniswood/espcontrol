---
title: "Display Settings"
description: Configure EspControl backlight brightness, screen rotation, and artwork loading in one guide.
---

# Display

Match the physical screen to its room and mounting position. Open **Screen > Display** on your [device setup page](/features/setup).

| Setting | What to do |
|---|---|
| [Backlight](/settings/display#backlight) | Set fixed brightness, sunrise/sunset levels, or timed day/night levels. |
| [Rotation](/settings/display#rotation) | Turn the interface to match the panel’s mounting orientation. |
| [Artwork Loading](/settings/display#artwork-loading) | Trade faster album art for smoother touch on ESP32-S3 panels. |

Start with **Backlight > Manual** and set a comfortable level. Set Rotation if needed, then choose **Apply Configuration** and check the physical screen. Rotation appears only on supported panels.

For dimmed or overnight brightness, use [Sleep & Schedule](/settings/sleep).

## Backlight

Open **Screen > Display > Backlight** on your [device setup page](/features/setup).

1. Choose a mode from the table below.
2. Set the visible brightness sliders (10%–100%).
3. For **Timed**, enter **Dawn** and **Dusk**. For **Automatic**, check the displayed sunrise and sunset times and your [timezone](/settings/system#time).

| Mode | What to set |
|---|---|
| **Manual** | One **Brightness** level. |
| **Automatic** | **Daytime Brightness** and **Nighttime Brightness**, switching at locally calculated sunrise and sunset. |
| **Timed** | The same day/night levels, switching at your **Dawn** and **Dusk** times. |

For example, Dawn `07:00` and Dusk `22:00` use the daytime level between those times. Before the clock syncs, the panel uses daytime brightness.

### Sleep Brightness

The [screensaver](/settings/sleep#screensaver) and [Night Schedule](/settings/sleep#night-schedule) have their own dimmed-screen and clock brightness settings. **Display Off / Screen Off** turns the backlight off completely. Waking normally restores the appropriate brightness; a temporary wake during Night Schedule uses its saved wake settings.

### Home Assistant Control

Use **Screen: Brightness Mode** and **Display Backlight** in Home Assistant. Its **Sunrise and sunset** and **Fixed times** options correspond to **Automatic** and **Timed** in web setup.

Changing Display Backlight's brightness selects Manual mode. Turning it on or off keeps the selected mode, so sleep/wake automations can still use automatic brightness.

## Rotation

Rotation changes the direction the panel UI faces, so the same device can be mounted in the orientation that suits your installation.

You will find this control in **Screen > Display > Rotation** on the panel web page. Choose the rotation that matches how the panel is mounted.

The available choices depend on the panel model and firmware support. Most supported panels offer 0, 90, 180, and 270 degree options. The setup preview updates when you change the value, and the setting is saved on the device.

If the rotation card is not shown, update the firmware first and check that your panel model supports rotation.

## Artwork Loading

ESP32-S3 panels decode album art and camera snapshots in software, a slice at a time between screen updates. **Screen > Display > Artwork Loading** sets how long each slice runs.

| Option | Effect |
|---|---|
| **Screen default** | Uses the value tuned for your panel model. Recommended for most screens. |
| **Smoothest touch (12 ms)** | Taps stay responsive while artwork loads, but new artwork appears more slowly. |
| **Balanced (40 ms)** | A middle ground for panels where artwork feels slow. |
| **Faster artwork (80 ms)** | New artwork appears sooner; taps can lag briefly after a track change. |
| **Fastest artwork (150 ms)** | Shortest artwork delay; taps can lag for a second or two while artwork loads. |

The change applies to the next image the panel loads and is saved on the device. Time spent waiting for Home Assistant to report a new track is not affected. ESP32-P4 panels decode artwork in hardware, so the card is not shown on those models.
