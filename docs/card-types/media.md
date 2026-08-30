---
title: Media Cards
description:
  How to use media cards on your EspControl panel to control Home Assistant media players.
---

# Media

A Media card controls a Home Assistant `media_player`. Choose a small one-job card, or use **All Controls** for the main playback screen.

![Wide media card showing now-playing title and artist](/images/card-media.png)

## Set Up a Media Card

1. Select a card and change its type to **Media**. New Media cards start as **Cover Art**.
2. Choose a **Type** and enter the player entity, such as `media_player.living_room`.
3. Set a label or icon where the chosen type offers one.

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

- Playback buttons send the matching Home Assistant media action.
- Shuffle and Repeat appear in **All Controls** only when the player advertises support for them. A supported control stays disabled until Home Assistant reports its current state, then continues to follow changes made elsewhere.
- Repeat cycles through **Off**, **All**, and **One**. Its icon changes to Repeat Once in **One** mode, while active Shuffle and Repeat modes use the accent colour.
- Volume and track position follow changes made elsewhere in Home Assistant. Some players show progress but do not support seeking; some only support volume up and down rather than an exact level.
- All Controls hides tabs that the selected player does not support. Its speaker tab appears only when compatible speakers are available.
- For **Track, Album or Playlist**, choose the speaker, then provide the content ID or URI. You can also set the player source or input when the integration uses one.

## Cover Art

Cover Art is available in square **1×1**, **2×2**, and **3×3** card sizes. It uses one of the panel's shared image slots. ESP32-P4 screens have six slots shared between Camera and Cover Art cards across all pages. If the card shows **Too many**, remove one of those image cards.

## Speaker Groups

For speaker groups, first confirm the speakers can join in Home Assistant. EspControl uses the compatible-player list supplied by the configured discovery entity; by default this is `sensor.speaker_group`. The group screen stays hidden when no usable speakers are reported.

::: info Requires Home Assistant actions
Media cards send Home Assistant actions. If a control does not respond, check [Enable Actions](/getting-started/home-assistant-actions).
:::
