---
title: 7-inch Waveshare ESP32-S3 Touch LCD 7B
description:
  EspControl on the Waveshare ESP32-S3 Touch LCD 7B - a 7-inch 1024x600 landscape touchscreen with 15 cards, powered by ESP32-S3.
---

# 7-inch Waveshare ESP32-S3 Touch LCD 7B

The **Waveshare ESP32-S3 Touch LCD 7B** is a 7-inch landscape touchscreen powered by an **ESP32-S3** processor. It has a 1024 x 600 display and room for **15 cards** on the home screen.

EspControl targets the Type B Waveshare 7-inch panel with an RGB LCD interface, GT911 capacitive touchscreen, and Waveshare CH32V003 I/O controller for reset and backlight control.

## Specifications

| | |
|---|---|
| **Screen size** | 7 inches |
| **Resolution** | 1024 x 600 |
| **Orientation** | Landscape |
| **Display interface** | RGB DPI |
| **Processor** | ESP32-S3 (240 MHz) |
| **WiFi** | 2.4 GHz |
| **Flash** | 16 MB |
| **PSRAM** | 8 MB octal, 80 MHz |
| **Touch** | GT911 capacitive |
| **Power** | USB-C |

## Card Grid

The home screen uses a **5-column x 4-row** grid, giving you **20 card slots**. Any home-screen card can be turned into a [Subpage](/features/subpages) folder containing up to 14 more cards, so you can organise far more than 20 controls without cluttering the home screen.

Flexible card sizes are supported: Single, Tall, Wide, and Large.

## Install

Connect the display to your computer with a USB-C data cable, then click the button below.

<EspInstallButton slug="waveshare-esp32-s3-touch-lcd-7b" />

For a full walkthrough including WiFi setup and Home Assistant pairing, see the [Install guide](/getting-started/install).

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
    url: https://github.com/jtenniswood/espcontrol/
    file: devices/waveshare-esp32-s3-touch-lcd-7b/packages.yaml
    refresh: 1sec
```

## Where to Buy

- **Waveshare:** [ESP32-S3-Touch-LCD-7B](https://www.waveshare.com/esp32-s3-lcd-7b.htm)
