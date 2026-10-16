---
title: "Backup and Restore"
description: Export and restore EspControl settings, transfer a setup to another panel, or reset the device.
---

# Backup and Restore

Open **System > Backup** on your [device setup page](/features/setup).

## Save a Backup

Choose **Export** and keep the downloaded file somewhere safe. It includes cards, subpages, colours, brightness, schedules, display settings, artwork connections, and update preferences. Export before resetting or changing firmware versions.

Backups do not include Wi-Fi login details, the Home Assistant encryption key, or the touchscreen PIN. They can contain passwords from Wifi Sharing cards and private card settings, so keep the file private.

## Restore a Backup

1. Choose **Import** and select your saved file.
2. Leave **Also restore panel name** off to keep the destination panel's name and address. Enable it only if you want to copy the saved name too.
3. Let the restore finish, then check the cards, sensors, screen behaviour, and artwork.
4. Set your touchscreen PIN again: importing clears the destination's PIN.

A backup from a different-sized panel rearranges cards that no longer fit. Check the layout afterward. Older backups remain supported. Restoring a name uses the destination panel's own hardware suffix and restarts it after the restore.

## Compatibility Notes

New firmware keeps older card configurations and backups readable. If the editor warns that a saved configuration cannot be downgraded, export a backup and stay on compatible firmware; older versions may have smaller storage limits.

## Reset the Display

Open **System > Factory Reset**, choose **Save backup**, then select and confirm a reset:

| Option | What is removed | What is kept |
|---|---|---|
| **Partial reset** | Cards, layouts, subpages, actions, custom device name, and device preferences. | Saved Wi-Fi connection and Home Assistant encryption key. |
| **Complete reset** | The above, plus saved Wi-Fi credentials and the Home Assistant encryption key. | Installed firmware. |

After Partial reset, set up cards again. After Complete reset, follow first-time setup and reconnect Home Assistant if needed; Ethernet panels remain reachable over their wired connection. Home Assistant integration records and automations are not deleted.

Both resets keep defaults compiled into custom ESPHome YAML, including any compiled names or credentials. Install stock firmware to restore stock defaults. Reset clears operational settings; it is not secure erasure.

Factory Reset requires supported firmware. Reload other open setup pages after reset. If power fails during reset, cleanup resumes on startup. If startup stays paused with **Reset recovery** in serial logs, wait for cleanup to finish before importing a backup.
