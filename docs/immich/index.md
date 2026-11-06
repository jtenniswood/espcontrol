---
title: Immich Photos
description: Connect your Immich library to EspControl with a photo screensaver, playback controls and photo details through Home Assistant.
---

# Immich Photos

<span id="overview"></span>
<span id="what-you-can-do"></span>

EspControl Immich Companion turns your Immich library into a slideshow for EspControl or Home Assistant. Choose albums, memories or keyword searches, pair portraits and show photo details. Your originals stay unchanged in Immich.

<span id="get-started"></span>

## Installation

You need **Home Assistant**, an **Immich 3.2 or later** server reachable from it, and a read-only API key. The integration provides Home Assistant entities; MQTT and a separate add-on are not required.

<span id="installation-status"></span>
<span id="hacs-installation"></span>

1. Open **HACS → ⋮ → Custom repositories** and add `https://github.com/jtenniswood/espcontrol-immich` as an **Integration**.
2. Download **EspControl Immich Companion** and restart Home Assistant.
3. Open **Settings → Devices & services → Add integration → EspControl Immich Companion**.
4. Enter your Immich server address and API key, choose a photo source and name your frame.

Check the [companion releases](https://github.com/jtenniswood/espcontrol-immich/releases) for availability. If HACS offers no download, use the manual route below. Companion releases are separate from EspControl firmware.

### Manual installation

Download a release or chosen revision from the [companion repository](https://github.com/jtenniswood/espcontrol-immich). Copy `custom_components/immich_frames` into `<config>/custom_components/immich_frames`, where `<config>` contains Home Assistant's `configuration.yaml`. Restart Home Assistant and add the integration as above. Back up an existing installation before replacing it and keep only one installed copy.

### Immich connection and permissions

Use your server's base address, such as `https://photos.example.com`, and an API key for the account whose photos you want to show. Keep the key private. HTTPS certificates are verified, and shared albums must be accessible to that account.

| Permission | Needed for |
|---|---|
| `asset.read` | Connection checks, searches and metadata |
| `asset.view` | Images and preview fallback |
| `asset.download` | Full-quality originals; recommended |
| `album.read` | Albums |
| `memory.read` | Memories |
| `person.read`, `tag.read` | People and tag catalogs in the optional API |

## Connect to EspControl

<span id="before-you-start"></span>

On the frame's Home Assistant device page, confirm its **Image** entity shows a photo and copy the actual `image.*` entity ID. Connect the panel to the same Home Assistant instance.

The Camera screensaver supports **ESP32-P4 panels** and the **4-inch 4848S040 ESP32-S3**. [Update the firmware](/settings/system#firmware) if Camera controls are missing.

### Match the image to your display

Set **Configuration → Screen shape** on the frame's device page to match the panel's current [rotation](/settings/display#rotation):

| Shape | Companion output |
|---|---|
| Landscape | 1280 × 800 |
| Portrait | 800 × 1280 |
| Square | 720 × 720 |

EspControl scales this output to the panel's resolution. **Photo fit → Show full image** keeps the whole photo against a matching background; **Crop to fit** trims the edges to fill the frame.

### Use the photo screensaver

1. Open the panel's web setup and go to **Screen → Sleep & Schedule → Screensaver**.
2. Choose **Timer** with an inactivity delay, or **Sensor** with a presence sensor.
3. Set **Then → Camera** and enter the frame's `image.*` ID in **Camera Entity**.
4. Under **Expanded Image**, choose **Show full image** (Fit) or **Crop to fit** (Fill). This sizes the companion's finished image on the panel.
5. Finish editing the field to save automatically, then let the screensaver activate.

The companion's **Slideshow Timer** changes the photo; the panel's timer determines when the screensaver starts. Touch the screen to wake it. See [Screensaver](/settings/sleep#screensaver) for clock overlays and Night Schedule behaviour.

### Add photo details

Enable **Display Metadata** and set **Photo Metadata Entity** to one companion `sensor.*` entity, such as **Date** or **Location**. Copy its actual ID from Home Assistant. The panel accepts one sensor and hides missing details; paired photos use details from the **left photo**. See [Photo metadata](/settings/sleep#photo-metadata) for positioning.

### Use a Camera card

Add a [Camera card](/card-types/cameras) with the same `image.*` entity to show photos alongside your controls. Tap it for a larger view. Cards and screensavers can share a frame or use independent frames.

<span id="playback-and-a-first-check"></span>

**Check the setup:** press **Next** on the Home Assistant device page and confirm the panel updates, any metadata matches, and touching the screensaver wakes the panel.

## Using your frame

<span id="everyday-use-and-help"></span>

Open **Settings → Devices & services → EspControl Immich Companion**. Use **Configure** on the integration entry to change sources, and the frame's device page for display settings and playback.

### Choose what to show

| Source | Photos included |
|---|---|
| **All photos** | Your Immich timeline |
| **Albums** | One or more of your own or shared albums |
| **Memories** | Memories around today, including two days before and after |
| **Keywords** | Smart-search results for a description such as “beach at sunset” |

Videos, locked and trashed photos are excluded. New Memories frames stay empty if no memories match; older saved fallback choices are retained. Changing sources keeps the frame's name and entities.

### Fit the screen and select photos

Alongside Screen shape and Photo fit, **Configuration** offers:

| Setting | Choices |
|---|---|
| **Photo orientation** | Mixed (including squares), portraits or landscapes |
| **Photo time range** | All time; last 1, 3 or 6 months; last 1, 2, 3, 4, 5 or 10 years |
| **Portrait images** | Individual portraits, individual and paired, or paired only |
| **Portrait image window** | 0–7 days between capture dates; default 2, with 0 meaning the same date |

For a slideshow entirely of portrait pairs, combine **Paired portrait photos only** with **Portrait photos only**. Paired-only mode otherwise still allows landscapes and squares. Unmatched portraits appear in full in mixed pairing mode and are skipped in paired-only mode. Time ranges use UTC and apply to both photos.

New frames use landscape output, Show full image, mixed orientations, individual photos and All time. Source or display changes save immediately, restart playback and clear Previous history. Timer changes keep the current image and history.

### Playback, details and automations

| Control | Action |
|---|---|
| **Slideshow** | Pause or resume |
| **Slideshow Timer** | 10 seconds to 24 hours; default 30 seconds |
| **Next** | Request another photo and resume playback |
| **Previous** | Revisit a slide and its details without pausing |
| **Clear cache** | Remove the saved disk copy; the current photo stays visible |

History lasts until Home Assistant restarts. Use these controls in dashboards and automations, such as pausing at bedtime or connecting a button to Next.

<span id="home-assistant-entities"></span>

The **Image** entity includes **Open in Immich** and, for pairs, **Open second photo in Immich** attributes. Your browser may require an Immich login. Sensors provide **Date, Location, People, Tags, Rating, Camera** and **Favourite** for the single or left photo; missing values stay blank. Images, details and links update together.

Add the integration again for another frame. Native frames have independent settings and credentials: replacing a key on one does not update the others. Renaming a frame preserves its entity IDs.

### Image quality

The companion requests full-size photos, resizes them and renders a high-quality JPEG. Give the API key `asset.download` permission for originals. If the original or a compatible HEIC/RAW conversion is unavailable, it tries a preview. Soft fallback images may need better preview-generation settings in Immich; new slides pick up improvements without clearing the cache.

### Updates and outages

Update through HACS and restart Home Assistant; settings and entity IDs are retained. Older device presets migrate to screen shapes, but automations selecting those old values need updating: `jc1060p470` → `landscape`, `jc4880p443` → `portrait`, `4848s040` → `square`.

During an outage, the last complete compatible slide can remain visible and survive a restart. This saves one slide, not your library. Its account, source and display rules must still match, and Memories or date-limited photos must remain valid. Clear cache removes this restart fallback until another slide is saved.

**Upgrading from the older cache format requires Immich to be reachable for the first new image.** See [migration and rollback](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md#settings-upgrades-and-rollback) for development-build upgrades.

## Troubleshooting

| Problem | Check |
|---|---|
| Cannot connect | Server address, Immich version, HTTPS certificate and network access **from Home Assistant** |
| Invalid key | Key and `asset.read`; use Home Assistant's reconnect prompt for each affected frame |
| Missing albums | `album.read` and the key account's album access; select Albums again to retry |
| No photos | Start with All photos, All time, Mixed orientations and individual photos, then narrow the selection |
| Empty keyword results | Try broader terms and check smart search in Immich |
| Photo never changes | Slideshow switch, timer and Immich availability; Next resumes playback |
| Panel shows nothing | Check the Image entity in Home Assistant first, then the panel's entity ID, firmware and connection |
| Blank details | The photo may lack metadata; pairs use the left photo |

[Report a problem](https://github.com/jtenniswood/espcontrol-immich/issues) with versions, installation method, source and relevant logs. Remove keys and private details.

## Settings reference

For automations, select the entities belonging to your frame. These are the saved values; see the [full settings reference](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/settings-reference.md) for the contract.

| Control | Values |
|---|---|
| Screen shape | `landscape`, `portrait`, `square` |
| Photo fit | `show_full`, `crop` |
| Portrait images | `single`, `pairs`, `pairs_only` |
| Photo orientation | `any`, `portrait`, `landscape` |
| Photo time range | `all_time`, `1_month`, `3_months`, `6_months`, `1_year`, `2_years`, `3_years`, `4_years`, `5_years`, `10_years` |

<span id="photo-sources"></span>

Source values are `all`, `album`, `memories`, `smart` (Keywords) and `filter` (API only). For saved source fields and entity types, see the [settings](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/settings-reference.md) and [entity reference](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/entity-contract.md).

## Browser & API Add-on

This optional add-on provides browser previews and an HTTP API for custom clients. Its frames and connections are separate from the integration, and **it does not create Home Assistant entities**. It supports amd64 and aarch64 systems.

### Add-on setup

1. Open **Settings → Apps → App store → Repositories** in Home Assistant and add `https://github.com/jtenniswood/espcontrol-immich`.
2. Install and start **EspControl Immich Companion**, then open its web UI.
3. Enter a connection name, server address and API key, then choose **Connect**.
4. Name a frame, choose its connection, source and display settings, then select **Create frame**.

Use the preview's Previous, Pause/Resume and Next controls. Editing, deleting and transferring frames use the API.

<span id="photo-and-display-options"></span>
<span id="advanced-selection-through-the-api"></span>

The add-on includes the same display controls plus a 0–7-day memory window, optional memory fallback and photo ordering. The API adds filters for favourites, people, tags, locations, ratings and other metadata, plus reference-photo searches.

<span id="http-api"></span>

See the [browser and API guide](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/container.md) for configuration, endpoints and examples. Home Assistant manages ingress access; standalone containers serve port **8099 without their own login**, so keep them on a trusted network. Exports omit connection credentials; back up the add-on's data for a full restore.

---

Adapted from the EspControl Immich Companion documentation: [Overview](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/README.md), [Installation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/installation.md), [Using your frame](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/native-integration.md), [Settings reference](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/settings-reference.md), [Home Assistant entities](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/entity-contract.md), [Browser and API add-on](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/container.md), used under its [MIT license](/immich-license.txt).
