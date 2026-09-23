---
title: "Show Home Assistant Media Cover Art on Your Touchscreen"
description:
  How to show media cover art while music or video is playing on your EspControl panel.
---

# Media Cover Art

Media Cover Art can turn the panel into a now-playing display while a selected Home Assistant media player is playing.

You need a `media_player` entity that supplies artwork in Home Assistant and a reachable Home Assistant image-download address. First check that artwork appears in Home Assistant, then configure the panel below. The [migration guide](/getting-started/migrate-esphome-media-player) covers dedicated album-art layouts.

You will find these controls in **Settings > Sleep & Schedule > Cover Art Screen Saver** on the panel web page.

## Play and pause

Tap the round button in the bottom-right corner to pause the displayed player. The artwork stays visible and the button changes to Play, so you can resume without leaving the screensaver. The button stays visible when track details fade away.

The button uses a brighter version of the colour extracted from the artwork and lightens while pressed, with a neutral grey fallback when artwork is unavailable. Its icon switches between black and white to remain readable, including on pale artwork. On 7-inch displays, its shape is adjusted for the panel's pixels so it appears round in either orientation.

Only a pause requested with this button keeps the screensaver open. Pausing from Home Assistant, a phone, or another control still ends cover-art mode. Touch anywhere outside the button to dismiss the screensaver without changing playback: paused music stays paused, and playing music keeps playing. Night schedules and display takeovers still apply. If the Home Assistant connection is lost while paused, the retained screensaver closes.

## Track details and layout

Titles expand to fit longer track names, up to these limits on the cover-art screensaver:

| Display | Maximum title lines |
| --- | --- |
| 4-inch square S3 and P4-86, in any orientation | 3 |
| 4.3-inch, in landscape | 3 |
| 7-inch, in landscape | 4 |
| 10-inch, in landscape | 5 |

Shorter titles use only the space they need. Artist text uses the remaining space, with an ellipsis when it does not fit. The 4.3-inch, 7-inch and 10-inch portrait layouts keep their shorter title limits.

On square displays, **Show Track Details For** controls how long details stay visible when a track changes. Pausing with the screensaver button brings the details back and keeps them visible until playback resumes, then the fade-out timer starts again. With **Never**, pausing still reveals the details, but resuming hides them immediately. With **Always**, they remain visible. Rectangular displays and screens without artwork keep track details visible.

## Settings

Turn on **Show Cover Art** to reveal the settings, then choose the **Media Player Entity** to watch, such as `media_player.living_room`.

### Screensaver Settings

- **Keep Screen Awake During Playback** — on by default. While Show Cover Art is enabled, this prevents normal screensaver sleep during playback and lets artwork appear after **Show After**. It has no effect while Show Cover Art is off.
- **Persistent Play/Pause Control** — on by default, directly below **Keep Screen Awake During Playback** on the 4-inch square S3 and P4-86. Turn it off to hide the button and restore the full width for track details, still allowing up to three title lines. Turning it off while paused from the screensaver dismisses it without resuming playback. Larger screens always show the playback button and do not show this setting.
- **Show After** — choose 3, 5, 10, or 30 seconds, 1 minute, or 5 minutes. The default is 10 seconds. This also controls when cover art returns after you dismiss it; every touch restarts the countdown.
- **Show Track Details For** — available on the 4-inch square displays. Choose **Never**, 3, 5, 10, 15, 20, 30, or 60 seconds, or **Always**. The default is 5 seconds.

With **Keep Screen Awake During Playback** off, Timer or Sensor screensaver mode can still show eligible artwork when the normal screensaver activates. With Screensaver mode **Disabled**, cover art uses **Show After**. A time-based [Night Schedule](/features/screen-schedule) takes priority over cover art while its night period is active.

### External sources

- **Show external sources** — allows cover art for the player's `TV`, `Line-in`, or `HDMI` input. Off by default.
- **External Source Media Entity** — shown when Show external sources is on. Optionally choose the player connected to that input. When it has current media, its playback state, artwork, track details, progress, and filtering attributes drive the cover-art screen.

If you turn off Show external sources, the saved external player remains configured. A usable external player can still supply artwork; if it becomes unavailable, the external-input hide rule applies again.

### Advanced Options

Turn on **Advanced Filtering** to reveal **Only Show When**. Enter matching media player attributes, such as `app_id=com.apple.TVMusic` or `app_id=com.apple.TVMusic; media_content_type=music`. Turning Advanced Filtering off clears the saved conditions.

Playback time and the progress bar appear only when Home Assistant provides a usable media duration. Live radio streams without a duration continue to show artwork and track details without an empty progress line.

If cover art is shown for `TV` or `Line-in` instead of hidden, the artist line shows **Source** because these inputs normally do not provide artist data.

Cover art is separate from the normal [Screensaver](/features/screensaver) mode. Use Screensaver when you want the panel to dim, show a clock, or turn off after inactivity.

For artwork downloads, open **Settings > System > Home Assistant Settings**. **Connection > Automatic** discovers the Home Assistant HTTP endpoint; **Manual** lets you choose **Home Assistant Protocol** (`http` or `https`) and **Home Assistant Port**. The card shows the current artwork endpoint. See [Home Assistant Settings](/features/setup#home-assistant-settings) for discovery and fallback behavior.

## Track Details Work but Artwork Is Missing

Playback metadata uses the native ESPHome connection; image downloads use HTTP or HTTPS. One can work while the other fails.

1. Confirm the selected player has artwork in Home Assistant.
2. Open **Settings > System > Home Assistant Settings** and check the displayed artwork endpoint.
3. If automatic discovery chooses an unreachable endpoint, use **Manual** with the reachable protocol and port. A reverse proxy may need **Artwork Base URL**.
4. Retry playback. Confirm both the small Cover Art card and its expanded view load an image.

Artwork availability depends on the source. If only Spotify, radio, Plex, or another source fails, capture [USB logs](/reference/collect-usb-logs) and include the source and firmware version in a report, with private URLs and credentials removed.
