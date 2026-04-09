---
title: Install
description:
  How to flash Espcontrol firmware to a Guition ESP32 touchscreen, connect it to WiFi, and add it to Home Assistant.
---

# Install

Flash the Espcontrol firmware to your Guition ESP32 display directly from your browser — no special software or technical knowledge required.

## What you need

- **A supported Guition ESP32 panel:**
  - **JC1060P470** — 7-inch, 1024x600, landscape
  - **JC4880P443** — 4.3-inch, 480x800, portrait
  - **4848S040** — 4-inch, 480x480, square
- **USB-C cable** — must be a data cable, not a charge-only cable
- **A computer** running Chrome or Edge (desktop). Safari and Firefox are not supported for flashing.
- **Home Assistant** running on your network

## Flash the firmware

Connect the display to your computer with the USB-C cable, then click the button for your panel.

**7-inch (JC1060P470):**

<EspInstallButton slug="guition-esp32-p4-jc1060p470" />

**4.3-inch (JC4880P443):**

<EspInstallButton slug="guition-esp32-p4-jc4880p443" />

**4-inch square (4848S040):**

<EspInstallButton slug="guition-esp32-s3-4848s040" />

::: tip Which cable?
If the install button doesn't detect your device, try a different USB-C cable. Charge-only cables (often thinner and cheaper) won't work — you need one that supports data transfer.
:::

### Step by step

1. **Plug in the display** using the USB-C cable. If your computer asks to install drivers, allow it.
2. **Click "Install Espcontrol"** above. A dialog will ask you to choose a serial port — select the one that appeared when you plugged in the display.
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
4. **Allow the device to perform Home Assistant actions** go to **ESPHome** integration in Home Assistant and click the **Configure** option of your device to Allow the device to perform Home Assistant actions.

This connection is how the display gets the current time, temperature data, and the ability to control your devices.

## Configure your panel

With the display connected to WiFi and paired with Home Assistant, you're ready to set it up.

1. **Find the device's address.** It's shown on the display screen. You can also find it in your router's device list or in **Home Assistant > Settings > Devices & Services > ESPHome** (click the device, then look for the IP address).
2. **Open that address in a browser** — for example, `http://192.168.1.42`. This opens the device's built-in web page.
3. **Add your buttons.** On the **Screen** tab, tap an empty slot and choose which Home Assistant device it should control. Pick an icon and optionally set a custom label.
4. **Adjust your settings.** On the **Settings** tab, set your button colours, temperatures, screensaver timeout, brightness, and more.
5. **Tap "Apply Configuration"** when you're done. The display restarts with your new settings.

That's it — your panel is ready to use. See the [Web UI](/web-ui) guide for a full walkthrough of every setting.

## Troubleshooting

### The install button doesn't detect my device

- Make sure you're using **Chrome or Edge** on a desktop computer. Mobile browsers and Safari/Firefox don't support the required browser feature (WebSerial).
- Try a **different USB-C cable**. Charge-only cables won't work.
- Try a **different USB port** on your computer.
- On Windows, you may need to install drivers — check Device Manager for an unrecognised device.

### The display is stuck on the loading screen

- Give it up to 60 seconds after first boot. It needs time to connect to WiFi and download resources.
- If it stays on the loading screen, power-cycle it and check whether the WiFi hotspot appears. If it does, the display couldn't connect to your network — go through the WiFi setup again.

### Home Assistant doesn't discover the device

- Make sure the display and Home Assistant are on the **same WiFi network** (not a guest network or a different VLAN).
- In Home Assistant, go to **Settings > Devices & Services > Add Integration** and search for **ESPHome**. Enter the device's IP address manually.

### The web page looks broken or unstyled

- The device's web page loads some resources from the internet. Make sure the display has a working internet connection (not just local network access).
- Try clearing your browser cache and reloading.

### I want to start over

- To re-flash the firmware, connect via USB-C and use the install button at the top of this page again.
- To clear WiFi settings and start fresh, re-flash the device. It will create the setup hotspot again.

## Advanced: ESPHome package install

If you already use ESPHome and prefer to compile firmware yourself, see the [ESPHome Manual Setup](/esphome-manual-setup) guide. This method is for users who want full control over the build or want to extend the configuration with their own customisations.

Next: [Web UI](/web-ui)
