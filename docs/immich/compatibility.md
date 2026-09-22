---
title: Immich Compatibility
description: Immich server requirements, supported photo sources and filters, native and add-on feature coverage, and read-only integration limits.
---

# Compatibility and feature coverage

The minimum supported server is Immich 3.2.0. The initial test target is Immich 3.2.x. The integration checks the server version before setup and reports unavailable capabilities instead of silently dropping an active rule.

Implemented sources and search features:

- All photos source with the safe image/timeline constraints applied automatically.
- Albums source with multiple selections by name using Immich's `albumIds` metadata filter.
- Keywords text searches; reference-photo searches through the optional add-on API.
- On This Day memories; adjustable memory windows and optional fallback through the add-on. New native frames use a two-day window with no fallback.
- Custom filters through the add-on API, including favourites, people, tags, locations, dates, ratings, camera details and OCR text.
- Explicit image, non-trashed, non-locked safety constraints.
- Single-image and capture-date matching-pair output.
- Local orientation selection (`any`, portrait, landscape, or square) applied after Immich metadata retrieval.

See [Using your frame](/immich/using-your-frame) for the native controls and [the optional add-on guide](/immich/add-on) for browser and API capabilities. Square-only selection is retained for older configurations and API use; the native selector offers Mixed, portrait and landscape.

The integration does not modify Immich assets. Uploading, editing, tagging, rating changes, library administration, and video playback are outside the read-only frame scope.

The optional renderer app is built and tested for Home Assistant `amd64` and `aarch64` images. These are the architectures supported by the current official Home Assistant base images. CI builds both declared architectures; a published release creates one multi-architecture GHCR manifest.

---

Adapted from the [EspControl Immich Companion documentation](https://github.com/jtenniswood/espcontrol-immich/blob/18ed9ecd30e6ce16fd07bbe6c34e1f72a67acb15/docs/compatibility.md), used under its [MIT license](/immich-license.txt).
