---
title: "System Controls"
description: Manage EspControl device naming, backups, firmware, Home Assistant image connections, battery support, factory reset, and restart in one guide.
---

# System

Manage the device name, saved setup, firmware, and Home Assistant image connection. Open **System > System** on your [device setup page](/features/setup).

| Setting | What to do |
|---|---|
| [Device Name](/settings/maintenance#device-name) | Rename the panel and check its new network address. |
| [Backup](/settings/maintenance#backup) | Export a setup or import a saved configuration. |
| [Firmware](/settings/maintenance#firmware) | Check for updates and choose automatic update behaviour. |
| [Home Assistant Settings](/settings/maintenance#home-assistant-settings) | Check or change the image and artwork connection. |
| [Battery](/settings/maintenance#battery) | Enable readings on compatible panels with a battery add-on. |
| [Factory Reset](/settings/maintenance#reset-the-display) | Clear configuration, optionally keeping network credentials. |
| [Restart](/settings/maintenance#restart) | Reboot from Home Assistant without changing saved settings. |

Export a backup before resetting or changing firmware versions. Device Name, Battery, and Factory Reset need supported hardware or firmware. To restart without changing settings, use the [Home Assistant Restart button](/settings/maintenance#restart).

## Device Name

1. Open **System > Device Name** and enter a name such as **Kitchen**.
2. Check the address preview, then choose **Save & Restart**.
3. Reopen the setup page using the new address or the current IP shown before restart.

The address includes a hardware suffix, for example `kitchen-b2c3.local`. Clearing the name restores the firmware's original name and address. Normal firmware updates keep the custom name.

Home Assistant entity IDs stay unchanged, and a name set manually in Home Assistant takes precedence. ESPHome action names include the hostname: update automations using actions such as `esphome.<old_name>_navigate`. Reload the ESPHome integration if it still lists the old action names. Update router-managed DNS entries separately if needed.

## Backup

Open **System > Backup** on your [device setup page](/features/setup).

### Save a Backup

Choose **Export** and keep the downloaded file somewhere safe. It includes cards, subpages, colours, brightness, schedules, display settings, artwork connections, and update preferences. Export before resetting or changing firmware versions.

Backups do not include Wi-Fi login details, the Home Assistant encryption key, or the touchscreen PIN. They can contain passwords from Wifi Sharing cards and private card settings, so keep the file private.

### Restore a Backup

1. Choose **Import** and select your saved file.
2. Leave **Also restore panel name** off to keep the destination panel's name and address. Enable it only if you want to copy the saved name too.
3. Let the restore finish, then check the cards, sensors, screen behaviour, and artwork.
4. Set your touchscreen PIN again: importing clears the destination's PIN.

A backup from a different-sized panel rearranges cards that no longer fit. Check the layout afterward. Older backups remain supported. Restoring a name uses the destination panel's own hardware suffix and restarts it after the restore.

### Compatibility Notes

New firmware keeps older card configurations and backups readable. If the editor warns that a saved configuration cannot be downgraded, export a backup and stay on compatible firmware; older versions may have smaller storage limits.

## Firmware

Open **System > Firmware** on your [device setup page](/features/setup). [Export a backup](/settings/maintenance#backup) before changing versions.

### Update Your Panel

1. Compare **Current version** and **Available version**. Choose **Check for Update** to refresh the result.
2. Install the available update, or open **Auto updates** to enable **Auto Update** and choose its **Update Frequency**: Hourly, Daily, Weekly, or Monthly.
3. Keep power connected while the panel downloads, installs, and restarts. The screen may go dark during installation.
4. Reopen setup and confirm **Current version**. Check your cards and screen behaviour.

Updates preserve saved settings. An **Update available** badge on the collapsed Firmware section can refer to either panel firmware or Wi-Fi firmware.

### Stable Releases, Development Builds, and Custom Firmware

Check the [release list](https://github.com/jtenniswood/espcontrol/releases) and the matching assets for your panel. Documentation follows the current source, so a documented option may require firmware newer than your installed release.

Built-in updates install released binaries. For custom ESPHome YAML, turn **Auto Update** off and use [manual ESPHome updates](/getting-started/manual-esphome-setup). Ethernet builds with `disable_updates: "true"` also use ESPHome OTA or USB.

**JC8012P4A1 V3** rejects browser OTA uploads; follow the [V3 native ESPHome OTA instructions](/screens/jc8012p4a1-v3).

### Installing an Older Version

Open **Previous firmware**, choose a **Version**, and press **Install**. The panel offers up to four previous stable versions when available. Keep the setup page open during download and upload, then confirm the selected version after restart. If upload or confirmation fails, check Current version before retrying.

### ESP32-C6 WiFi Co-processor Updates

Supported P4 Wi-Fi panels have a separate **WiFi firmware** panel under Firmware. It updates the Wi-Fi chip independently of the main display firmware.

Automatic Wi-Fi firmware updates are on by default and checked daily. Open **WiFi firmware > Auto Update** to change this, or use **Check for Update** and **Update WiFi Firmware** manually. In Home Assistant, use the separate **WiFi Firmware: Auto Update** and diagnostic entities.

Ethernet-only builds with the Wi-Fi co-processor disabled do not expose these controls. If Wi-Fi is too unreliable for an online update, follow the [P4 WiFi troubleshooting checks](/getting-started/troubleshooting#a-p4-panel-has-unreliable-wifi).

### If a New Option Is Missing

Refresh the setup page and check firmware availability for your model. New card types and settings can require both an updated editor and updated panel firmware.

## Home Assistant Settings

Leave **System > Home Assistant Settings > Connection** on **Automatic** for normal local installations. The panel discovers a local Home Assistant image endpoint and shows its address, source, and health.

If controls work but images do not:

1. Check that the camera image or player artwork appears in Home Assistant.
2. Compare the panel's displayed endpoint with **Home Assistant > Settings > System > Network > Home Assistant URL > Local network**.
3. If needed, choose **Manual** and enter **Home Assistant Host**, **Home Assistant Protocol**, and **Home Assistant Port**. Leave Host blank to use the connected Home Assistant API address. Use Manual for a hostname-based proxy or a proxy on another host.
4. Retry the image. **Connection checked** means Home Assistant responded; **Images received** confirms a successful image download.

Automatic discovery accepts an advertised local URL only on the connected Home Assistant server's IP. If it is unreachable, the panel checks advertised/configured protocol and port fallbacks; it does not scan ports. The separate Internet URL is not used. Image access errors do not trigger a server switch.

For HTTPS, use a host matching its certificate. A self-signed certificate must be trusted separately. **Access denied** needs an authentication or IP-ban check in Home Assistant; switching protocols will not fix it. See [Camera troubleshooting](/card-types/cameras#troubleshooting) or [missing artwork](/settings/sleep#track-details-work-but-artwork-is-missing).

## Battery

Some Guition JC8012P4A1 (10.1") units ship with an IP5306-based battery add-on board. EspControl can show a battery icon in the top bar for these units, but the wiring (GPIO52, post-divider voltage range) is based on community and third-party BSP reports and has **not** been confirmed by the firmware maintainer. It may not work, or may read incorrectly, on every unit.

Because of this, the feature is off by default and only appears in **System > Battery** on JC8012P4A1 panels.

- **Enable battery support** - turns battery readings and the battery icon in the top bar on or off. While this setting is off, the "Battery" diagnostic entity in Home Assistant is unavailable. When enabled, its percentage is estimated from the raw ADC voltage using a linear approximation between an empty and full voltage.

If the icon shows the wrong level, or your unit does not have the battery add-on, leave this setting off. The "Battery Voltage Raw" and "Battery Voltage" diagnostic sensors in Home Assistant can help you verify or recalibrate the readings against a multimeter. There is currently no way to detect charging state from firmware alone, so the icon only ever shows a charge level, never a "charging" state.

## Factory Reset {#reset-the-display}

Open **System > Factory Reset**, choose **Save backup**, then select and confirm a reset:

| Option | What is removed | What is kept |
|---|---|---|
| **Partial reset** | Cards, layouts, subpages, actions, custom device name, and device preferences. | Saved Wi-Fi connection and Home Assistant encryption key. |
| **Complete reset** | The above, plus saved Wi-Fi credentials and the Home Assistant encryption key. | Installed firmware. |

After Partial reset, set up cards again. After Complete reset, follow first-time setup and reconnect Home Assistant if needed; Ethernet panels remain reachable over their wired connection. Home Assistant integration records and automations are not deleted.

Both resets keep defaults compiled into custom ESPHome YAML, including any compiled names or credentials. Install stock firmware to restore stock defaults. Reset clears operational settings; it is not secure erasure.

Factory Reset requires supported firmware. Reload other open setup pages after reset. If power fails during reset, cleanup resumes on startup. If startup stays paused with **Reset recovery** in serial logs, wait for cleanup to finish before importing a backup.

## Restart

In Home Assistant, open **Settings > Devices & services > ESPHome**, choose the panel, and press **Restart** under its configuration controls. Saved cards and settings are kept.

For an automation, select the panel's Restart entity with the `button.press` action. Replace this example ID with the actual entity:

```yaml
actions:
  - action: button.press
    target:
      entity_id: button.your_panel_restart
```

For automations triggered by Home Assistant startup, wait until that entity is available before pressing it. Use **Apply Configuration** in the web page after editing settings.
