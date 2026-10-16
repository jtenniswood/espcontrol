---
title: Immich Photos
description: Set up Immich Photos on EspControl with installation, display setup, slideshow controls, settings, Home Assistant entities and the optional browser API.
---

# Immich Photos

## Overview

Bring your photo library into everyday view. EspControl Immich Companion connects **Immich**, your self-hosted photo library, to **Home Assistant**, turning selected photos into a slideshow for your EspControl display or Home Assistant dashboard.

Immich stores and organises your photos; the companion chooses what to show, sizes it for your screen and provides playback controls. Rediscover old memories, keep a family album on display or create a different slideshow for each room. Your originals stay in Immich, and the companion never edits or deletes them.

### What you can do

- **Choose your photos:** show your library, combine albums (including shared albums), revisit Memories around today's date, or search with descriptions such as “beach at sunset”.
- **Narrow the selection:** include recent photos, portrait photos, landscape photos or a mix.
- **Fit your display:** choose landscape, portrait or square output; show the whole photo against a matching background or crop to fill the screen.
- **Pair portraits:** place two photos taken around the same date side by side, with control over how close their dates must be.
- **Control playback:** pause, resume, go back, advance and set the slideshow interval from 10 seconds to 24 hours.
- **See the story:** view the photo's date, location, people, tags, rating, camera and favourite status, with links to open the originals in Immich.
- **Make each frame independent:** reuse an Immich connection with different sources and settings, and use Home Assistant entities in dashboards and automations.
- **Keep a photo on screen during outages:** retain the last compatible rendered slide when Immich is temporarily unavailable.

### Get started

You need Home Assistant and an **Immich 3.2 or later** server reachable from Home Assistant. Have your server address and a [read-only API key](/immich/#immich-connection-and-permissions) ready. [HACS](https://hacs.xyz/) is the recommended installation route; you can also [install manually](/immich/#manual-installation) without HACS.

Check the [installation status](/immich/#installation-status) before starting; the companion's development and releases are separate from EspControl firmware.

1. Open **HACS → ⋮ → Custom repositories**. Add `https://github.com/jtenniswood/espcontrol-immich` as an **Integration**.
2. Download **EspControl Immich Companion** and restart Home Assistant.
3. Open **Settings → Devices & services → Add integration → EspControl Immich Companion**.
4. Enter your Immich address and API key, choose a photo source and name your frame.
5. Open the frame's device page. Set **Configuration → Screen shape** to match your display.
6. Follow [Connect to EspControl](/immich/#connect-to-espcontrol) to use the frame's **Image** entity in the photo screensaver or a Camera card.

Each frame starts with individual photos, landscape output, the full image visible and a 30-second timer. Add the integration again to create more frames. The companion supplies the image and controls; EspControl displays the image through Home Assistant.

HACS is the recommended route for Home Assistant devices and controls; MQTT and a separate add-on are not required. If you want a browser preview and HTTP API instead, use the [optional add-on](/immich/#browser-api-add-on). Its frames are managed separately.

### Everyday use and help

Change albums or keywords through the frame's **Configure** option under **Settings → Devices & services**. Change display settings and timing on its device page.

| Guide | What you will find |
|---|---|
| [Connect to EspControl](/immich/#connect-to-espcontrol) | Screensaver, image sizing, metadata and Camera card setup |
| [Installation](/immich/#installation) | API permissions, manual installation, image quality and connection help |
| [Using your frame](/immich/#using-your-frame) | All sources, display controls, portrait pairing, photo details and automations |
| [Optional add-on](/immich/#browser-api-add-on) | Browser setup, extra filtering options and the HTTP API |
| [Settings reference](/immich/#settings-reference) | Exact values and limits for automation and API use |

Update through HACS and restart Home Assistant; frame settings are kept. Upgrading from the older cache format requires Immich to be reachable for the first new image. See [updates and outages](/immich/#updates-and-outages) or [report a problem](https://github.com/jtenniswood/espcontrol-immich/issues).

For contributors: [architecture and development](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md) and [entity reference](/immich/#home-assistant-entities).

## Installation

Use the Home Assistant integration for native frame devices, dashboards and automations. It runs within Home Assistant and does not need MQTT or a separate container. You need an **Immich 3.2 or later** server reachable from Home Assistant, plus a read-only API key.

For the separate browser/API experience, follow the [optional add-on guide](/immich/#browser-api-add-on).

### Installation status

As of **September 22, 2026**, the companion has no published GitHub releases. The guides in this section describe its documented development features. Check the [companion releases](https://github.com/jtenniswood/espcontrol-immich/releases) for availability; if HACS does not offer a download, use the manual development installation below. The companion's release status is separate from stable EspControl firmware.

### HACS installation

1. Open **HACS → ⋮ → Custom repositories**.
2. Add `https://github.com/jtenniswood/espcontrol-immich` with category **Integration**.
3. Find **EspControl Immich Companion**, download it and restart Home Assistant.
4. Open **Settings → Devices & services → Add integration → EspControl Immich Companion**.
5. Enter your Immich server address and API key, choose a photo source and name your frame.
6. Open the frame's device page and select **Configuration → Screen shape** to match your display.

Next, [connect the frame to EspControl](/immich/#connect-to-espcontrol).

See [Using your frame](/immich/#using-your-frame) for all controls. Add the integration again for more frames. For updates, download the new version in HACS and restart Home Assistant; frame settings are retained.

### Manual installation

Download your chosen revision from the [companion repository](https://github.com/jtenniswood/espcontrol-immich) using **Code → Download ZIP**, or use a published release when available. Copy the entire `custom_components/immich_frames` directory into `<config>/custom_components/immich_frames`, where `<config>` contains Home Assistant's `configuration.yaml`. Restart Home Assistant and add the integration as above.

The same process works for a development branch. Back up the existing integration and Home Assistant configuration before replacing it, and keep only one installed copy. See [migration and rollback](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md#settings-upgrades-and-rollback) when testing an upgrade.

### Immich connection and permissions

Use the Immich server's base address, for example `https://photos.example.com`, and an API key for the account whose photos you want to display. Keep the key private; it grants access to that account's photos. HTTPS connections use certificate verification.

Give the key these read permissions for the features you use:

| Permission | Used for |
|---|---|
| `asset.read` | Connection check, photo searches and metadata |
| `asset.view` | Image requests and preview fallback |
| `asset.download` | Full-quality originals; recommended for best image quality |
| `album.read` | Browsing and selecting albums |
| `memory.read` | Memories source |
| `person.read`, `tag.read` | People and tag catalogs in the optional add-on API |

Setup checks the server version and verifies the key by searching for photos. An empty library can still pass the connection check. Shared albums must be accessible to the account that owns the key.

### Image quality

Every layout requests full-size photos before resizing them to your screen, including both halves of a pair and unmatched portraits. Rendered images use high-quality JPEG output (quality 95 with full colour detail).

Full-size downloads use more bandwidth and can take longer on slower connections. Allow `asset.download` for originals. Formats such as HEIC or RAW may need a compatible full-size conversion from Immich. If a full-size request fails or its image cannot be decoded, the companion tries the preview; Immich may also supply a preview when a full-size conversion is unavailable.

Improved quality appears as new slides are rendered; you do not need to clear the cache. If fallback photos look soft, check Immich's preview quality and image-generation settings.

### Troubleshooting

| Problem | What to check |
|---|---|
| Cannot connect | Check the base address, server version and network access **from Home Assistant**. For HTTPS, check the certificate. |
| Invalid API key | Check the key and `asset.read` permission. For an existing frame, use Home Assistant's reconnect prompt; repeat for other frames using the old key. |
| Albums missing | Check `album.read` and access from the key's Immich account. Create or share an album, then choose Albums again to retry. |
| No matching photos | Try All photos, All time, Mixed orientations and individual photos, then narrow the selection. Memories may be empty; paired-only portraits need a partner within the date window. |
| Keywords return nothing | Try a broader description and check that smart search works in Immich. |
| Photo will not change | Check that Slideshow is on and the timer is suitable. Next resumes playback. If Immich is unreachable, the last saved photo may remain visible. |
| Photo details are blank | Details come from Immich and may not exist for that photo. For pairs, sensors describe the left photo. |

If you still need help, [report a problem](https://github.com/jtenniswood/espcontrol-immich/issues) with your integration and Immich versions, installation method, selected source and relevant logs. Remove API keys and other private details before sharing logs.

## Connect to EspControl

Use the **EspControl Immich Companion** HACS integration to turn your Immich library into a photo screensaver. The companion selects and prepares photos, Home Assistant exposes the frame's **Image** entity, and EspControl displays it.

### Before you start

- [Install the companion](/immich/#installation) and create a frame. Use the HACS integration for this setup; the optional add-on alone does not create Home Assistant entities.
- Confirm the frame's **Image** entity shows a photo in Home Assistant. On the frame's device page, open the entity and copy its actual `image.*` entity ID.
- Connect your EspControl panel to the same Home Assistant instance. The Camera screensaver supports ESP32-P4 panels and the **4-inch 4848S040 ESP32-S3** panel. Update the [panel firmware](/settings/maintenance#firmware) if its Camera controls are missing.

### Match the image to your display

On the companion frame's Home Assistant device page, open **Configuration → Screen shape** and choose the shape that matches the panel's current [rotation](/settings/display#rotation):

| Panel orientation | Companion screen shape | Image output |
|---|---|---|
| Landscape | Landscape | 1280 × 800 |
| Portrait | Portrait | 800 × 1280 |
| Square | Square | 720 × 720 |

These are the companion's output sizes; EspControl scales the image to its own screen. For example, the square output is resized for the 480 × 480 4848S040 panel.

Start with **Photo fit → Show full image** in the companion to keep the whole photo against a matching background. Choose **Crop to fit** if you prefer to fill the frame by trimming the edges. Photo orientation and [portrait pairing](/immich/#fit-the-screen-and-select-photos) control which photos appear independently of the screen shape.

### Use the photo screensaver

1. Open the EspControl panel's web setup page.
2. Go to **Screen → Sleep & Schedule → Screensaver**.
3. Select **Timer** and choose an inactivity delay, or select **Sensor** and enter your Home Assistant presence sensor.
4. Set **Then → Camera**. This option accepts both cameras and photo image entities.
5. Enter the companion frame's `image.*` entity ID in **Camera Entity**.
6. Under **Expanded Image**, choose **Show full image** (Fit) to keep the complete companion image visible on the full screen, or **Crop to fit** (Fill) to fill the screen by trimming the edges. This is a second sizing step after the companion's own Photo fit setting.
7. Finish editing the entity field so the setting saves, then let the screensaver activate. Settings save automatically.

The companion's **Slideshow Timer** controls how often the photo changes, while the EspControl screensaver timer controls when the panel starts showing photos. The companion starts with a 30-second slideshow interval. EspControl refreshes the image when Home Assistant reports an update and retains the previous successful image while downloading the next one.

Touch the screen to return to the panel. See [Screensaver](/settings/sleep#screensaver) for presence wake, brightness, the optional clock overlay and Night Schedule priority.

### Add photo details

With **Then → Camera** selected, enable **Display Metadata** and enter one of the companion's `sensor.*` entities in **Photo Metadata Entity**. For example, use the frame's **Date** or **Location** sensor. Copy the actual entity ID from Home Assistant; names depend on your frame.

The panel accepts one metadata sensor. The companion also provides People, Tags, Rating, Camera and Favourite sensors, which you can use in Home Assistant. For paired portraits, these sensors describe the **left photo**. Missing details are hidden on the panel. See [Photo metadata](/settings/sleep#photo-metadata) for positioning and clock-overlay behaviour.

### Use a Camera card

To keep photos alongside your room controls, add a [Camera card](/card-types/cameras) and select the same companion `image.*` entity. Tap the card to open its larger image view. The screensaver and card can use the same frame, or you can create independent companion frames for different photo selections.

### Playback and a first check

Open the frame's device page in Home Assistant to pause with **Slideshow**, advance with **Next**, or revisit a slide with **Previous**. Next resumes playback; Previous does not pause it. These controls also work in Home Assistant dashboards and automations. See [Using your frame](/immich/#playback-details-and-automations).

To check the setup, wait for a photo to appear on the panel, press **Next** in Home Assistant, and confirm the panel updates. If metadata is enabled, confirm it matches the displayed photo. Check that tapping the screensaver wakes the panel.

If no photo appears, check the Image entity in Home Assistant first and follow [companion troubleshooting](/immich/#troubleshooting). If it works there, recheck the panel's Camera Entity, firmware support and Home Assistant connection. If photos look soft, review [image quality and original-download permissions](/immich/#image-quality).

## Using Your Frame

Each frame is a Home Assistant device with an **Image** entity, playback controls and photo details. [Install the integration](/immich/#installation), then open its device page under **Settings → Devices & services → EspControl Immich Companion**.

### Choose what to show

Choose a source during setup or select **Configure** on an existing frame's integration entry.

| Source | What it shows |
|---|---|
| **All photos** | Photos from your Immich timeline |
| **Albums** | Photos from one or more albums; search by name and combine your own or shared albums |
| **Memories** | Immich memories around today's date, including two days before and after |
| **Keywords** | Photos matching a description such as “beach at sunset”, using Immich's smart search |

Sources include still images and exclude trashed and locked photos. Videos are not played. The companion reads your library without changing photos, albums, tags or ratings.

Albums and Keywords open their own editor; All photos and Memories save directly. Changing a source keeps the frame's name and entities. Closing the editor without saving leaves it unchanged.

New Memories frames have no fallback: if there are no matching memories, they do not switch to your whole library. Older saved memory-window and fallback choices are retained. The native interface does not offer editors for those choices, custom filters, reference-photo searches or sort order; see the [optional add-on](/immich/#browser-api-add-on) for those capabilities.

### Fit the screen and select photos

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

Full-size photos are requested for every layout, then resized to the chosen screen dimensions. If a full-size image is unavailable or cannot be decoded, the companion tries its preview. See [image quality](/immich/#image-quality).

### Playback, details and automations

| Control | Action |
|---|---|
| **Slideshow** | Turn off to pause; turn on to resume automatic playback |
| **Slideshow Timer** | Set **10–86,400 seconds** between slides; default **30 seconds** |
| **Next** | Resume playback and request another photo |
| **Previous** | Return to an earlier slide with its matching details; does not pause playback |
| **Clear cache** | Remove the saved disk copy; the currently displayed photo stays visible |

Previous history lasts for the current session. Restarting Home Assistant starts playback again and clears that history.

Photo sensors show **Date, Location, People, Tags, Rating, Camera** and **Favourite** for the single photo or the **left photo** in a pair. Missing details stay blank. To see the original, expand the Image entity's **Attributes → Open in Immich**; pairs also have **Open second photo in Immich**. Your browser needs access to Immich and may ask you to sign in.

Use the Image entity on a dashboard and the switch, buttons, selects and numbers in Home Assistant automations. For example, turn **Slideshow** off at bedtime and on in the morning, or connect a physical button to **Next**. Select the entities belonging to your frame; exact automation values are in the [settings reference](/immich/#settings-reference).

Add the integration again for another frame, reusing a saved connection or entering a new one. Each native frame keeps its own credentials: replacing a key on one frame does not update the others, and deleting one frame leaves the rest connected.

### Updates and outages

Update in HACS, then restart Home Assistant. Settings and entity identities are preserved. Older device presets automatically become their matching screen shape; automations using retired preset values need the [new shape values](/immich/#home-assistant-entities).

During a temporary outage, the last complete compatible slide remains visible. This is a saved slide, not an offline copy of your library. Restarts can restore it while its source, account and display settings still match; Memories and date-limited caches must also remain valid for the current date. Clearing the cache removes this restart fallback until another slide is saved.

Upgrading from the older cache format regenerates the saved image, so **Immich must be reachable for the first image after that upgrade**. If an API key stops working, use Home Assistant's reconnect prompt to replace it without recreating the frame. For connection or empty-selection problems, see [troubleshooting](/immich/#troubleshooting). Development-build migration and rollback guidance is in the [architecture guide](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md#settings-upgrades-and-rollback).

## Settings Reference

These values come from the companion's [settings contract](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/custom_components/immich_frames/core/settings.py). Use them when configuring Home Assistant automations or the optional add-on API.

| Control | Service values, in order | Default | Effect |
|---|---|---|---|
| Photo time range | `all_time` — All time; `1_month` — Last 1 month; `3_months` — Last 3 months; `6_months` — Last 6 months; `1_year` — Last 1 year; `2_years` — Last 2 years; `3_years` — Last 3 years; `4_years` — Last 4 years; `5_years` — Last 5 years; `10_years` — Last 10 years | `all_time` | selection |
| Screen shape | `landscape` — Landscape (1280 × 800); `portrait` — Portrait (800 × 1280); `square` — Square (720 × 720) | `landscape` | render |
| Photo fit | `crop` — Crop to fit; `show_full` — Show full image | `show_full` | render |
| Portrait images | `single` — Single portrait photos only; `pairs` — Single and Paired portrait photos; `pairs_only` — Paired portrait photos only | `single` | selection |
| Photo orientation | `any` — Mixed (landscapes and portraits); `portrait` — Portrait photos only; `landscape` — Landscape photos only | `any` | selection |
| Portrait image window | 0–7 | `2` | selection |
| Slideshow Timer | 10–86400 | `30` | timer |

Screen outputs are exactly Landscape 1280 × 800, Portrait 800 × 1280, and Square 720 × 720.

Square-only photo selection is retained for old saved configurations, but is not offered as a new native control choice. Square photos remain included in Mixed.

Setup and Configure edit the photo source; the device page owns these display and timing controls.

### Photo sources

| Source | Saved value | Fields |
|---|---|---|
| All photos | `all` | None |
| Albums | `album` | album_ids |
| Memories | `memories` | memory_window_days, fallback_to_all |
| Keywords | `smart` | smart_query, smart_reference_asset_id |
| Custom filter | `filter` | filter |

Timer changes reschedule playback without invalidating photos. Render and selection changes invalidate incompatible snapshots.

## Home Assistant Entities

Each configured frame is one native Home Assistant device. Entity IDs are generated from the config entry identifier, so renaming a frame does not break automations.

| Entity | Type | Purpose |
|---|---|---|
| Image | Image | Final rendered single image or pair; clickable Immich links in its attributes |
| Date/Location/People/Tags/Rating/Camera | Sensors | Details for the single photo, or the left photo in a pair |
| Favourite | Sensor | Yes if the single photo (or left photo in a pair) is a favourite in Immich, No if it is not; blank when the status is missing |
| Slideshow | Switch | Pause or resume automatic advancement |
| Screen shape | Select (Configuration) | Choose Landscape (1280 × 800), Portrait (800 × 1280), or Square (720 × 720); saves the choice and reloads the frame |
| Photo fit | Select (Configuration) | Crop to fit or Show full image |
| Portrait images | Select (Configuration) | Single portrait photos only; Single and Paired portrait photos; Paired portrait photos only |
| Photo time range | Select (Configuration) | All time, or the last 1, 3 or 6 months; 1, 2, 3, 4, 5 or 10 years |
| Photo orientation | Select (Configuration) | Mixed, portrait or landscape photos; legacy square-only values are retained |
| Portrait image window | Number (Configuration) | 0–7 days between paired portraits; default 2 days; 0 means the same date |
| Slideshow Timer | Number (Controls) | 10–86,400 seconds |
| Next, Previous, Clear cache | Buttons | Manual frame controls |

The Image entity exposes `open_in_immich` for the single/left photo and `open_second_photo_in_immich` only when a right photo exists. Each is an Immich web URL (`/photos/<asset-id>`) without the API key. Links use the same snapshot as the image, including cached slides and Previous navigation.

The image and metadata entities update from the same coordinator snapshot. The image is replaced only after every selected photo has been downloaded and rendered. During a temporary Immich outage the last complete cached slide remains available.

Photo detail sensors always use the single photo or the left photo in a pair. Existing frames have the old photo-details selector removed automatically when the integration loads, including when Immich is offline.

Screen shape offers the service values `landscape`, `portrait`, and `square`. The existing entity ID is preserved. Saved device presets migrate automatically: `jc1060p470` becomes `landscape`, `jc4880p443` becomes `portrait`, and `4848s040` becomes `square`. Automations that select a retired device value must use its corresponding shape value. See the [screen shapes](/immich/#using-your-frame) for output dimensions.

Photo and pairing settings save immediately and reload the frame. The interval updates the slideshow timer directly. All settings share the same saved values as Configure and survive Home Assistant restarts. Cached photos are restored only when their photo rules and rendering settings still match.

The exact control values and order are checked against [the settings reference](/immich/#settings-reference). Cache restoration also requires matching source/account rules and a complete verified slide record.

## Browser & API Add-on

The add-on turns Immich photos into slides you can preview in a browser or retrieve as JPEGs through an HTTP API. Use it when you want to build your own display client or manage frames outside Home Assistant's entity controls.

For Home Assistant devices, dashboards and automations, use the [HACS integration](/immich/#installation). Installing the add-on alone does not create native entities, and its frames and connections are separate from those in the integration.

### Add-on setup

The add-on supports **amd64** and **aarch64** systems and requires **Immich 3.2 or later**.

1. In Home Assistant, open **Settings → Apps → App store → Repositories** and add `https://github.com/jtenniswood/espcontrol-immich`.
2. Install **EspControl Immich Companion**, start it and open its web UI.
3. Under **Immich connection**, enter a name, server address and [read-only API key](/immich/#immich-connection-and-permissions), then choose **Connect**.
4. Name your frame, select a connection and photo source, choose display settings and select **Create frame**.
5. Use the frame's preview and **Previous**, **Pause/Resume** and **Next** buttons.

You can add multiple connections and create independent frames for each. The browser page supports creation and playback; editing, deleting and transferring frame settings use the API below.

### Photo and display options

The creation form offers **All photos**, multiple **Albums**, **Memories** and **Keywords**, plus the same screen shapes, photo fit, orientation, time range, portrait pairing and timer settings described in [Using your frame](/immich/#fit-the-screen-and-select-photos).

It also offers:

- **Memory window:** 0–7 days on either side of today; default 2. Zero uses today's memories only.
- **Memory fallback:** optionally use your normal photo filter when the memory search returns no candidates. Orientation and time-range rules still apply.
- **Order:** Random, Newest first or Oldest first for normal photo searches. Keywords uses Immich's search results; Memories uses date-ordered results.

Full-size images, preview fallback and saved-slide recovery work as described in [image quality](/immich/#image-quality) and [updates and outages](/immich/#updates-and-outages).

Optional add-on configuration values:

| Option | Purpose |
|---|---|
| `immich_url`, `immich_api_key` | Seed a default connection on first use; leave empty to connect in the browser. Later changes do not replace an already saved connection. |
| `cache_limit_mb` | Total disk cache budget; default 256 MB, minimum effective value 16 MB. Older saved slides are removed when the budget is exceeded. |

### Advanced selection through the API

Send JSON to the frame creation or update endpoints. Alongside the [standard settings](/immich/#settings-reference), the API accepts a `filter` for favourites, people, tags, locations, dates, ratings, camera details, filenames and OCR text. Rules combine with the source and time range; `or` supports alternatives. Album, person and tag rules use IDs from the catalog endpoints.

For example, create a frame showing favourite photos rated at least four:

```json
{
  "name": "Favourites",
  "connection_id": "YOUR_CONNECTION_ID",
  "source": "filter",
  "filter": {
    "isFavorite": {"eq": true},
    "rating": {"gte": 4}
  }
}
```

Filters default to timeline photos. An explicit `visibility` rule can include archived or hidden photos; locked and trashed assets and videos remain excluded. See the [supported filter fields and operators](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/custom_components/immich_frames/core/filtering.py) for the full list.

For similar-photo searches, use `source: "smart"` with `smart_reference_asset_id`; `smart_query` supplies search text. For normal searches, `order_direction` accepts `random`, `asc` or `desc`, and `order_field` accepts `fileCreatedAt` (default), `localDateTime`, `fileSizeInBytes` or `rating`. The timer field is `slideshow_interval`, in seconds.

### HTTP API

Paths are relative to the add-on's web UI. Home Assistant manages ingress access. A standalone container serves port **8099** without its own login, so keep it on a trusted network.

| Request | Purpose |
|---|---|
| `GET /api/connections` | List saved connections without keys |
| `POST /api/connections` | Validate and save `name`, `url`, `api_key`; supply an existing `id` to replace that connection |
| `DELETE /api/connections/{id}` | Remove an unused connection; a configured default cannot be deleted |
| `GET /api/catalog/{kind}?connection_id=ID` | List `albums`, `people`, `tags` or `memories` |
| `GET /api/capabilities?connection_id=ID` | Report server version and supported search capabilities |
| `GET /api/frames` | List frames |
| `POST /api/frames` | Create a frame; returns its `frame_id` |
| `PUT /api/frames/{id}` | Update supplied frame settings |
| `DELETE /api/frames/{id}` | Delete a frame |
| `GET /api/frames/{id}/state` | Current photo metadata, playback state and versioned `image_url` |
| `GET /api/frames/{id}/image` | Current rendered JPEG |
| `POST /api/frames/{id}/commands/{command}` | Run `next`, `previous`, `pause`, `resume` or `clear_cache` |
| `POST /api/frames/{id}/refresh` | Refresh a frame, respecting pause |
| `GET /api/export` | Export frame settings without connection credentials |
| `POST /api/import` | Import a `frames` array in the export format; matching frame IDs are updated |
| `GET /api/health` | Check service status and version |

For matching image and metadata, fetch state and then resolve its `image_url` relative to the web UI root. If that image returns **409**, fetch state again: the requested slide is no longer retained. Before the first image, state and unversioned image requests return **503**; unknown frames return **404**.

Replacing a connection updates all add-on frames that use it. When importing elsewhere, create the connections first and match their IDs in the imported frames. Exports do not include API keys, so they are not a complete backup. Back up the add-on's data for a full restore; see [migration and rollback](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md#settings-upgrades-and-rollback).

---

Adapted from the EspControl Immich Companion documentation: [Overview](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/README.md), [Installation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/installation.md), [Using your frame](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/native-integration.md), [Settings reference](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/settings-reference.md), [Home Assistant entities](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/entity-contract.md), [Browser and API add-on](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/container.md), used under its [MIT license](/immich-license.txt).
