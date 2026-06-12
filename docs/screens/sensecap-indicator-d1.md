---
title: Seeed SenseCAP Indicator D1
description:
  EspControl on the Seeed SenseCAP Indicator D1 - a 4-inch 480x480 square touchscreen powered by ESP32-S3.
---

# Seeed SenseCAP Indicator D1

::: warning Community-maintained device
This SenseCAP Indicator D1 firmware is maintained by the `davidmerrique/espcontrol` fork on the `community/seeed-sensecap-indicator-d1` branch. It is not officially supported by upstream EspControl; firmware, testing, and user support are community-provided.
:::

The **Seeed SenseCAP Indicator D1** is a 4-inch square touchscreen powered by an **ESP32-S3** processor. EspControl uses its 480 x 480 display for **9 cards** on the home screen.

## Specifications

| | |
|---|---|
| **Screen size** | 4 inches |
| **Resolution** | 480 x 480 |
| **Orientation** | Square |
| **Display interface** | MIPI RGB |
| **Processor** | ESP32-S3 |
| **WiFi** | Built-in (2.4 GHz) |
| **PSRAM** | Octal, 80 MHz |
| **Touch** | FT5x06 capacitive |
| **Backlight** | GPIO45 PWM |
| **User button** | GPIO38 |
| **Power** | USB-C |

## Card Grid

<!--@include: ../generated/screens/sensecap-indicator-d1-grid.md-->

## Install

Connect the display to your computer with a USB-C data cable, then click the button below.

<!--@include: ../generated/screens/sensecap-indicator-d1-install.md-->

For a full walkthrough including WiFi setup and Home Assistant pairing, see the [Install guide](/getting-started/install).

::: tip After flashing or OTA update
This panel uses MIPI RGB with octal PSRAM, which requires a brief hardware reset after flashing or OTA updates. The firmware handles this automatically with a short deep-sleep cycle - the display may flicker once before coming back up normally.
:::

## ESPHome Manual Setup

If you use ESPHome and prefer to compile firmware yourself:

```yaml
substitutions:
  name: "kitchen-screen"
  friendly_name: "Kitchen Screen"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/davidmerrique/espcontrol/
    ref: community/seeed-sensecap-indicator-d1
    file: devices/seeed-sensecap-indicator-d1/packages.yaml
    refresh: 1sec
```

## Where to Buy

- **Seeed Studio:** [SenseCAP Indicator D1](https://www.seeedstudio.com/SenseCAP-Indicator-D1-p-5643.html)
