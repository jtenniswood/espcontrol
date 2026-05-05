---
title: Media Cards
description:
  How to use media cards on your EspControl panel for playback buttons, volume, track position, and now-playing details.
---

# Media

::: warning Experimental
Media cards are currently behind **Developer/Experimental Features**. Open the setup page with `?developer=experimental`, then enable **Developer/Experimental Features** in the Developer settings section.
:::

Media cards control or display Home Assistant `media_player` entities from the panel.

## Setting Up a Media Card

1. Select a card and change its type to **Media**.
2. Choose a **Media Mode**.
3. Enter a **Media Player Entity** such as `media_player.living_room`.
4. Set a **Label** and **Icon** if you want to override the Home Assistant name.
5. For **Play/Pause Button**, choose whether the card displays the label, such as `Office`, or the player state, such as `Playing`.

For playback controls, add one Media card for each button you want, using the same media player entity on each card.

## Media Modes

| Mode | What it does |
| --- | --- |
| **Play/Pause Button** | Shows one normal button that sends play/pause. It can display either the card label or the current player state. |
| **Previous Button** | Shows one normal button that skips to the previous track. |
| **Next Button** | Shows one normal button that skips to the next track. |
| **Volume Slider** | Shows a horizontal slider and the current volume percentage. Dragging it updates the percentage before sending the new volume when you release. |
| **Track Position** | Shows a horizontal progress slider, the current time position, and the playing or paused state. While playing, the slider keeps moving in proportion to the reported track length. Dragging it seeks within the current track when Home Assistant reports a duration. |
| **Now Playing** | Shows the current track title using the large value font, with the artist underneath using the normal card label font. |

## Home Assistant Actions

Media cards use Home Assistant actions. If the card does not control playback, check [Home Assistant Actions](/getting-started/home-assistant-actions).

Not every media player supports every command or attribute. For example, some speakers support volume but not seeking, and some streaming sources do not report track duration, title, or artist.
