---
title: Immich Photos
description: Display your Immich photo library on EspControl with the Home Assistant companion, including albums, memories, photo pairing and slideshow controls.
---

# Immich Photos

Bring your photo library into everyday view. EspControl Immich Companion connects **Immich**, your self-hosted photo library, to **Home Assistant**, turning selected photos into a slideshow for your EspControl display or Home Assistant dashboard.

Immich stores and organises your photos; the companion chooses what to show, sizes it for your screen and provides playback controls. Rediscover old memories, keep a family album on display or create a different slideshow for each room. Your originals stay in Immich, and the companion never edits or deletes them.

## What you can do

- **Choose your photos:** show your library, combine albums (including shared albums), revisit Memories around today's date, or search with descriptions such as “beach at sunset”.
- **Narrow the selection:** include recent photos, portrait photos, landscape photos or a mix.
- **Fit your display:** choose landscape, portrait or square output; show the whole photo against a matching background or crop to fill the screen.
- **Pair portraits:** place two photos taken around the same date side by side, with control over how close their dates must be.
- **Control playback:** pause, resume, go back, advance and set the slideshow interval from 10 seconds to 24 hours.
- **See the story:** view the photo's date, location, people, tags, rating, camera and favourite status, with links to open the originals in Immich.
- **Make each frame independent:** reuse an Immich connection with different sources and settings, and use Home Assistant entities in dashboards and automations.
- **Keep a photo on screen during outages:** retain the last compatible rendered slide when Immich is temporarily unavailable.

## Get started

You need Home Assistant and an **Immich 3.2 or later** server reachable from Home Assistant. Have your server address and a [read-only API key](/immich/installation#immich-connection-and-permissions) ready. [HACS](https://hacs.xyz/) is the recommended installation route; you can also [install manually](/immich/installation#manual-installation) without HACS.

Check the [installation status](/immich/installation#installation-status) before starting; the companion's development and releases are separate from EspControl firmware.

1. Open **HACS → ⋮ → Custom repositories**. Add `https://github.com/jtenniswood/espcontrol-immich` as an **Integration**.
2. Download **EspControl Immich Companion** and restart Home Assistant.
3. Open **Settings → Devices & services → Add integration → EspControl Immich Companion**.
4. Enter your Immich address and API key, choose a photo source and name your frame.
5. Open the frame's device page. Set **Configuration → Screen shape** to match your display.
6. Follow [Connect to EspControl](/immich/display-setup) to use the frame's **Image** entity in the photo screensaver or a Camera card.

Each frame starts with individual photos, landscape output, the full image visible and a 30-second timer. Add the integration again to create more frames. The companion supplies the image and controls; EspControl displays the image through Home Assistant.

HACS is the recommended route for Home Assistant devices and controls; MQTT and a separate add-on are not required. If you want a browser preview and HTTP API instead, use the [optional add-on](/immich/add-on). Its frames are managed separately.

## Everyday use and help

Change albums or keywords through the frame's **Configure** option under **Settings → Devices & services**. Change display settings and timing on its device page.

| Guide | What you will find |
|---|---|
| [Connect to EspControl](/immich/display-setup) | Screensaver, image sizing, metadata and Camera card setup |
| [Installation](/immich/installation) | API permissions, manual installation, image quality and connection help |
| [Using your frame](/immich/using-your-frame) | All sources, display controls, portrait pairing, photo details and automations |
| [Optional add-on](/immich/add-on) | Browser setup, extra filtering options and the HTTP API |
| [Settings reference](/immich/settings-reference) | Exact values and limits for automation and API use |

Update through HACS and restart Home Assistant; frame settings are kept. Upgrading from the older cache format requires Immich to be reachable for the first new image. See [updates and outages](/immich/using-your-frame#updates-and-outages) or [report a problem](https://github.com/jtenniswood/espcontrol-immich/issues).

For contributors: [architecture and development](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md), [entity reference](/immich/entities) and [compatibility](/immich/compatibility).

---

Adapted from the [EspControl Immich Companion documentation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/README.md), used under its [MIT license](/immich-license.txt).
