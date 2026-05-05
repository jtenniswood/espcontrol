---
title: Manual ESPHome Setup for EspControl
description:
  How to add EspControl to ESPHome manually, compile the firmware, and install it by USB or OTA.
---

# Manual ESPHome Setup

The normal [browser install](/getting-started/install) is the easiest route. Use this page if you prefer to manage EspControl from ESPHome, want to compile the firmware yourself, or need to install from the ESPHome Device Builder dashboard.

## What You Need

- A supported ESP32 panel.
- ESPHome Device Builder in Home Assistant, or the ESPHome command line on your computer.
- A USB-C data cable for the first install.
- Your WiFi name and password, unless you are using an advanced wired Ethernet option.

::: tip First install or update?
Use USB for a blank screen or a screen that is not already running EspControl. Once EspControl is installed and connected to WiFi, later ESPHome installs can usually be done wirelessly with OTA.
:::

## Choose the Correct Package File

Each screen uses a different ESPHome package file. Pick the one that matches your panel:

| Panel | Package file |
| --- | --- |
| 10.1-inch JC8012P4A1 | `devices/guition-esp32-p4-jc8012p4a1/packages.yaml` |
| 7-inch JC1060P470 | `devices/guition-esp32-p4-jc1060p470/packages.yaml` |
| 4.3-inch JC4880P443 | `devices/guition-esp32-p4-jc4880p443/packages.yaml` |
| 4-inch Waveshare ESP32-P4 86 Panel | `devices/waveshare-esp32-p4-86-panel/packages.yaml` |
| 4-inch 4848S040 | `devices/guition-esp32-s3-4848s040/packages.yaml` |

## ESPHome Device Builder

1. Open **Home Assistant > ESPHome Device Builder**.
2. Select **New Device**.
3. Enter a name, such as `espcontrol-kitchen`.
4. When ESPHome creates the starter YAML, replace it with the template below.
5. Change `name`, `friendly_name`, WiFi details, and the `file` line for your screen.
6. Click **Save**, then open the device menu and choose **Validate**.

```yaml
substitutions:
  name: "espcontrol-kitchen"
  friendly_name: "EspControl Kitchen"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/benJDtom/espcontrol/
    file: devices/guition-esp32-p4-jc1060p470/packages.yaml
    refresh: 1d
```

If you do not use ESPHome secrets, replace the two `!secret` lines with your WiFi details:

```yaml
wifi:
  ssid: "Your WiFi Name"
  password: "Your WiFi Password"
```

## Advanced: Ethernet Options

Some supported ESP32-P4 panels include wired Ethernet. ESPHome cannot run WiFi and Ethernet in the same firmware, so this option is Ethernet-only and is intended for manual installs.

Use this template for Ethernet-capable models. Do not add a `wifi:` block. Change the `file` line to match your screen:

| Panel | Ethernet package file |
| --- | --- |
| 7-inch JC1060P470 Ethernet model | `devices/guition-esp32-p4-jc1060p470/packages.yaml` |
| Waveshare ESP32-P4 86 Panel ETH-2RO | `devices/waveshare-esp32-p4-86-panel/packages.yaml` |

```yaml
substitutions:
  name: "espcontrol-office"
  friendly_name: "EspControl Office"
  network_transport: ethernet
  disable_updates: "true"

packages:
  setup:
    url: https://github.com/benJDtom/espcontrol/
    file: devices/waveshare-esp32-p4-86-panel/packages.yaml
    refresh: 1d
```

If Ethernet is unplugged or your network does not give the display an IP address, the display will show an Ethernet setup message. It will not create a WiFi hotspot in this mode.

The `disable_updates: "true"` substitution removes EspControl's built-in GitHub firmware update checker and update controls. ESPHome OTA stays enabled, so you can still install firmware manually once the display is online.

The Ethernet firmware is intentionally different from the normal WiFi firmware:

- It uses the panel's built-in wired Ethernet port instead of WiFi.
- It does not include WiFi, the captive portal, or the first-boot WiFi setup hotspot.
- It keeps the ESP32-C6 hosted WiFi/Bluetooth co-processor disabled because it is not needed for wired networking.
- It does not support Bluetooth proxy.
- It uses a higher backlight PWM frequency on this panel to avoid the visible shimmer that can appear when Ethernet is active.

When switching a display between WiFi firmware and Ethernet firmware, install the new firmware over USB. OTA updates can fail during this change because the currently running firmware and the new firmware use different network hardware.

To switch back to WiFi later, remove `network_transport: ethernet`, add your `wifi:` block again, then recompile and install the firmware over USB.

::: warning Keep the device name simple
Use lowercase letters, numbers, and hyphens for `name`. For example, `espcontrol-kitchen` is better than `Kitchen Touchscreen`.
:::

## Install by USB

Use this for the first install.

1. Plug the display into the computer running ESPHome, or into the Home Assistant machine if you are using the add-on.
2. In ESPHome Device Builder, open the device menu and choose **Install**.
3. Choose the USB serial option if it is available.
4. Wait for compiling and flashing to finish before unplugging the display.

If ESPHome cannot access the USB port directly, choose **Manual download** instead. For a blank screen, select the factory firmware option if ESPHome asks which format to use. Then open [ESPHome Web Tools](https://web.esphome.io/) in Chrome or Edge, connect to the display, and flash the downloaded file.

## After the Display Boots

1. Wait for the display to join WiFi.
2. Add it to Home Assistant when the ESPHome integration discovers it.
3. Open the display address in a browser, for example `http://192.168.1.42`.
4. Configure cards, colours, brightness, and other settings from the built-in web page.
5. Follow [Home Assistant Actions](/getting-started/home-assistant-actions) so the display is allowed to control your Home Assistant devices.

## Updating Later

Because the package uses `refresh: 1d`, ESPHome checks GitHub for EspControl updates about once a day when it compiles. To update manually, open ESPHome Device Builder and run **Install** again. If the display is online, use OTA so you do not need to reconnect USB.

Next: [Home Assistant Actions](/getting-started/home-assistant-actions)
