---
title: Using Your Immich Frame
description: Choose Immich photo sources, fit images to your display, pair portraits, control playback, view photo details and recover from outages.
---

# Using your frame

Each frame is a Home Assistant device with an **Image** entity, playback controls and photo details. [Install the integration](/immich/installation), then open its device page under **Settings → Devices & services → EspControl Immich Companion**.

## Choose what to show

Choose a source during setup or select **Configure** on an existing frame's integration entry.

| Source | What it shows |
|---|---|
| **All photos** | Photos from your Immich timeline |
| **Albums** | Photos from one or more albums; search by name and combine your own or shared albums |
| **Memories** | Immich memories around today's date, including two days before and after |
| **Keywords** | Photos matching a description such as “beach at sunset”, using Immich's smart search |

Sources include still images and exclude trashed and locked photos. Videos are not played. The companion reads your library without changing photos, albums, tags or ratings.

Albums and Keywords open their own editor; All photos and Memories save directly. Changing a source keeps the frame's name and entities. Closing the editor without saving leaves it unchanged.

New Memories frames have no fallback: if there are no matching memories, they do not switch to your whole library. Older saved memory-window and fallback choices are retained. The native interface does not offer editors for those choices, custom filters, reference-photo searches or sort order; see the [optional add-on](/immich/add-on) for those capabilities.

## Fit the screen and select photos

Use **Configuration** on the device page. Changes save immediately.

| Setting | What it does |
|---|---|
| **Screen shape** | Outputs Landscape **1280 × 800**, Portrait **800 × 1280** or Square **720 × 720**. This sets the image size, independently of which photo orientations you include. |
| **Photo fit** | **Show full image** keeps the whole photo with a dim, colour-matched background. **Crop to fit** fills the screen by trimming edges. |
| **Photo orientation** | Includes a mix, portraits only or landscapes only. Mixed also includes square photos. |
| **Photo time range** | Shows All time, the last **1, 3 or 6 months**, or **1, 2, 3, 4, 5 or 10 years**. Applies to every source and both photos in a pair; rolling date ranges use UTC. |
| **Portrait images** | Shows individual portraits, a mix of individual and paired portraits, or paired portraits only. |
| **Portrait image window** | Allows **0–7 days** between the capture dates of paired portraits. Default **2**; **0** means the same date. |

Pairs appear side by side with a thin divider and the selected fit applied to each photo. In mixed pairing mode, an unmatched portrait is always shown in full with a matching background. Paired-only mode skips portraits without a dated partner, but still allows landscapes and squares when your orientation setting permits them. For a slideshow made entirely of portrait pairs, combine **Paired portrait photos only** with **Portrait photos only**.

New frames default to landscape output, Show full image, mixed orientations, individual photos and All time. Display or source changes reload the frame, restart playback and clear Previous history. Timer changes keep the current image and history.

Full-size photos are requested for every layout, then resized to the chosen screen dimensions. If a full-size image is unavailable or cannot be decoded, the companion tries its preview. See [image quality](/immich/installation#image-quality).

## Playback, details and automations

| Control | Action |
|---|---|
| **Slideshow** | Turn off to pause; turn on to resume automatic playback |
| **Slideshow Timer** | Set **10–86,400 seconds** between slides; default **30 seconds** |
| **Next** | Resume playback and request another photo |
| **Previous** | Return to an earlier slide with its matching details; does not pause playback |
| **Clear cache** | Remove the saved disk copy; the currently displayed photo stays visible |

Previous history lasts for the current session. Restarting Home Assistant starts playback again and clears that history.

Photo sensors show **Date, Location, People, Tags, Rating, Camera** and **Favourite** for the single photo or the **left photo** in a pair. Missing details stay blank. To see the original, expand the Image entity's **Attributes → Open in Immich**; pairs also have **Open second photo in Immich**. Your browser needs access to Immich and may ask you to sign in.

Use the Image entity on a dashboard and the switch, buttons, selects and numbers in Home Assistant automations. For example, turn **Slideshow** off at bedtime and on in the morning, or connect a physical button to **Next**. Select the entities belonging to your frame; exact automation values are in the [settings reference](/immich/settings-reference).

Add the integration again for another frame, reusing a saved connection or entering a new one. Each native frame keeps its own credentials: replacing a key on one frame does not update the others, and deleting one frame leaves the rest connected.

## Updates and outages

Update in HACS, then restart Home Assistant. Settings and entity identities are preserved. Older device presets automatically become their matching screen shape; automations using retired preset values need the [new shape values](/immich/entities).

During a temporary outage, the last complete compatible slide remains visible. This is a saved slide, not an offline copy of your library. Restarts can restore it while its source, account and display settings still match; Memories and date-limited caches must also remain valid for the current date. Clearing the cache removes this restart fallback until another slide is saved.

Upgrading from the older cache format regenerates the saved image, so **Immich must be reachable for the first image after that upgrade**. If an API key stops working, use Home Assistant's reconnect prompt to replace it without recreating the frame. For connection or empty-selection problems, see [troubleshooting](/immich/installation#troubleshooting). Development-build migration and rollback guidance is in the [architecture guide](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md#settings-upgrades-and-rollback).

---

Adapted from the [EspControl Immich Companion documentation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/native-integration.md), used under its [MIT license](/immich-license.txt).
