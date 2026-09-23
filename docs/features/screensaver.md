---
title: "Screensaver"
description:
  How to configure screensaver modes and presence sensor wake on your EspControl panel.
---

# Screensaver

The panel can use a screensaver when it's not being used. When active, it can dim the normal screen, show a dim clock, display a Home Assistant image, or turn the backlight off so the panel goes dark. Touch the screen to wake it up.

There are three screensaver modes, configured in **Settings > Sleep & Schedule > Screensaver** on the [Setup](/features/setup) page:

## Disabled

The screensaver does not run automatically. This is the default setting.

## Timer

The screensaver turns on after the panel hasn't been touched for a set amount of time. Choose from:

- 10 seconds
- 30 seconds
- 1 minute
- **5 minutes** (the default)
- 10, 15, 20, 30, or 45 minutes
- 1 hour

If the 10 or 30 second choices are not shown, update the panel firmware first. The web page checks what range the installed firmware supports before showing the shorter timer values.

### What Happens

Use **Then** to choose what happens when the screensaver activates:

- **Screen Dimmed** — keeps the normal screen visible, but lowers the backlight. The first tap wakes the screen instead of pressing a card.
- **Clock** — shows a large drifting clock at reduced brightness. The clock repositions itself periodically to prevent burn-in.
- **Display Off** (the default) — switches to a black screen and turns the backlight off completely. While the backlight is off, EspControl can exercise the LCD pixels in the background to reduce burn-in risk; this should not be visible.
- **Camera** — on ESP32-P4 panels and the 4-inch 4848S040 ESP32-S3 panel, shows the selected `camera.*` or `image.*` entity full-screen. Choose **Fit** to keep the whole image visible (with black space where its shape does not match the panel) or **Fill** to crop the image to the screen. The image refreshes when Home Assistant reports an update and the previous successful image stays visible while a replacement downloads. If no image is available, the panel shows **Camera unavailable** and retries safely.

When Screen Dimmed is selected, Manual brightness mode uses **Dimmed Screen Brightness**. Automatic and Timed brightness modes use separate **Daytime Dimmed Screen Brightness** and **Nighttime Dimmed Screen Brightness** values, changing at the same sunrise/sunset or dawn/dusk boundary as the main screen. When Clock is selected, set separate **Daytime Clock Brightness** and **Nighttime Clock Brightness** values.

The 4-inch ESP32-S3 camera screensaver uses the panel's native 480×480 resolution. The screensaver requests the original Home Assistant snapshot to avoid extra compression, then resizes it on the panel. If downloading or decoding it fails, subsequent requests ask Home Assistant for display-sized snapshots for the rest of that screensaver session. Sharpness still depends on the source image; filtered JPEG resizing preserves detail when reducing images and smooths edges when enlarging them.

### Photos from Immich

Use [EspControl Immich Companion](/immich/) to show photos from your Immich library, albums, Memories or keyword searches. Its Home Assistant integration provides an `image.*` entity that works with **Then → Camera**, plus slideshow controls and photo-detail sensors.

Follow [Connect to EspControl](/immich/display-setup) to install the companion, choose the image entity, match the screen shape and add optional date or location metadata. The companion controls photo selection and slideshow timing; EspControl displays the images when the screensaver activates.

### Photo metadata

With **Camera** selected, enable **Display Metadata** below **Display Clock** to reveal the **Photo Metadata Entity** field. Enter a Home Assistant `sensor.*` entity containing the current photo's caption, date, or location. Turning the toggle off hides metadata and the field without clearing the saved entity. Existing metadata setups remain enabled after upgrading.

Text appears toward the bottom-right on screens at least 1024 pixels wide. On smaller screens and large panels rotated into portrait, it appears in the upper-right corner. Empty, `unknown`, and `unavailable` states are hidden; the clock stays in the same position. Metadata can also appear with **Display Clock** switched off. Long text wraps to at most three lines.

**Display Clock** applies only to the Camera screensaver, including `camera.*` and `image.*` sources. It is hidden while the media Cover Art screensaver is active. Camera and clock-overlay controls appear only when the installed firmware supports them.

## Sensor


Instead of a timer, the screensaver is controlled by a motion or presence sensor (like a mmWave sensor mounted nearby). When someone is in the room, the screen stays on. When nobody is detected, the screen goes to sleep — and wakes up again when someone walks past.

To use this, enter the name of your motion or presence sensor from Home Assistant (for example, `binary_sensor.hallway_presence`) in **Presence Entity**. This remains the Screensaver's own sensor; Night Schedule has a separate **Sensor Entity** when it uses Sensor mode.

Below the presence entity, use **Then** to choose whether the panel dims the screen, shows the clock, displays a camera/image, or turns the display off when nobody is detected. Camera is available on ESP32-P4 panels and the 4-inch 4848S040 ESP32-S3 panel, using the same **Camera Entity** setting as Timer mode. The **Fit** and **Fill** choices apply here too.

Presence wakes the panel from those dimmed, clock, camera, or display-off states. When the normal cards or media cover art are already visible, presence does not change the page or restart the cover-art timer.

Switching back to Timer keeps the sensor name saved, so you can return to Sensor mode later without typing it in again.

::: tip
Touching the screen or pressing its **Screen: Wake** button in Home Assistant always wakes it up, no matter which screensaver mode you're using.
:::

## Wake from Home Assistant

Every panel exposes a stateless **Screen: Wake** button in Home Assistant. Pressing it behaves like touching the sleeping panel: it wakes a dimmed, clock, camera, display-off, cover-art, or manually sleeping screen and restarts the normal inactivity timers. If the screen is already awake, it extends the active period without changing the page or pressing a card.

You can use the button in an automation, for example to wake the panel when a door opens:

```yaml
triggers:
  - trigger: state
    entity_id: binary_sensor.front_door
    to: "on"
actions:
  - action: button.press
    target:
      entity_id: button.your_panel_screen_wake
```

Replace the example entity IDs with your own door sensor and the panel's **Screen: Wake** entity. Home Assistant assigns the final button entity ID, so select the entity from the automation editor rather than relying on the example name.

## Screen Schedule

The [screen schedule](/features/screen-schedule) is separate from the screensaver. Use it when you want the panel to be fully dark, dimmed, or showing a clock overnight.

When Night Schedule is using fixed **Time** hours, it has priority over screensaver sensor wake during night time. The screensaver presence sensor still keeps the panel awake and wakes it during normal daytime operation, but it does not override scheduled night time. Touch and Home Assistant button wake still work, using the temporary wake settings from the screen schedule.

If you want presence to control when the panel is in night mode, set Night Schedule to **Sensor** mode instead of **Time** mode.

On the first start after updating to the separate-sensor firmware, the existing Screensaver Presence Entity is copied to the Night Schedule Sensor Entity once. You can then change either setting independently without affecting the other.
