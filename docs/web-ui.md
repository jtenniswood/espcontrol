---
title: Web UI
description:
  How to use the built-in web page to configure buttons, display settings, screensaver, and brightness on your Espcontrol panel.
---

# Web UI

Your Espcontrol panel has a built-in web page for configuration. Open it by typing the device's IP address into any browser on your phone or computer — for example, `http://192.168.1.42`.

::: tip Finding the IP address
The address is shown on the display screen when no buttons are configured. You can also find it in your router's device list or in Home Assistant under **Settings > Devices & Services > ESPHome**.
:::

## Screen tab

This is where you set up your buttons.

- **Live preview** — see your button layout as it will appear on the display, including the top bar with clock and temperatures.
- **Add buttons** — tap an empty slot to assign a Home Assistant device to it. Choose the device you want to control (its entity ID in Home Assistant, like `light.living_room`).
- **Remove buttons** — remove a button to free up the slot.
- **Reorder buttons** — drag and drop buttons to rearrange them.
- **Set an icon** — pick from hundreds of icons, or choose **Auto** to let the panel pick one based on the device type (lights get a lightbulb, fans get a fan, etc.).
- **Custom label** — give the button a name, or leave it blank to use the device's name from Home Assistant.
- **Sensor readout** — optionally show a sensor value on the button (like a temperature reading) by entering a sensor entity and unit.

The panel supports up to **20 buttons**.

## Settings tab

Adjust the look and behaviour of your panel.

### Buttons

- **On colour / Off colour** — the colour of buttons when they are switched on or off. Enter a colour code like `FF8C00` for orange.

### Display

- **Indoor / Outdoor temperature** — toggle the temperature display on or off, and choose which Home Assistant sensor to use for each.
- **Presence sensor** — if you have a motion or presence sensor, enter its entity ID here. The panel will wake from the screensaver when it detects movement.

### Brightness

- **Timezone** — select your timezone so the panel can calculate sunrise and sunset for your location. Sunrise and sunset times are calculated on-device — no internet or Home Assistant dependency.
- **Daytime brightness** — how bright the screen should be during the day (10%–100%).
- **Nighttime brightness** — how bright the screen should be at night (10%–100%).
- **Sunrise / Sunset** — shown for reference so you know when the brightness will change. Updates automatically each day based on your timezone.

### Screensaver

- **Timeout** — how long the panel waits before activating the screensaver (30 seconds to 30 minutes, in 30-second steps). The default is 5 minutes.

### Firmware

- **Version** — the firmware version currently running on your panel.
- **Auto Update** — turn on to let the panel install new firmware versions automatically.
- **Update Frequency** — how often to check for updates: Hourly, Daily, Weekly, or Monthly.
- **Check for Update** — press to check for a new version right now.

## Logs tab

A live stream of what the device is doing behind the scenes. This is mostly useful for troubleshooting — if something isn't working, the logs may show why. Entries are colour-coded by severity.

## Apply Configuration

After making changes, tap **Apply Configuration** to restart the panel and load your new settings. Some changes (like adding a button) take effect immediately, but a restart ensures everything is applied cleanly.

## Related

- [Buttons & Icons](/buttons-and-icons) — how Auto icons work and the full icon list
- [Display & Screensaver](/display-screensaver) — more about temperature display and screensaver behaviour
- [Backlight Schedule](/backlight-schedule) — how day/night brightness works
- [Firmware Updates](/firmware-updates) — more about automatic and manual updates
