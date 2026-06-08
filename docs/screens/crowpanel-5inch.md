---
title: 5-inch Elecrow CrowPanel
description:
  EspControl on the Elecrow CrowPanel Advance 5.0 — a 5-inch 800x480 landscape touchscreen with 15 cards, powered by ESP32-S3.
---

# 5-inch CrowPanel Advance 5.0

The **Elecrow CrowPanel Advance 5.0** is a 5-inch landscape touchscreen powered by an **ESP32-S3** processor. It has an 800×480 display and room for **15 cards** on the home screen in a 5×3 grid.

## Specifications

| | |
|---|---|
| **Screen size** | 5 inches |
| **Resolution** | 800 × 480 |
| **Orientation** | Landscape |
| **Display interface** | RGB parallel (ST7262) |
| **Processor** | ESP32-S3-WROOM-1 |
| **WiFi** | Built-in (2.4 GHz) |
| **PSRAM** | 8 MB OPI |
| **Touch** | GT911 capacitive |
| **Backlight** | LEDC PWM (GPIO2) |
| **Power** | USB-C |

## Card Grid

<!--@include: ../generated/screens/crowpanel-5inch-grid.md-->

## Install

Connect the display to your computer with a USB-C data cable, then click the button below.

<!--@include: ../generated/screens/crowpanel-5inch-install.md-->

For a full walkthrough including WiFi setup and Home Assistant pairing, see the [Install guide](/getting-started/install).

::: warning USB flashing only
This panel uses a 4MB flash chip with a single-app partition. Firmware updates require a USB connection — over-the-air updates are not supported.
:::

::: tip After flashing
This panel uses an ESP32-S3 with an RGB display, which requires a brief hardware reset after flashing. The firmware handles this automatically — the display may flicker once before coming back up normally.
:::

## ESPHome Manual Setup

If you use ESPHome and prefer to compile firmware yourself:

```yaml
substitutions:
  name: "living-room-screen"
  friendly_name: "Living Room Screen"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/jtenniswood/espcontrol/
    file: devices/crowpanel-5inch/packages.yaml
    refresh: 1sec
```

## Where to Buy

- **Elecrow:** [CrowPanel Advance 5.0](https://www.elecrow.com/crowpanel-advance-5-0-hmi-esp32-s3-ai-powered-ips-touch-screen-800x480.html)
- **GitHub (hardware docs):** [Elecrow-RD/CrowPanel-Advance-5-HMI-ESP32-S3](https://github.com/Elecrow-RD/CrowPanel-Advance-5-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480)
