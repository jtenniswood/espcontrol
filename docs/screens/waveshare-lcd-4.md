---
title: 4-inch Waveshare ESP32-S3-Touch-LCD-4
description:
  EspControl on the Waveshare ESP32-S3-Touch-LCD-4 — a 4-inch 480x480 square touchscreen with 9 cards, powered by ESP32-S3.
---

# 4-inch Waveshare ESP32-S3-Touch-LCD-4

The **Waveshare ESP32-S3-Touch-LCD-4** is a 4-inch square touchscreen powered by an **ESP32-S3** processor. It matches the same 480×480 EspControl layout as the Guition 4848S040, with **9 cards** on the home screen.

::: warning Board revision
This profile targets **Rev 4.0** boards (marked **V4.0** on the silkscreen), which use a CH32V003 I/O expander. Older Rev 3.x boards use a different expander and are not supported by this profile yet.
:::

## Specifications

| | |
|---|---|
| **Screen size** | 4 inches |
| **Resolution** | 480 × 480 |
| **Orientation** | Square |
| **Display interface** | RGB (ST7701S) |
| **Processor** | ESP32-S3-N16R8 |
| **WiFi** | Built-in (2.4 GHz) |
| **PSRAM** | Octal, 80 MHz |
| **Touch** | GT911 capacitive |
| **Power** | USB-C (press **PWR** once if the board does not boot) |

Unlike the Guition 4848S040 relay variant, this Waveshare board does not include built-in relays.

## Card Grid

<!--@include: ../generated/screens/waveshare-lcd-4-grid.md-->

## Install

Connect the display to your computer with a USB-C data cable, then click the button below.

<!--@include: ../generated/screens/waveshare-lcd-4-install.md-->

For a full walkthrough including WiFi setup and Home Assistant pairing, see the [Install guide](/getting-started/install).

::: tip After flashing or OTA update
This panel uses RGB with octal PSRAM and may need a brief hardware reset after flashing or OTA updates. The firmware handles this automatically with a short deep-sleep cycle — the display may flicker once before coming back up normally.
:::

## ESPHome Manual Setup

If you use ESPHome and prefer to compile firmware yourself:

```yaml
substitutions:
  name: "kitchen-panel"
  friendly_name: "Kitchen Panel"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/codenamefez/espcontrol/
    file: devices/waveshare-esp32-s3-touch-lcd-4/packages.yaml
    refresh: 1sec
```

## Where to Buy

- **Waveshare:** [ESP32-S3-Touch-LCD-4](https://www.waveshare.com/esp32-s3-touch-lcd-4.htm)
- **Wiki / pinout:** [Waveshare wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4)

## Related Projects

Community hardware references that helped validate this board:

- [PedanticAvenger/Waveshare_ESP32-S3-TOUCH-LCD-4](https://github.com/PedanticAvenger/Waveshare_ESP32-S3-TOUCH-LCD-4)
- [betamoojw/Waveshare_ESP32-S3-Touch-LCD-4Inch](https://github.com/betamoojw/Waveshare_ESP32-S3-Touch-LCD-4Inch) (PlatformIO reference)
