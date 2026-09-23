---
title: Immich Browser and API Add-on
description: Install the optional Immich Companion add-on for browser previews, advanced photo filters, independent frames and HTTP image access.
---

# Optional browser/API add-on

The add-on turns Immich photos into slides you can preview in a browser or retrieve as JPEGs through an HTTP API. Use it when you want to build your own display client or manage frames outside Home Assistant's entity controls.

For Home Assistant devices, dashboards and automations, use the [HACS integration](/immich/installation). Installing the add-on alone does not create native entities, and its frames and connections are separate from those in the integration.

## Get started

The add-on supports **amd64** and **aarch64** systems and requires **Immich 3.2 or later**.

1. In Home Assistant, open **Settings → Apps → App store → Repositories** and add `https://github.com/jtenniswood/espcontrol-immich`.
2. Install **EspControl Immich Companion**, start it and open its web UI.
3. Under **Immich connection**, enter a name, server address and [read-only API key](/immich/installation#immich-connection-and-permissions), then choose **Connect**.
4. Name your frame, select a connection and photo source, choose display settings and select **Create frame**.
5. Use the frame's preview and **Previous**, **Pause/Resume** and **Next** buttons.

You can add multiple connections and create independent frames for each. The browser page supports creation and playback; editing, deleting and transferring frame settings use the API below.

## Photo and display options

The creation form offers **All photos**, multiple **Albums**, **Memories** and **Keywords**, plus the same screen shapes, photo fit, orientation, time range, portrait pairing and timer settings described in [Using your frame](/immich/using-your-frame#fit-the-screen-and-select-photos).

It also offers:

- **Memory window:** 0–7 days on either side of today; default 2. Zero uses today's memories only.
- **Memory fallback:** optionally use your normal photo filter when the memory search returns no candidates. Orientation and time-range rules still apply.
- **Order:** Random, Newest first or Oldest first for normal photo searches. Keywords uses Immich's search results; Memories uses date-ordered results.

Full-size images, preview fallback and saved-slide recovery work as described in [image quality](/immich/installation#image-quality) and [updates and outages](/immich/using-your-frame#updates-and-outages).

Optional add-on configuration values:

| Option | Purpose |
|---|---|
| `immich_url`, `immich_api_key` | Seed a default connection on first use; leave empty to connect in the browser. Later changes do not replace an already saved connection. |
| `cache_limit_mb` | Total disk cache budget; default 256 MB, minimum effective value 16 MB. Older saved slides are removed when the budget is exceeded. |

## Advanced selection through the API

Send JSON to the frame creation or update endpoints. Alongside the [standard settings](/immich/settings-reference), the API accepts a `filter` for favourites, people, tags, locations, dates, ratings, camera details, filenames and OCR text. Rules combine with the source and time range; `or` supports alternatives. Album, person and tag rules use IDs from the catalog endpoints.

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

## HTTP API

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

Adapted from the [EspControl Immich Companion documentation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/container.md), used under its [MIT license](/immich-license.txt).
