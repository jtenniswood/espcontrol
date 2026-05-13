---
title: Media Cards
description:
  How to use media cards on your EspControl panel to control Home Assistant media player entities.
---

# Media

A Media card controls a Home Assistant `media_player` entity. It can work as a simple playback button, a volume control, a track position control, or a now-playing display.

## Setting Up a Media Card

1. Select a card and change its type to **Media**.
2. Choose the media **Type**:
   - **Play/Pause Button**
   - **Previous Button**
   - **Next Button**
   - **Volume Button**
   - **Track Position**
   - **Now Playing**
3. Enter the media player entity, for example `media_player.living_room`.
4. Set a label or icon if the selected type shows those fields.

## Playback Buttons

**Play/Pause Button**, **Previous Button**, and **Next Button** send the matching Home Assistant media player action when tapped.

For Play/Pause, you can choose whether the card shows its fixed label or the live state, such as **Playing** or **Paused**.

## Volume Button

The Volume Button shows the current volume percentage. Tapping it opens a volume control popup on the panel, where you can adjust the volume without leaving the current page.

The card watches the media player's `volume_level` attribute, so it also updates when volume changes elsewhere.

## Track Position

Track Position shows playback progress and elapsed time.

- Drag the progress bar to seek within the current track.
- The card uses Home Assistant's `media_duration`, `media_position`, and `media_position_updated_at` attributes when they are available.
- You can show a fixed label or the live playback state.

Seeking depends on the media player integration. Some players expose progress but do not support seeking.

## Now Playing

Now Playing shows the media title and artist from Home Assistant.

You can choose optional controls:

- **None** shows only the current title and artist.
- **Track Position** adds a progress background and lets you seek.
- **Play/Pause** makes the card tappable so it toggles playback.

Now Playing works best on wider or larger cards because it has more room for track text.

::: info Requires Home Assistant actions
Media cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
