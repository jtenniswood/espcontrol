---
title: Install EspControl Firmware
description:
  How to flash EspControl firmware to a supported ESP32 touchscreen, connect it to WiFi, and add it to Home Assistant.
---

# Install

Flash the EspControl firmware to your supported ESP32 display directly from your browser — no special software or technical knowledge required.

::: tip Prefer ESPHome?
If you want to compile and install the firmware yourself, use the [manual ESPHome setup guide](/getting-started/manual-esphome-setup).
:::

## What You Need

- **A supported ESP32 panel:**
  - **[JC8012P4A1](/screens/jc8012p4a1)** — 10.1-inch, 1280x800, landscape
  - **[JC1060P470](/screens/jc1060p470)** — 7-inch, 1024x600, landscape
  - **[JC4880P443](/screens/jc4880p443)** — 4.3-inch, 480x800, portrait
  - **[ESP32-P4 86 Panel](/screens/p4-86)** — 4-inch, 720x720, square
  - **[4848S040](/screens/4848s040)** — 4-inch, 480x480, square
- **USB-C cable** — must be a data cable, not a charge-only cable
- **A computer** running Chrome or Edge (desktop). Safari and Firefox are not supported for flashing.
- **Home Assistant** running on your network

## Flash the Firmware

Connect the display to your computer with the USB-C cable, choose your panel, then click the install button.

<EspInstallSelector />

::: tip Which cable?
If the install button doesn't detect your device, try a different USB-C cable. Charge-only cables (often thinner and cheaper) won't work — you need one that supports data transfer.
:::

### Step by Step

1. **Plug in the display** using the USB-C cable. If your computer asks to install drivers, allow it.
2. **Choose your panel** above, then click **Install EspControl**. A dialog will ask you to choose a serial port — select the one that appeared when you plugged in the display.
3. **Wait for the flash to complete.** This takes a few minutes. You'll see a progress bar. Don't disconnect the cable until it finishes.
4. **The display restarts** and shows a loading screen.

## Connect to WiFi

After flashing, the display needs to connect to your WiFi network.

1. **The display creates a hotspot** called **espcontrol-xxxxxx** (the exact name appears on screen). Connect to it from your phone or laptop.
2. **A setup page opens automatically** (captive portal). If it doesn't, open a browser and go to `192.168.4.1`.
3. **Choose your WiFi network** from the list and enter your password.
4. **The display reconnects** and shows a loading screen while it joins your network. Once connected, the screen will show your device's address (something like `192.168.1.xxx`).

::: tip If the hotspot doesn't appear
Power-cycle the display by unplugging and re-plugging the USB-C cable. The hotspot only appears when the display can't connect to a saved WiFi network.
:::

## Add to Home Assistant

Once the display is on your WiFi network, Home Assistant should discover it automatically.

1. **Open Home Assistant** in your browser.
2. **Look for a notification** in the bottom left — it should say a new device was discovered. If you don't see one, go to **Settings > Devices & Services** and look for a new **ESPHome** entry.
3. **Click "Configure"** and follow the prompts to add the device.

This connection is how the display gets the current time, temperature data, and the ability to control your devices. After adding the device, you need to [allow it to perform Home Assistant actions](/getting-started/home-assistant-actions) so the touchscreen can control your devices.

## Configure Your Panel

With the display connected to WiFi and paired with Home Assistant, you're ready to set it up.

1. **Find the device's address.** It's shown on the display screen. You can also find it in your router's device list or in **Home Assistant > Settings > Devices & Services > ESPHome** (click the device, then look for the IP address).
2. **Open that address in a browser** — for example, `http://192.168.1.42`. This opens the device's built-in web page.
3. **Add your cards.** On the **Screen** tab, tap an empty slot and choose the card type you want. For example, a **Switch** card controls a Home Assistant entity, while a **Sensor** card displays a reading.
4. **Adjust your settings.** On the **Settings** tab, set your card colours, temperatures, screensaver timeout, brightness, and more.
5. **Tap "Apply Configuration"** when you're done. The display restarts with your new settings.

That's it — your panel is ready to use. See the [Setup](/features/setup) guide for a full walkthrough of every setting.

Next: [Troubleshooting](/getting-started/troubleshooting)
