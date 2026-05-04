---
title: Media Cards
description:
  How to use media cards on your EspControl panel for playback controls, volume, and track position.
---

# Media

Media cards control Home Assistant `media_player` entities from the panel.

## Setting Up a Media Card

1. Select a card and change its type to **Media**.
2. Choose a **Media Mode**.
3. Enter a **Media Player Entity** such as `media_player.living_room`.
4. Set a **Label** and **Icon** if you want to override the Home Assistant name.

## Media Modes

| Mode | What it does |
| --- | --- |
| **Playback Controls** | Shows previous, play/pause, and next controls inside one card. This works best as a **Wide** card. |
| **Volume Slider** | Shows a horizontal slider for volume. Dragging it sets the media player's volume from 0 to 100 percent. |
| **Track Position** | Shows a vertical progress slider, the current time position, and the playing or paused state. Dragging it seeks within the current track when Home Assistant reports a duration. |

## Home Assistant Actions

Media cards use Home Assistant actions. If the card does not control playback, check [Home Assistant Actions](/getting-started/home-assistant-actions).

Not every media player supports every command. For example, some speakers support volume but not seeking, and some streaming sources do not report track duration.
