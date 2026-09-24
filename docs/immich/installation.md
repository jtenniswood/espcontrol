---
title: Install the Immich Companion
description: Install EspControl Immich Companion through HACS or manually, configure Immich API permissions, and troubleshoot photo connections and image quality.
---

# Installation

Use the Home Assistant integration for native frame devices, dashboards and automations. It runs within Home Assistant and does not need MQTT or a separate container. You need an **Immich 3.2 or later** server reachable from Home Assistant, plus a read-only API key.

For the separate browser/API experience, follow the [optional add-on guide](/immich/add-on).

## Installation status

As of **September 22, 2026**, the companion has no published GitHub releases. The guides in this section describe its documented development features. Check the [companion releases](https://github.com/jtenniswood/espcontrol-immich/releases) for availability; if HACS does not offer a download, use the manual development installation below. The companion's release status is separate from stable EspControl firmware.

## HACS installation

1. Open **HACS → ⋮ → Custom repositories**.
2. Add `https://github.com/jtenniswood/espcontrol-immich` with category **Integration**.
3. Find **EspControl Immich Companion**, download it and restart Home Assistant.
4. Open **Settings → Devices & services → Add integration → EspControl Immich Companion**.
5. Enter your Immich server address and API key, choose a photo source and name your frame.
6. Open the frame's device page and select **Configuration → Screen shape** to match your display.

Next, [connect the frame to EspControl](/immich/display-setup).

See [Using your frame](/immich/using-your-frame) for all controls. Add the integration again for more frames. For updates, download the new version in HACS and restart Home Assistant; frame settings are retained.

## Manual installation

Download your chosen revision from the [companion repository](https://github.com/jtenniswood/espcontrol-immich) using **Code → Download ZIP**, or use a published release when available. Copy the entire `custom_components/immich_frames` directory into `<config>/custom_components/immich_frames`, where `<config>` contains Home Assistant's `configuration.yaml`. Restart Home Assistant and add the integration as above.

The same process works for a development branch. Back up the existing integration and Home Assistant configuration before replacing it, and keep only one installed copy. See [migration and rollback](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/architecture.md#settings-upgrades-and-rollback) when testing an upgrade.

## Immich connection and permissions

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

## Image quality

Every layout requests full-size photos before resizing them to your screen, including both halves of a pair and unmatched portraits. Rendered images use high-quality JPEG output (quality 95 with full colour detail).

Full-size downloads use more bandwidth and can take longer on slower connections. Allow `asset.download` for originals. Formats such as HEIC or RAW may need a compatible full-size conversion from Immich. If a full-size request fails or its image cannot be decoded, the companion tries the preview; Immich may also supply a preview when a full-size conversion is unavailable.

Improved quality appears as new slides are rendered; you do not need to clear the cache. If fallback photos look soft, check Immich's preview quality and image-generation settings.

## Troubleshooting

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

---

Adapted from the [EspControl Immich Companion documentation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/installation.md), used under its [MIT license](/immich-license.txt).
