---
title: Migrate from ESPHome Media Player
description:
  How to recreate the ESPHome Media Player cover-art, playback-control, and clock screensaver experience with EspControl.
---

# Migrate from ESPHome Media Player

If you used [ESPHome Media Player](https://jtenniswood.github.io/esphome-media-player/) as a dedicated music controller, EspControl can recreate the main experience while also giving you cards for the rest of Home Assistant.

The two projects do not share their saved settings. Before changing firmware, note the Home Assistant `media_player` entity used by the old display. You will select the same entity in EspControl after following the [Install guide](/getting-started/install) and [enabling Home Assistant actions](/getting-started/home-assistant-actions).

## Choose the Experience You Want

| If you want... | Use... |
| --- | --- |
| Normal EspControl cards until media starts, then a full-screen now-playing display | [Automatic cover art](#option-1-automatic-cover-art) |
| An artwork-led home screen that always stays visible | [A large Cover Art card](#option-2-dedicated-cover-art-home-screen) |
| A clock or dark screen when the panel is inactive | [Screensaver](#match-the-old-idle-screen) |

## Option 1: Automatic Cover Art

This is the closest match to the old media-player behaviour. Your normal EspControl cards remain available, but artwork takes over the screen when the selected player starts playing.

1. Open the panel's address in a browser and select **Settings**.
2. Open **Sleep & Schedule > Media Cover Art**.
3. Turn on **Show Cover Art**.
4. Set **Media Player Entity** to the player you previously controlled, for example `media_player.living_room`.
5. Set **Show After** to **3 seconds** for the quickest transition to artwork.
6. On a 4-inch square display, set **Show Track Details For** to **Always** if you want the title, artist, time, and progress to remain visible. Choose a shorter time if you prefer artwork with less text.
7. Open **Advanced Options** and leave **Keep Screen Awake During Playback** on.

When Home Assistant supplies a track duration, EspControl also shows playback time and a progress bar. Live radio and other streams without a duration still show their available artwork and track details.

Touching the artwork returns you to the normal card screen. EspControl waits for the selected **Show After** time from your most recent touch before showing the artwork again, so you have time to use other controls.

::: tip TV and Line-in sources
Open **Advanced Options** and turn on **Hide for external source inputs** if your speaker reports `TV` or `Line-in` and you do not want its artwork screen to appear for those sources.
:::

See [Media Cover Art](/features/media-cover-art) for filtering, custom Home Assistant ports, and more detailed behaviour.

## Option 2: Dedicated Cover-Art Home Screen

This layout is best on the 4-inch square **4848S040** and **ESP32-P4 86 Panel**, where a 3 x 3 card fills the card area.

1. Open the **Screen** tab on the panel web page.
2. Remove or move any existing home-screen cards so the grid is empty.
3. Select an empty space and choose **Media > Cover Art**.
4. Enter the same Home Assistant `media_player` entity.
5. Turn on **Show Track Details** if you want the title and artist over the artwork.
6. Set **Press Action** to **All Controls**. Tapping the artwork will then open playback and volume controls.
7. Right-click the card, choose **Size**, then choose **Extra Large (3x3)**.
8. Open **Settings > Display > Clock Bar** and turn off **Show Clock Bar** so the artwork can use the full display height.
9. Wait a few seconds for the automatic update and cover art to settle.

On rectangular panels, use the largest square Cover Art card that suits the grid. It will not fill the entire rectangular screen, so automatic cover art is usually the better choice when you want a true full-screen now-playing view.

If **Cover Art** or **Extra Large (3x3)** is missing, update the panel to current EspControl firmware and reload its web page.

See [Media cards](/card-types/media#cover-art) for the other card sizes and press actions.

## Match the Old Idle Screen

Automatic cover art and the normal EspControl screensaver are separate. This lets the panel remain awake while media plays, then switch to a clock or turn off after you stop using it.

1. Open **Settings > Sleep & Schedule > Screensaver**.
2. Choose **Timer**.
3. Choose how long EspControl should wait after the last touch.
4. Choose **Clock** for the drifting clock, **Display Off** for a dark panel, or **Screen Dimmed** to leave the cards faintly visible.
5. Set the clock or dimmed brightness to a comfortable level.

Use [Night Schedule](/features/screen-schedule) as well if the panel should always dim, show a clock, or turn off during set overnight hours. See [Screensaver](/features/screensaver) for presence-sensor wake and the other available modes.

## Old and New Controls

| ESPHome Media Player | EspControl equivalent |
| --- | --- |
| Full-screen album art during playback | **Settings > Sleep & Schedule > Media Cover Art** |
| Always-visible artwork | **Media > Cover Art** card |
| Title and artist overlay | **Show Track Details For** or **Show Track Details** |
| Playback and volume panel | Cover Art **Press Action > All Controls**, or a **Media > All Controls** card |
| Clock after playback stops | **Screensaver > Timer > Clock** |
| Screen off after playback stops | **Screensaver > Timer > Display Off** |
| Scheduled overnight screen off | **Night Schedule** |

EspControl uses taps and cards instead of the old player's swipe gestures. The **All Controls** popup is the simplest replacement for play/pause, previous, next, and volume controls in one place. You can also add separate [Media cards](/card-types/media) for any control you want to keep directly on the home screen.
