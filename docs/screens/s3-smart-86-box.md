---
title: 4-inch Waveshare ESP32-S3 Smart 86 Box
description:
  EspControl on the Waveshare ESP32-S3 Smart 86 Box - a 4-inch 480x480 square touchscreen with 9 cards, powered by ESP32-S3.
---

# 4-inch Waveshare ESP32-S3 Smart 86 Box

The **Waveshare ESP32-S3 Smart 86 Box** is a compact 86-type smart panel powered by an **ESP32-S3** processor. EspControl uses its 4-inch 480x480 touch display as a 3x3 Home Assistant control panel with **9 cards** on the home screen.

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
| **Touch** | GT911 capacitive |
| **Power** | USB-C |

## Card Grid

<!--@include: ../generated/screens/s3-smart-86-box-grid.md-->

## Install

Connect the display to your computer with a USB-C data cable, then click the button below.

<!--@include: ../generated/screens/s3-smart-86-box-install.md-->

For a full walkthrough including WiFi setup and Home Assistant pairing, see the [Install guide](/getting-started/install).

::: tip After flashing or OTA update
This panel uses MIPI RGB with octal PSRAM, which requires a brief hardware reset after flashing or OTA updates. The firmware handles this automatically with a short deep-sleep cycle - the display may flicker once before coming back up normally.
:::

## ESPHome Manual Setup

If you use ESPHome and prefer to compile firmware yourself:

```yaml
substitutions:
  name: "hallway-screen"
  friendly_name: "Hallway Screen"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/jtenniswood/espcontrol/
    file: devices/waveshare-esp32-s3-smart-86-box/packages.yaml
    refresh: 1sec
```
