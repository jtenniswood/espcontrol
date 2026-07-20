---
title: EspControl Screensaver
description:
  How to configure screensaver modes and presence sensor wake on your EspControl panel.
---

# Screensaver

The panel can use a screensaver when it's not being used. When active, it can dim the normal screen, show a dim clock, or turn the backlight off so the panel goes dark. Touch the screen to wake it up.

There are three screen saver modes, configured in the **Settings** tab on the [Setup](/features/setup) page:

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

When the screensaver activates, you can choose what happens:

- **Screen Dimmed** — keeps the normal screen visible, but lowers the backlight. The first tap wakes the screen instead of pressing a card.
- **Clock** — shows a large drifting clock at reduced brightness (the default). The clock repositions itself periodically to prevent burn-in.
- **Photos** — rotates through the photos in a Home Assistant media folder, optionally with the date, time, and current weather shown over the picture. See [Photos](#photos) below.
- **Display Off** — switches to a black screen and turns the backlight off completely. While the backlight is off, EspControl can exercise the LCD pixels in the background to reduce burn-in risk; this should not be visible.

When Screen Dimmed is selected, set **Dimmed Screen Brightness**. When Clock is selected, set separate **Daytime Clock Brightness** and **Nighttime Clock Brightness** values. Clock brightness uses the same sunrise and sunset calculation as the main screen brightness.

### Photos

The photo screensaver shows pictures from a folder in Home Assistant's [media sources](https://www.home-assistant.io/more-info/local-media/setup-media/) — for example a folder you keep in `config/www` or the Home Assistant `media` folder. Photos change on a timer, and the panel can float the date, time, and current weather over each picture.

Because browsing a media folder needs an authenticated connection, the panel connects to Home Assistant over its websocket API. Enter your Home Assistant address and a long-lived access token in the **Home Assistant** connection fields on the Settings page. The token is stored on the panel and is included in configuration backups, so keep exported backups private.

Then set:

- **Photos Folder** — the media-source folder to show, for example `media-source://media_source/local/photos`. Point it at a folder, not a single file.
- **Seconds Per Photo** — how long each picture stays on screen before the next one.
- **Shuffle** — show the photos in a random order instead of in sequence.
- **Show Date and Time** — float the current date and time over the photo.
- **Show Weather** — float the current conditions and temperature over the photo. Enter a Home Assistant `weather.` entity to use.

::: tip
The panel needs to reach the photos over your local network. Folders in Home Assistant's `config/www` are served without a login and work well; large folders take a moment to list the first time.
:::

## Sensor

Instead of a timer, the screensaver is controlled by a motion or presence sensor (like a mmWave sensor mounted nearby). When someone is in the room, the screen stays on. When nobody is detected, the screen goes to sleep — and wakes up again when someone walks past.

To use this, enter the name of your motion or presence sensor from Home Assistant (for example, `binary_sensor.hallway_presence`).

Below the presence entity, use **Then** to choose whether the panel dims the screen, shows the clock, or turns the display off when nobody is detected. This uses the same options as Timer mode.

Presence wakes the panel from those dimmed, clock, or display-off states. When the normal cards or media cover art are already visible, presence does not change the page or restart the cover-art timer.

Switching back to Timer keeps the sensor name saved, so you can return to Sensor mode later without typing it in again.

::: tip
Touching the screen or pressing its **Screen: Wake** button in Home Assistant always wakes it up, no matter which screensaver mode you're using.
:::

## Wake from Home Assistant

Every panel exposes a stateless **Screen: Wake** button in Home Assistant. Pressing it behaves like touching the sleeping panel: it wakes a dimmed, clock, display-off, cover-art, or manually sleeping screen and restarts the normal inactivity timers. If the screen is already awake, it extends the active period without changing the page or pressing a card.

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
