---
title: "Screensaver"
description: Set EspControl sleep after inactivity or presence changes, show a clock or photos, and protect touchscreen wake with a PIN.
---

# Screensaver

Open **Screen > Sleep & Schedule > Screensaver** on your [device setup page](/features/setup). Touch the sleeping panel to wake it.

## Choose When It Starts

| Mode | What to set |
|---|---|
| **Disabled** | No automatic screensaver. This is the default. |
| **Timer** | Choose an inactivity delay from 10 seconds to 1 hour. Every touch restarts it. Short delays require supported firmware. |
| **Sensor** | Enter a Home Assistant motion or presence entity in **Presence Entity**, such as `binary_sensor.hallway_presence`. Presence keeps the panel awake; no presence starts sleep. |

## Choose What It Shows

In Timer or Sensor mode, set **Then**:

| Action | Result |
|---|---|
| **Display Off** | Turns the backlight off. Default action. |
| **Screen Dimmed** | Keeps your cards visible at a lower brightness. The first tap wakes instead of pressing a card. |
| **Clock** | Shows a drifting clock. Set daytime/nighttime clock brightness; optionally enable **Show date**. |
| **Camera** | Shows a `camera.*` or `image.*` entity full-screen on supported P4 and 4-inch 4848S040 S3 panels. |

For Screen Dimmed, set its brightness. Automatic and Timed [Backlight](/features/backlight) modes offer separate daytime and nighttime dimmed levels.

Choose **Apply Configuration**, wait for the panel to reconnect, and test both sleep and wake. With Sensor mode, clear and trigger the sensor to check each state.

## Show Photos or a Camera

Select **Then > Camera** and enter **Camera Entity**. Choose **Fit** to keep the whole image visible, or **Fill** to crop to the screen. Enable **Display Clock** if you want a time overlay. These options require supported firmware.

The image refreshes when Home Assistant reports an update; the last successful image stays visible during downloads. Animated GIFs play automatically within the [image limits](/card-types/cameras#setting-up-a-camera-card). If images fail, check [Home Assistant connection settings](/features/home-assistant).

### Photos from Immich

Use [EspControl Immich Companion](/immich/) for photo selection and slideshow timing. Follow [Connect to EspControl](/immich/display-setup), then use its Home Assistant `image.*` entity as the Camera Entity.

### Photo Metadata

With Camera selected, enable **Display Metadata** and enter **Photo Metadata Entity**, a `sensor.*` containing the caption, date, or location. Turning metadata off keeps the saved entity.

Text appears at the bottom-right on screens at least 1024 pixels wide and at the upper-right on smaller screens or portrait orientations. Empty, unknown, and unavailable states are hidden; long text wraps to three lines. Metadata can appear with Display Clock off. The clock overlay applies to Camera, including image entities, and is hidden during media cover art.

## Wake from Home Assistant

Press the panel's **Screen: Wake** button in Home Assistant. It wakes the panel and restarts inactivity timers without pressing a card or changing the page. Select your panel's actual button entity in the automation editor; replace these example IDs:

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

## Screen Schedule

[Night Schedule](/features/screen-schedule) has its own hours or sensor. Its Time mode takes priority over screensaver presence wake and media cover art during night hours. Touch or Screen: Wake still works using the schedule's temporary-wake settings. To make presence control night behaviour, use Night Schedule's Sensor mode.

## PIN protection after wake

In **Screen > Sleep & Schedule > Pin Protection**, enable **Require PIN after wake**, enter a numeric **PIN** (1–16 digits), and choose **Save**. Protection starts once a PIN is saved. Update the panel firmware if these controls are missing.

- **Save** also changes an existing PIN. **Clear** removes it and disables protection; switching the toggle off keeps the PIN for later.
- **Randomize keypad layout** shuffles the digits each time the keypad opens (off by default).
- The [Screen Lock card](/card-types/screen-lock) shares this PIN and keypad setting, and works with automatic wake protection off.

After a screensaver or manual sleep, enter the PIN and press the checkmark to unlock. An incorrect PIN keeps the panel locked. **X** clears digits; with no digits entered, it closes the keypad without unlocking. All wake methods use this protection, and Cover Art playback controls remain blocked until unlocked.

The PIN protects local touchscreen controls only. Secure web setup separately with [web authentication](/getting-started/manual-esphome-setup#advanced-password-protect-the-web-page), as anyone with setup access can change or clear the PIN. PINs persist across restarts but are excluded from backups; importing a backup clears the destination panel's PIN, so set it again afterward.

## Start the Screensaver with a Card

Add a **Screensaver** card in Layout; it needs no Home Assistant entity. Tapping it starts the selected Then action immediately, even with automatic screensavers disabled or media playing. Touch to wake, then enter the PIN if enabled.
