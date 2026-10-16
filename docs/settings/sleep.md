---
title: "Sleep & Schedule Settings"
description: Configure EspControl media cover art, screensaver, night schedule, and PIN protection in one guide.
---

# Sleep & Schedule

Choose when the screen sleeps, what it shows, and how it wakes. Open **Screen > Sleep & Schedule** on your [device setup page](/features/setup).

| Setting | What to do |
|---|---|
| [Cover Art Screen Saver](/settings/sleep#cover-art-screen-saver) | Show artwork from a playing media player. |
| [Screensaver](/settings/sleep#screensaver) | Sleep after inactivity or when a presence sensor clears. |
| [Night Schedule](/settings/sleep#night-schedule) | Set overnight behaviour using hours or a separate sensor. |
| [Pin Protection](/settings/sleep#pin-protection-after-wake) | Require a PIN before using touchscreen controls after wake. |

Start with **Screensaver > Timer** and **Then > Display Off**. Add Night Schedule if you need different overnight behaviour. A time-based Night Schedule takes priority over presence wake and media cover art; touch or Home Assistant’s **Screen: Wake** button still works.

## Cover Art Screen Saver

Open **Screen > Sleep & Schedule > Cover Art Screen Saver** on your [device setup page](/features/setup).

### Set It Up

1. Check that your `media_player` entity shows artwork in Home Assistant.
2. Turn on **Show Cover Art** and enter **Media Player Entity**, such as `media_player.living_room`.
3. Set **Show After** to the delay before artwork appears; the default is 10 seconds.
4. Start playback, leave the panel untouched, and check that the artwork loads.

### Screensaver Settings

| Setting | Use it to |
|---|---|
| **Keep Screen Awake During Playback** | Prevent normal screensaver sleep while playing. On by default; applies only with Show Cover Art enabled. |
| **Show After** | Choose how long to wait after the last touch before showing artwork. |
| **Show Track Details For** | Set the detail timeout on 4-inch square panels, including Never and Always. |
| **Persistent Play/Pause Control** | Show or hide the playback button on 4-inch square panels. Larger panels always show it. |

With Keep Screen Awake off, Timer or Sensor screensaver mode can still show eligible artwork when sleep activates. With Screensaver Disabled, artwork uses Show After. A time-based [Night Schedule](/settings/sleep#night-schedule) takes priority during night hours.

### External Sources

Enable **Show external sources** for TV, Line-in, or HDMI inputs. Optionally set **External Source Media Entity** to the player feeding that input, so its artwork and playback details can be used.

The saved external player remains configured when the toggle is off and can still supply usable artwork. If that player becomes unavailable, the external-input hide rule applies again.

### Advanced Options

Enable **Advanced Filtering** to reveal **Only Show When**. Enter matching attributes, for example `app_id=com.apple.TVMusic; media_content_type=music`. Turning Advanced Filtering off clears these conditions.

### Track Details Work but Artwork Is Missing

Playback metadata and image downloads use separate connections.

1. Confirm artwork appears for the selected player in Home Assistant.
2. Open **System > Home Assistant Settings** and check the image endpoint.
3. Follow [connection settings](/features/home-assistant) to set a reachable host, protocol, and port if Automatic fails.
4. Retry playback and confirm artwork loads.

If only one music source fails, collect [USB logs](/reference/collect-usb-logs) and report the source and firmware version. Remove private URLs and credentials before sharing logs.

## Screensaver

Open **Screen > Sleep & Schedule > Screensaver** on your [device setup page](/features/setup). Touch the sleeping panel to wake it.

### Choose When It Starts

| Mode | What to set |
|---|---|
| **Disabled** | No automatic screensaver. This is the default. |
| **Timer** | Choose an inactivity delay from 10 seconds to 1 hour. Every touch restarts it. Short delays require supported firmware. |
| **Sensor** | Enter a Home Assistant motion or presence entity in **Presence Entity**, such as `binary_sensor.hallway_presence`. Presence keeps the panel awake; no presence starts sleep. |

### Choose What It Shows

In Timer or Sensor mode, set **Then**:

| Action | Result |
|---|---|
| **Display Off** | Turns the backlight off. Default action. |
| **Screen Dimmed** | Keeps your cards visible at a lower brightness. The first tap wakes instead of pressing a card. |
| **Clock** | Shows a drifting clock. Set daytime/nighttime clock brightness; optionally enable **Show date**. |
| **Camera** | Shows a `camera.*` or `image.*` entity full-screen on supported P4 and 4-inch 4848S040 S3 panels. |

For Screen Dimmed, set its brightness. Automatic and Timed [Backlight](/features/backlight) modes offer separate daytime and nighttime dimmed levels.

Choose **Apply Configuration**, wait for the panel to reconnect, and test both sleep and wake. With Sensor mode, clear and trigger the sensor to check each state.

### Show Photos or a Camera

Select **Then > Camera** and enter **Camera Entity**. Choose **Fit** to keep the whole image visible, or **Fill** to crop to the screen. Enable **Display Clock** if you want a time overlay. These options require supported firmware.

The image refreshes when Home Assistant reports an update; the last successful image stays visible during downloads. Animated GIFs play automatically within the [image limits](/card-types/cameras#setting-up-a-camera-card). If images fail, check [Home Assistant connection settings](/features/home-assistant).

#### Photos from Immich

Use [EspControl Immich Companion](/immich/) for photo selection and slideshow timing. Follow [Connect to EspControl](/immich/display-setup), then use its Home Assistant `image.*` entity as the Camera Entity.

#### Photo Metadata

With Camera selected, enable **Display Metadata** and enter **Photo Metadata Entity**, a `sensor.*` containing the caption, date, or location. Turning metadata off keeps the saved entity.

Text appears at the bottom-right on screens at least 1024 pixels wide and at the upper-right on smaller screens or portrait orientations. Empty, unknown, and unavailable states are hidden; long text wraps to three lines. Metadata can appear with Display Clock off. The clock overlay applies to Camera, including image entities, and is hidden during media cover art.

### Wake from Home Assistant

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

### Screen Schedule

[Night Schedule](/settings/sleep#night-schedule) has its own hours or sensor. Its Time mode takes priority over screensaver presence wake and media cover art during night hours. Touch or Screen: Wake still works using the schedule's temporary-wake settings. To make presence control night behaviour, use Night Schedule's Sensor mode.

### Start the Screensaver with a Card

Add a **Screensaver** card in Layout; it needs no Home Assistant entity. Tapping it starts the selected Then action immediately, even with automatic screensavers disabled or media playing. Touch to wake, then enter the PIN if enabled.

## Night Schedule

Open **Screen > Sleep & Schedule > Night Schedule** on your [device setup page](/features/setup).

### Set the Schedule

1. Choose **Mode > Time** for fixed hours, or **Sensor** to follow a Home Assistant sensor. **Disabled** leaves normal screensaver and backlight rules in charge.
2. In **Time**, set **Daytime** (normal use starts) and **Night Time** (night behaviour starts). For example, `06:00` and `23:00` keep normal use from 6 AM until 11 PM. The window can cross midnight. Equal times keep normal mode on all day.
3. In **Sensor**, enter **Sensor Entity** and choose **Activate Night Schedule When > Sensor Is Off** or **Sensor Is On**. The opposite state returns to normal use.
4. Choose **At Night Time** from the table below.
5. Choose **Apply Configuration**, then test the boundary or change the sensor state.

| Night action | What to set |
|---|---|
| **Screen Off** | **When Woken, Idle Time to Screen Off** and **When Woken, Screen Brightness** for temporary wakes. |
| **Screen Dimmed** | **Dimmed Screen Brightness** for overnight use. |
| **Clock** | **Clock Brightness** and **Clock Text Colour**. |

### How It Works with Other Settings

The schedule's sensor is separate from the Screensaver **Presence Entity**. Time-based night hours take priority over screensaver presence wake and media cover art. If presence should control night behaviour, use **Sensor** mode here.

Normal-use hours follow your [Backlight](/features/backlight) and [Screensaver](/settings/sleep#screensaver) settings. When a scheduled Clock period ends, an idle panel goes directly to its normal screensaver action.

Enable **Screen > Appearance > Clock Bar > Show Night Mode Icon** to see a moon while night mode is active.

### Manual Wake and Sleep

Touch the sleeping panel or press its **Screen: Wake** button in Home Assistant. Screen Off and Clock modes temporarily wake using the saved wake brightness and idle time, then return to night behaviour. Waking does not change the schedule. [PIN protection](/settings/sleep#pin-protection-after-wake) still applies.

Press and hold a touchscreen button for three seconds to sleep manually. It stays asleep even during normal-use hours until you wake it. See [Wake from Home Assistant](/settings/sleep#wake-from-home-assistant) for an automation example.

## PIN Protection {#pin-protection-after-wake}

In **Screen > Sleep & Schedule > Pin Protection**, enable **Require PIN after wake**, enter a numeric **PIN** (1–16 digits), and choose **Save**. Protection starts once a PIN is saved. Update the panel firmware if these controls are missing.

- **Save** also changes an existing PIN. **Clear** removes it and disables protection; switching the toggle off keeps the PIN for later.
- **Randomize keypad layout** shuffles the digits each time the keypad opens (off by default).
- The [Screen Lock card](/card-types/screen-lock) shares this PIN and keypad setting, and works with automatic wake protection off.

After a screensaver or manual sleep, enter the PIN and press the checkmark to unlock. An incorrect PIN keeps the panel locked. **X** clears digits; with no digits entered, it closes the keypad without unlocking. All wake methods use this protection, and Cover Art playback controls remain blocked until unlocked.

The PIN protects local touchscreen controls only. Secure web setup separately with [web authentication](/getting-started/manual-esphome-setup#advanced-password-protect-the-web-page), as anyone with setup access can change or clear the PIN. PINs persist across restarts but are excluded from backups; importing a backup clears the destination panel's PIN, so set it again afterward.
