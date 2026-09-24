---
title: "Control Home Assistant Media Players from Your Touchscreen"
description:
  How to use media cards on your EspControl panel to control Home Assistant media players.
---

# Control Home Assistant Media Players

A Media card controls a Home Assistant `media_player`. Choose a small one-job card, or use **All Controls** for the main playback screen.

Add your Sonos, Spotify, Apple TV, Plex, Music Assistant, or other player integration in Home Assistant first. EspControl uses the entities and capabilities that integration exposes; it does not connect a speaker or streaming account for you.

![Wide media card showing now-playing title and artist](/images/card-media.png)

## Set Up a Media Card

1. Select a card and change its type to **Media**. New Media cards start as **Cover Art**.
2. Choose a **Type** and enter the player entity, such as `media_player.living_room`.
3. For **Cover Art** and **All Controls**, enter a **Name** below Entity. This appears in the clock bar while the media popup is open. All Controls also uses it on the card when Label is selected. Other types offer a label or icon where supported.

| Type | Best for |
|---|---|
| **All Controls** | Playback, volume, progress, and any supported power or speaker controls in one popup. |
| **Cover Art** | Showing current artwork; tapping it opens All Controls. |
| **Track, Album or Playlist** | Playing a saved playlist, source, URL, or other media content. |
| **Speaker Group** | Opening the speaker-group screen directly. |
| **Play/Pause**, **Previous**, **Next** | A simple playback shortcut. |
| **Volume** | Opening a volume control. **Maximum Volume** can cap the level. |
| **Track Position** | Seeing and, where supported, seeking through the current item. |
| **Now Playing** | Showing title and artist, with optional progress or play/pause. |

## What to Expect

- **Now Playing** has independent **Tile Display** and **Tap Action** settings, described below.
- Playback buttons send the matching Home Assistant media action.
- Titles and artists display fullwidth Latin characters as ordinary letters (for example, `ＯＳＡＫＡ` becomes `OSAKA`) and omit emoji. This also applies to the Cover Art screensaver. Supported accented, Greek, and Cyrillic text is preserved; other scripts still depend on the panel's font coverage. The original Home Assistant metadata is unchanged.
- Shuffle and Repeat appear in **All Controls** only when the player advertises support for them. A supported control stays disabled until Home Assistant reports its current state, then continues to follow changes made elsewhere.
- Repeat cycles through **Off**, **All**, and **One**. Its icon changes to Repeat Once in **One** mode, while active Shuffle and Repeat modes use the accent colour.
- Volume and track position follow changes made elsewhere in Home Assistant. Some players show progress but do not support seeking; some only support volume up and down rather than an exact level.
- All Controls hides tabs that the selected player does not support. Its speaker tab appears only when compatible speakers are available.
- For **Track, Album or Playlist**, choose the speaker, then provide the content ID or URI. You can also set the player source or input when the integration uses one.

## Now Playing

When testing an unreleased branch through remote ESPHome packages, include `common/addon/embedded_web_ui.yaml` after the device package from the same repository and branch. This embeds that branch's editor; the standard device package otherwise loads the published editor, which may not yet contain these settings.

Choose what the tile displays separately from what happens when you tap it. Title and artist remain visible in every display mode.

| Tile Display | Appearance |
|---|---|
| **Track Info** | Title and artist on the normal tile background. |
| **Progress** | Playback progress behind the title and artist. The played portion uses your highlight colour while playing, and stays at the current position in grey while paused. |
| **Playback State** | The whole tile changes colour to indicate playing or paused. |

| Tap Action | Behavior |
|---|---|
| **None** | Tapping does nothing. |
| **Play/Pause** | Toggles playback. |
| **Seek** | Seeks to the position tapped. Available only with **Progress**. |

Changing Tile Display keeps the selected Tap Action. If you switch away from Progress while Seek is selected, Tap Action becomes None.

These selections update in place so Card Settings stays open. If playback progress data is missing, the display requests it again when playback resumes or track metadata refreshes. Players that do not report a duration cannot show a progress fill.

Existing saved cards and backups retain their behavior automatically: **None** becomes **Track Info + None**, **Track Position** becomes **Progress + Play/Pause**, and **Play/Pause** becomes **Playback State + Play/Pause**. You do not need to edit or save each card after updating.

## Cover Art

Cover Art is available in square **1×1**, **2×2**, and **3×3** card sizes. It uses one of the panel's shared image slots. ESP32-P4 screens have six slots shared between Camera and Cover Art cards across all pages. If the card shows **Too many**, remove one of those image cards.

## Speaker Groups

Use **Speaker Group** to open the speaker controls directly, or access them from **All Controls**. Follow the [Speaker Groups setup guide](/features/speaker-groups) for the Home Assistant discovery sensor templates, Music Assistant options, card setup, group volume, and troubleshooting.

::: info Requires Home Assistant actions
Media cards send Home Assistant actions. If a control does not respond, check [Enable Actions](/getting-started/home-assistant-actions).
:::
