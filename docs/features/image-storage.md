---
title: "Enable Image Storage"
description:
  Back up an older EspControl display, reinstall over USB, and restore its settings to enable uploaded card background images.
---

# Enable Image Storage

Uploaded card backgrounds need a dedicated area in the display's flash memory. Some displays still have an older partition layout without this image-storage area. They can run current firmware, but need one full USB reinstall to enable image support.

The explicit layout includes a **2 MB storage partition**. Some of that space is reserved for saved settings and panel identity; images use a separate region. The layout was introduced on 11 August 2026 and included in v2.8.0, released on 16 August. A display installed with an older layout keeps it when updated over the air: OTA updates replace the firmware, not the partition table. The installed firmware version alone therefore does not tell you whether image storage is available.

## What Works on the Old Layout?

| Feature | Without image storage |
|---|---|
| Normal cards, Home Assistant controls, colours and display settings | Continue working |
| Camera and Home Assistant Image cards, media cover art and the cover-art screensaver | Continue working; they do not use the uploaded-image library |
| Configuration backup and restore without image files | Continue working |
| Uploading, selecting, renaming or deleting stored card background images | Disabled |
| Showing uploaded backgrounds on the display or web preview | Unavailable; the cards remain usable without those pictures |
| Restoring image files from a backup | Blocked before configuration is changed; reinstall over USB first |

The **Card Image** controls and **Settings > System > Backup** show an information banner when the required storage is missing or incompatible. It explains that image support needs a USB reinstall, asks you to export a backup first, and links to this guide. Other features remain available. Export on such a display saves a configuration-only backup and explains that images were not included.

## 1. Save a Backup

1. Open the display's address in your browser.
2. Open **Settings > System > Backup** and choose **Export**.
3. Keep the downloaded file on your computer. Check that it downloaded before reinstalling; do not rely on a browser tab to preserve your settings.

Backups preserve cards, subpages and display settings. When image storage is available, a full backup also includes uploaded images. If storage is missing, configuration-only export is expected. Keep the original image files if you will need to upload them later.

::: warning Keep connection details ready
A USB reinstall can erase saved settings, Wi-Fi credentials and the Home Assistant encryption key. Backups do not restore the Wi-Fi login or encryption key. Have your network details ready and be prepared to pair the display with Home Assistant again. See [Backup and Restore](/features/backup) for what a backup includes.
:::

## 2. Reinstall Over USB

1. Identify your exact [screen model and hardware revision](/screens/).
2. Connect its programming port to your computer with a USB **data** cable.
3. Follow [Install EspControl](/getting-started/install#flash-the-firmware), choosing the matching model and revision. Complete a full USB firmware installation so the partition table is written.
4. Let the display restart, then reconnect Wi-Fi and Home Assistant as needed.

Uploading a firmware file through the display's web page or using ESPHome OTA does not install the new partition layout. Resetting the display does not change it either.

For a custom ESPHome installation, follow [Manual ESPHome Setup](/getting-started/manual-esphome-setup#install-by-usb). Use the current device configuration and perform the full USB install; when flashing a downloaded file, use the matching **factory** image rather than the OTA image. If you are testing a feature branch, use a factory build from that same branch.

## 3. Restore and Check Image Support

1. Reopen the display's web page and go to **Settings > System > Backup > Import**.
2. Choose your saved backup and wait for restoration to finish. Apply the configuration if prompted.
3. Open a supported card's settings and expand **Card Image**. The image-storage banner should be gone and **Upload image** should be enabled.
4. Upload a picture, select it, and check the web preview and physical display. A configuration-only backup has no image files to restore, so upload those pictures separately.

After the full USB install, future firmware updates can use OTA normally. If the banner remains, check that you installed the correct current factory firmware for your exact display and revision; another OTA update will not resolve a missing partition.
