---
title: Media Cards
description:
  How to use media cards on your EspControl panel to control Home Assistant media player entities.
---

# Media

A Media card controls a Home Assistant `media_player` entity. It can work as a simple playback button, a volume control, a track position control, or a now-playing display.

![Wide media card showing now-playing title and artist](/images/card-media.png)

## Setting Up a Media Card

1. Select a card and change its type to **Media**.
2. Choose the media **Type**:
   - **Play/Pause Button**
   - **Previous Button**
   - **Next Button**
   - **Volume Button**
   - **Track Position**
   - **Now Playing**
   - **Playlist Button**
3. Enter the media player entity, for example `media_player.living_room`.
4. Set a label or icon if the selected type shows those fields.

## Playback Buttons

**Play/Pause Button**, **Previous Button**, and **Next Button** send the matching Home Assistant media player action when tapped.

For Play/Pause, you can choose whether the card shows its fixed label or the live state, such as **Playing** or **Paused**.

## Volume Button

The Volume Button shows the current volume percentage. Tapping it opens a volume control popup on the panel, where you can adjust the volume without leaving the current page.

Set **Maximum Volume** to cap the panel control below 100%. The popup dial rescales to that maximum, so a 40% cap makes 40% the end of the arc.

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

## Playlist Button

Playlist Button is a generic shortcut for anything Home Assistant can play with `media_player.play_media`. It is not tied to any particular music service.

Enter:

- **Media Content ID / URI** - the ID or URI Home Assistant uses for the playlist, station, album, or favorite.
- **Media Content Type** - usually `playlist`, but some integrations use values such as `music`, `album`, or `channel`.

To find the right values, browse to the item in Home Assistant's media browser, then test it from Home Assistant developer tools with the `media_player.play_media` action. Once that service call starts the right media, copy the working `media_content_id` and `media_content_type` into EspControl.

::: info Requires Home Assistant actions
Media cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
