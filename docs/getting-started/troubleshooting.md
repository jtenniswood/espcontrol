---
title: "Troubleshoot EspControl Installation and Home Assistant Controls"
description:
  Solutions for common issues when installing EspControl, connecting to WiFi, or adding the device to Home Assistant.
---

# Troubleshooting

Start with the symptom below. Have your screen model, hardware revision, and firmware version ready; use the matching [screen guide](/screens/) to confirm the installed firmware.

## Web Configuration Changes Won't Save

The web editor can load a compatible hosted update independently of the display's firmware. Reload the page to pick up a published fix; reflashing is usually unnecessary for a hosted editor problem.

If saving or importing a backup fails, try opening `http://YOUR-DISPLAY-IP/?espcontrol_fallback=1` to use the editor embedded in the firmware. Replace `YOUR-DISPLAY-IP` with the display's address. On builds with an embedded editor, this bypasses the hosted editor for that page load. Save one setting and reload to check that it persisted. Reopen the normal device address to return to the hosted editor.

## The Screen Doesn't Respond to Commands

- If the display shows your Home Assistant devices but nothing happens when you tap controls, such as turning lights on, Home Assistant actions probably need to be enabled for the display.
- Follow the [Enable Actions](https://jtenniswood.github.io/espcontrol/getting-started/home-assistant-actions) guide and make sure **Allow the device to perform Home Assistant actions** is turned on.

## The Install Button Doesn't Detect My Device

- Make sure you're using **Chrome or Edge** on a desktop computer. Mobile browsers and Safari/Firefox don't support the required browser feature (WebSerial).
- Try a **different USB-C cable**. Charge-only cables won't work.
- Try a **different USB port** on your computer.
- On Windows, you may need to install drivers — check Device Manager for an unrecognised device.

## The Display Is Stuck on the Loading Screen

- Give it up to 60 seconds after first boot. It needs time to connect to WiFi and download resources.
- If it stays on the loading screen, power-cycle it and check whether the WiFi hotspot appears. If it does, the display couldn't connect to your network — go through the WiFi setup again.
- If you need to report the problem, collect a startup log with the [USB log guide](/reference/collect-usb-logs) and include it in the GitHub issue.

## Home Assistant Doesn't Discover the Device

- Make sure the display and Home Assistant are on the **same WiFi network** (not a guest network or a different VLAN).
- In Home Assistant, go to **Settings > Devices & Services > Add Integration** and search for **ESPHome**. Enter the device's IP address manually.

## Home Assistant Says "Connection Requires Encryption"

- Update both Home Assistant and ESPHome Device Builder to 2026.8 or newer. These versions pass a display's unique API encryption key between Home Assistant and Device Builder when the display is adopted or rebuilt.
- If you already rebuilt the display and Home Assistant asks for a key, open its YAML in ESPHome Device Builder and use the existing `api.encryption.key` value in Home Assistant's reauthentication prompt. Do not generate a different key.
- Normal OTA updates retain the key stored on the display. Erasing the whole display during a USB install can remove it and may require pairing the display with Home Assistant again.
- See [Manual Setup](/getting-started/manual-esphome-setup) for more about automatic API encryption.

## A P4 Panel Has Unreliable WiFi

- P4 panels use a separate ESP32-C6 WiFi processor. Mismatched or outdated C6
  firmware can cause repeated disconnects, failed initial setup, or a panel that
  disappears from Home Assistant after restarting.
- Use the [C6 WiFi recovery installer](/getting-started/c6-recovery) to reinstall
  EspControl and repair the C6 over USB without depending on WiFi.
- This recovery is for P4 panels only, not the ESP32-S3 4848S040.

## The Web Page Looks Broken or Unstyled

- The setup page loads hosted web resources through your **browser**. Check that the browser can reach them, even if the panel itself is on a restricted IoT network.
- Force-refresh the page or try a private window. If loading still fails, try the [embedded-editor fallback](#web-configuration-changes-won-t-save).

## WiFi Does Not Connect

- Use 2.4 GHz WiFi, check the password, and move closer to the access point during setup.
- If saved WiFi cannot reconnect, wait up to **90 seconds** for the `ESP_xxxxxx` setup hotspot, then repeat [WiFi setup](/getting-started/install#connect-to-wifi).
- Ethernet-only custom builds have no WiFi hotspot. Check the wired connection and DHCP lease instead.
- For repeated P4 disconnections, use the [P4 WiFi checks](#a-p4-panel-has-unreliable-wifi).

## Stripes, Haze, or a Halo on the Screen

Confirm the firmware matches the exact panel revision and use a known-good power supply and cable that meet its specifications. If the fault remains, include photos, the power setup, model, and firmware version in a report. A hardware fault may need the seller's help.

## I Want to Start Over

- Save a backup, then use **Settings > System > Factory Reset**. Partial reset keeps WiFi and the Home Assistant encryption key; Complete reset clears those saved credentials too.
- Both keep installed firmware and compiled defaults. A normal reflash is not a guaranteed reset. Follow [Reset the display](/features/backup#reset-the-display) for the full procedure and older-firmware limitations.

## I Need Help With a Bug

- Open a [GitHub issue](https://github.com/jtenniswood/espcontrol/issues/new) and describe the display model, firmware version, and what happened.
- For startup, WiFi, loading screen, or Home Assistant connection problems, include a USB log from the [Collect USB Logs](/reference/collect-usb-logs) guide.

Next: [Setup](/features/setup)
