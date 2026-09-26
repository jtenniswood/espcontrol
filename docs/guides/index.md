---
title: Home Assistant Touchscreen Setup and Control Guides
description: Set up an EspControl room panel, media remote, or camera display with practical guides and clear hardware requirements.
---

# What Would You Like to Control?

EspControl uses devices and entities already connected to Home Assistant. Install the relevant Home Assistant integration first, then choose the matching card on your panel. For your first screen, follow **[Choose a screen](/screens/)**, **[Install](/getting-started/install)**, and **[Configure](/features/setup)**.

## Build a Room Control Panel

Start with one [Light](/card-types/lights) or [Switch](/card-types/switches) card. Enable [Home Assistant actions](/getting-started/home-assistant-actions), save the card, and check that tapping it changes the real device and that changes made in Home Assistant appear on the panel.

Add [Climate](/card-types/climate), [Fans](/card-types/fans), [Covers](/card-types/covers), or an [Action](/card-types/actions) for a scene or script. Use [subpages](/features/subpages) to group rooms, and [screensaver settings](/features/screensaver) to wake or sleep the display using a Home Assistant presence sensor.

## Make a Sonos or Home Assistant Media Remote

Use a [Media card](/card-types/media) with a `media_player` entity. Playback, volume, artwork, and seeking depend on what that player's Home Assistant integration exposes. **All Controls** opens the full player view; **Track, Album or Playlist** creates a saved playback shortcut.

For multiple speakers, follow [Speaker Groups](/features/speaker-groups) and confirm the players can join in Home Assistant first. For a dedicated album-art display, use [Media Cover Art](/features/media-cover-art). Existing users of the older controller can follow the [migration guide](/getting-started/migrate-esphome-media-player).

## Show a Camera or Doorbell Snapshot

Add a [Camera card](/card-types/cameras) for a Home Assistant `camera` or `image` entity. The panel shows still snapshots, not live video. S3 panels have two shared image slots; P4 panels have six, shared with Media Cover Art across all pages.

Check that the snapshot loads, then tap it to confirm the larger view works. For a doorbell, combine the documented [P4 camera refresh action](/card-types/cameras#refreshing-cards-from-home-assistant) with [Screen: Wake](/features/screensaver#wake-from-home-assistant) in a Home Assistant automation. A Camera card does not automatically open when someone rings.

## Show Your Immich Photos

Install [EspControl Immich Companion](/immich/) to turn your photo library, albums, Memories or keyword searches into a slideshow. Follow [Connect to EspControl](/immich/display-setup) to use its Home Assistant image entity in the photo screensaver or a Camera card, with optional photo metadata.

## Maintain or Troubleshoot a Panel

- [Back up, restore, or reset](/features/backup) before major configuration changes.
- [Update firmware](/features/firmware-updates) and distinguish stable releases from custom ESPHome builds.
- [Troubleshoot setup and controls](/getting-started/troubleshooting) if installation, networking, or commands fail.
- [Browse common questions](/reference/faq) for compatibility and feature limits.

Custom firmware, Ethernet-only networking, and web authentication are covered in [advanced ESPHome setup](/getting-started/manual-esphome-setup). Recovery and USB logs are linked from Troubleshooting when needed.
