---
title: EspControl Troubleshooting
description:
  Solutions for common issues when installing EspControl, connecting to WiFi, or adding the device to Home Assistant.
---

# Troubleshooting

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

## A P4 Panel Has Unreliable WiFi

- P4 panels use a separate ESP32-C6 WiFi processor. Mismatched or outdated C6
  firmware can cause repeated disconnects, failed initial setup, or a panel that
  disappears from Home Assistant after restarting.
- Use the [C6 WiFi recovery installer](/getting-started/c6-recovery) to reinstall
  EspControl and repair the C6 over USB without depending on WiFi.
- This recovery is for P4 panels only, not the ESP32-S3 4848S040.

## The Web Page Looks Broken or Unstyled

- The device's web page loads some resources from the internet. Make sure the display has a working internet connection (not just local network access).
- Try clearing your browser cache and reloading.

## I Want to Start Over

- To re-flash the firmware, connect via USB-C and use the [install button](/getting-started/install#flash-the-firmware) again.
- To clear WiFi settings and start fresh, re-flash the device. It will create the setup hotspot again.

## I Need Help With a Bug

- Open a [GitHub issue](https://github.com/jtenniswood/espcontrol/issues/new) and describe the display model, firmware version, and what happened.
- For startup, WiFi, loading screen, or Home Assistant connection problems, include a USB log from the [Collect USB Logs](/reference/collect-usb-logs) guide.

Next: [Setup](/features/setup)
