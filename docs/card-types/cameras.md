---
title: Camera Cards
description:
  How to show Home Assistant camera and image entities on your EspControl panel.
---

# Camera

A Camera card shows a still image from a Home Assistant `camera` or `image` entity. It is useful for doorbells, driveway cameras, room snapshots, weather cameras, or any Home Assistant image entity you want visible on the panel.

Camera cards are display cards. They do not stream live video, pan the camera, or send camera control actions. Tapping the card opens a larger view of the latest loaded image.

::: info P4 screens only
Camera cards are not supported on the ESP32-S3 screen because it has an older, slower processor and less available memory than the ESP32-P4 screens.
:::

## Setting Up a Camera Card

1. Select a card and change its type to **Camera Card**.
2. Enter a **Camera Entity**, for example `camera.front_door`.
3. Optionally turn on **Show Label** and enter a label. If the label is blank, EspControl uses the entity name from Home Assistant.
4. Optionally turn on **Show Icon** and choose an icon. The default icon is **Camera**.
5. Choose **Expanded Image**:
   - **Crop to fit** fills the expanded view and may crop the edges.
   - **Show full image** keeps the whole image visible and may leave empty space around it.

The card accepts both `camera.*` and `image.*` entities, so `image.latest_package_snapshot` works as well as `camera.front_door`.

If your Home Assistant instance uses a custom port, open **Settings > System > Home Assistant Settings** and set **Home Assistant Port** to match it. Camera and image cards use this port when downloading snapshots.

## How It Works on the Panel

- The card asks Home Assistant for the entity picture and downloads it through Home Assistant.
- The small card requests a snapshot sized for its grid tile, which avoids downloading and processing more pixels than the tile can show.
- Tapping the card opens the larger view immediately. A recent tile is shown while the larger image loads, when one is available.
- Recently loaded images are kept for reuse when you move between pages, so returning to a camera page does not normally start from a blank tile.
- The card refreshes when Home Assistant reports a new entity picture or an entity state update.
- If the image cannot be loaded, the card shows **Loading**, **Unavailable**, **Configure**, or **Too many** instead of leaving a blank tile.
- Camera cards can be used on the main page or inside subpages.

## Practical Limits

Camera images use more memory than normal control cards, so EspControl limits how many can be active at once.

ESP32-P4 screens provide **6 shared image slots**. Each Camera card or Media card set to **Cover Art** uses one slot, across the main page and all subpages combined. For example, 4 Camera cards and 2 Media Cover Art cards use all 6 slots.

If you see a **Too many** message or a warning while saving, reduce the number of Camera cards across the main page and subpages.

For best results, use a few important camera snapshots rather than filling a whole page with cameras. Wider or larger card sizes usually make camera images easier to recognise.

Camera performance also depends on the connection between the panel and Home Assistant. The 7-inch JC1060P470 and ESP32-P4 86 Panel support an advanced Ethernet-only firmware option for manual ESPHome installs. A wired connection can give camera snapshots more consistent load times where 2.4 GHz WiFi is busy or weak. See the relevant screen page before switching, because moving between WiFi and Ethernet firmware should be done over USB.

## Troubleshooting

| Problem | What to check |
|---|---|
| The card says **Configure** | Add a `camera.*` or `image.*` entity to the card. |
| The card says **Unavailable** | Check that the entity exists in Home Assistant and has an image available. |
| The card says **Too many** | Remove or move some Camera cards so the panel has enough image download slots. |
| The picture is cropped | Change **Expanded Image** to **Show full image**. |
| The picture does not update often | Check whether the Home Assistant camera entity itself is updating its snapshot image. |
| Images remain slow on several cards | Check the panel's WiFi signal and Home Assistant response time. On supported Ethernet models, consider the advanced wired firmware option. |
