---
title: EspControl Media Cover Art
description:
  How to show media cover art while music or video is playing on your EspControl panel.
---

# Media Cover Art

Media Cover Art can turn the panel into a now-playing display while a selected Home Assistant media player is playing.

You will find these controls in **Settings > Sleep & Schedule > Media Cover Art** on the panel web page.

## Settings

- **Show Cover Art** - enables the cover art display and keeps the screen awake while artwork is shown.
- **Media Player Entity** - chooses the media player entity to watch, such as `media_player.living_room`.
- **Show After** - chooses how long cover art waits before appearing, from 3 seconds to 5 minutes.
- **Show After** also controls how long cover art waits before returning after you dismiss it by touch. The countdown restarts after every touch, so cover art returns only after the selected time has passed since your most recent touch.
- **Show Track Details For** - controls how long track information is shown over the artwork on the 4-inch square displays.
- **Advanced Options** - contains playback, source, and filtering controls you may not need every day.
- **Keep Screen Awake During Playback** - is on by default and prevents the display from sleeping while media plays. It does not bypass the **Show After** delay.
- **Hide for external source inputs** - hides cover art when the selected media player source is `TV` or `Line-in`.
- **Advanced Filtering** - reveals **Only Show When**, which limits cover art to matching media player attributes, such as `app_id=com.apple.TVMusic` or `app_id=com.apple.TVMusic; media_content_type=music`.

If cover art is shown for `TV` or `Line-in` instead of hidden, the artist line shows **Source** because these inputs normally do not provide artist data.

Cover art is separate from the normal [Screensaver](/features/screensaver) mode. Use Screensaver when you want the panel to dim, show a clock, or turn off after inactivity.

If your Home Assistant instance uses a custom port, open **Settings > System > Home Assistant Settings** and set **Home Assistant Port** to match it. Media cover art downloads use this port when loading artwork from Home Assistant.
