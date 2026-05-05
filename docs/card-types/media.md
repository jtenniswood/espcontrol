---
title: Media Cards
description:
  How to use media cards on your EspControl panel for playback buttons, volume, and track position.
---

# Media

::: warning Experimental
Media cards are currently behind **Developer/Experimental Features**. Open the setup page with `?developer=experimental`, then enable **Developer/Experimental Features** in the Developer settings section.
:::

Media cards control Home Assistant `media_player` entities from the panel.

## Setting Up a Media Card

1. Select a card and change its type to **Media**.
2. Choose a **Media Mode**.
3. Enter a **Media Player Entity** such as `media_player.living_room`.
4. Set a **Label** and **Icon** if you want to override the Home Assistant name.

For playback controls, add one Media card for each button you want, using the same media player entity on each card.

## Media Modes

| Mode | What it does |
| --- | --- |
| **Play/Pause Button** | Shows one normal button that sends play/pause. |
| **Previous Button** | Shows one normal button that skips to the previous track. |
| **Next Button** | Shows one normal button that skips to the next track. |
| **Volume Slider** | Shows a horizontal slider and the current volume number. Dragging it updates the number before sending the new volume when you release. |
| **Track Position** | Shows a horizontal progress slider, the current time position, and the playing or paused state. While playing, the slider keeps moving in proportion to the reported track length. Dragging it seeks within the current track when Home Assistant reports a duration. |

## Home Assistant Actions

Media cards use Home Assistant actions. If the card does not control playback, check [Home Assistant Actions](/getting-started/home-assistant-actions).

Not every media player supports every command. For example, some speakers support volume but not seeking, and some streaming sources do not report track duration.
