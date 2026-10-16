---
title: "Firmware Updates"
description: Check for EspControl firmware updates, choose automatic update behaviour, and restore a previous stable version.
---

# Firmware Updates

Open **System > Firmware** on your [device setup page](/features/setup). [Export a backup](/features/backup) before changing versions.

## Update Your Panel

1. Compare **Current version** and **Available version**. Choose **Check for Update** to refresh the result.
2. Install the available update, or open **Auto updates** to enable **Auto Update** and choose its **Update Frequency**: Hourly, Daily, Weekly, or Monthly.
3. Keep power connected while the panel downloads, installs, and restarts. The screen may go dark during installation.
4. Reopen setup and confirm **Current version**. Check your cards and screen behaviour.

Updates preserve saved settings. An **Update available** badge on the collapsed Firmware section can refer to either panel firmware or Wi-Fi firmware.

## Stable Releases, Development Builds, and Custom Firmware

Check the [release list](https://github.com/jtenniswood/espcontrol/releases) and the matching assets for your panel. Documentation follows the current source, so a documented option may require firmware newer than your installed release.

Built-in updates install released binaries. For custom ESPHome YAML, turn **Auto Update** off and use [manual ESPHome updates](/getting-started/manual-esphome-setup). Ethernet builds with `disable_updates: "true"` also use ESPHome OTA or USB.

**JC8012P4A1 V3** rejects browser OTA uploads; follow the [V3 native ESPHome OTA instructions](/screens/jc8012p4a1-v3).

## Installing an Older Version

Open **Previous firmware**, choose a **Version**, and press **Install**. The panel offers up to four previous stable versions when available. Keep the setup page open during download and upload, then confirm the selected version after restart. If upload or confirmation fails, check Current version before retrying.

## ESP32-C6 WiFi Co-processor Updates

Supported P4 Wi-Fi panels have a separate **WiFi firmware** panel under Firmware. It updates the Wi-Fi chip independently of the main display firmware.

Automatic Wi-Fi firmware updates are on by default and checked daily. Open **WiFi firmware > Auto Update** to change this, or use **Check for Update** and **Update WiFi Firmware** manually. In Home Assistant, use the separate **WiFi Firmware: Auto Update** and diagnostic entities.

Ethernet-only builds with the Wi-Fi co-processor disabled do not expose these controls. If Wi-Fi is too unreliable for an online update, use the [USB C6 recovery installer](/getting-started/c6-recovery).

## If a New Option Is Missing

Refresh the setup page and check firmware availability for your model. New card types and settings can require both an updated editor and updated panel firmware.
