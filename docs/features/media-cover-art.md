---
title: "Media Cover Art Screensaver"
description: Show now-playing artwork, control playback, and choose when EspControl switches to a media cover-art display.
---

# Media Cover Art

Open **Screen > Sleep & Schedule > Cover Art Screen Saver** on your [device setup page](/features/setup).

## Set It Up

1. Check that your `media_player` entity shows artwork in Home Assistant.
2. Turn on **Show Cover Art** and enter **Media Player Entity**, such as `media_player.living_room`.
3. Set **Show After** to the delay before artwork appears; the default is 10 seconds.
4. Start playback, leave the panel untouched, and check that the artwork loads.

## Screensaver Settings

| Setting | Use it to |
|---|---|
| **Keep Screen Awake During Playback** | Prevent normal screensaver sleep while playing. On by default; applies only with Show Cover Art enabled. |
| **Show After** | Choose how long to wait after the last touch before showing artwork. |
| **Show Track Details For** | Set the detail timeout on 4-inch square panels, including Never and Always. |
| **Persistent Play/Pause Control** | Show or hide the playback button on 4-inch square panels. Larger panels always show it. |

With Keep Screen Awake off, Timer or Sensor screensaver mode can still show eligible artwork when sleep activates. With Screensaver Disabled, artwork uses Show After. A time-based [Night Schedule](/features/screen-schedule) takes priority during night hours.

## External Sources

Enable **Show external sources** for TV, Line-in, or HDMI inputs. Optionally set **External Source Media Entity** to the player feeding that input, so its artwork and playback details can be used.

The saved external player remains configured when the toggle is off and can still supply usable artwork. If that player becomes unavailable, the external-input hide rule applies again.

## Advanced Options

Enable **Advanced Filtering** to reveal **Only Show When**. Enter matching attributes, for example `app_id=com.apple.TVMusic; media_content_type=music`. Turning Advanced Filtering off clears these conditions.

## Track Details Work but Artwork Is Missing

Playback metadata and image downloads use separate connections.

1. Confirm artwork appears for the selected player in Home Assistant.
2. Open **System > Home Assistant Settings** and check the image endpoint.
3. Follow [connection settings](/features/home-assistant) to set a reachable host, protocol, and port if Automatic fails.
4. Retry playback and confirm artwork loads.

If only one music source fails, collect [USB logs](/reference/collect-usb-logs) and report the source and firmware version. Remove private URLs and credentials before sharing logs.
