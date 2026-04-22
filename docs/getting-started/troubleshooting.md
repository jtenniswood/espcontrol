---
title: Troubleshooting
description:
  Solutions for common issues when installing Espcontrol, connecting to WiFi, or adding the device to Home Assistant.
---

# Troubleshooting

## The Install Button Doesn't Detect My Device

- Make sure you're using **Chrome or Edge** on a desktop computer. Mobile browsers and Safari/Firefox don't support the required browser feature (WebSerial).
- Try a **different USB-C cable**. Charge-only cables won't work.
- Try a **different USB port** on your computer.
- On Windows, you may need to install drivers — check Device Manager for an unrecognised device.

## The Display Is Stuck on the Loading Screen

- Give it up to 60 seconds after first boot. It needs time to connect to WiFi and download resources.
- If it stays on the loading screen, power-cycle it and check whether the WiFi hotspot appears. If it does, the display couldn't connect to your network — go through the WiFi setup again.

## Home Assistant Doesn't Discover the Device

- Make sure the display and Home Assistant are on the **same WiFi network** (not a guest network or a different VLAN).
- In Home Assistant, go to **Settings > Devices & Services > Add Integration** and search for **ESPHome**. Enter the device's IP address manually.

## The Web Page Looks Broken or Unstyled

- The device's web page loads some resources from the internet. Make sure the display has a working internet connection (not just local network access).
- Try clearing your browser cache and reloading.

## I Want to Start Over

- To re-flash the firmware, connect via USB-C and use the [install button](/getting-started/install#flash-the-firmware) again.
- To clear WiFi settings and start fresh, re-flash the device. It will create the setup hotspot again.

Next: [Setup](/features/setup)
