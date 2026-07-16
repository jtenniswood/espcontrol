---
title: EspControl Firmware Updates
description:
  How the EspControl panel checks for and installs firmware updates over the air, and how to control update behaviour.
---

# Firmware Updates

Your panel can update its firmware over the air — no USB cable or computer needed after the initial install. When a new version is available, the panel downloads and installs it automatically (if enabled) or waits for you to trigger the update manually.

## Update Settings

These are configured from the **Settings** tab in the [Setup](/features/setup) under the **Firmware** section. They also appear as controls in Home Assistant.

- **Current version** and **Available version** — compare the firmware running on your panel with the latest stable release.
- **Auto updates** — open this panel to turn **Auto Update** on or off and choose the **Update Frequency**: **Hourly**, **Daily**, **Weekly**, or **Monthly**.
- **Previous firmware** — open this panel to choose and install one of the available previous stable versions.
- **Check for Update** — press this button to check for a new version right now, regardless of the automatic schedule.

Manual Ethernet builds can be different. If you used the advanced Ethernet setup with `disable_updates: "true"`, these built-in GitHub update controls are removed from the panel firmware. Use ESPHome OTA or USB from your ESPHome setup to update that display instead.

## What Happens During an Update

1. The panel checks the update server for a newer version.
2. If one is available and **Auto Update** is on, it shows an update message, then downloads and installs the update.
3. During installation the screen may turn off. This is normal; do not power off the panel while the update is running.
4. The panel restarts with the new firmware and briefly confirms that the update completed. Your settings (cards, colours, temperatures, etc.) are preserved.

The update usually takes a minute or two. The display may show an update or loading screen briefly during the restart.

## Installing an Older Version

When older stable firmware is available, open **Previous firmware**, choose a **Version**, and press **Install**. This is intended for rolling back to one of the four most recent previous stable versions if you need to test or recover from a problem.

## Compatibility Notes

Firmware updates preserve the panel's existing cards, subpages, colours, brightness, clock, screensaver, and backup settings. The setup page still reads the same saved card string format after an update, so existing cards do not need to be recreated.

Backup imports remain compatible across firmware updates. If you move a backup between different panel sizes, the import may rearrange cards to fit the new screen while keeping supported cards and subpages intact.

## When New Cards Appear

Some features, especially new card types, need both the web setup page and the panel firmware. If the setup page shows a new card type but the panel does not display it correctly after you apply the configuration, check for a firmware update and install the latest version.

## Checking Updates from Home Assistant

You can also manage updates from Home Assistant. The **Auto Update** toggle, **Update Frequency** selector, and **Check for Update** button all appear as entities that you can control from the Home Assistant dashboard or use in automations.

The standard Home Assistant **Update** entity may also appear, depending on your Home Assistant version.

Displays built with `disable_updates: "true"` do not expose EspControl's built-in GitHub update controls. They can still be updated manually through ESPHome.

## ESP32-C6 WiFi Co-processor Updates

Some ESP32-P4 displays use a separate ESP32-C6 chip for WiFi. EspControl exposes separate Home Assistant entities for that co-processor firmware on the supported P4 WiFi builds:

- **7-inch JC1060P470**
- **10.1-inch JC8012P4A1**
- **10.1-inch JC8012P4A1 new panel**
- **4.3-inch JC4880P443**
- **4-inch ESP32-P4-86**

These entities are separate from the main EspControl display firmware controls. The normal **Firmware: Check for Update** and **Firmware: Install Update** controls update the panel firmware. The ESP32-C6 controls check and install compatible WiFi co-processor firmware from ESPHome's hosted firmware manifest.

On the panel's setup page, supported displays show these controls in the **WiFi firmware** panel under **Firmware**.

In Home Assistant, look for ESP32-C6 diagnostic entities showing the current version, latest version, and whether an update is available. When an update is available, use the ESP32-C6 check/install buttons for the co-processor, and continue using the regular EspControl firmware controls for normal panel updates.

Advanced Ethernet-only builds keep the ESP32-C6 WiFi co-processor off, so they do not expose these ESP32-C6 update controls.
