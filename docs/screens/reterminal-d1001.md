---
title: 8-inch Seeed reTerminal D1001
description:
  EspControl on the Seeed reTerminal D1001 - an 8-inch 1280x800 landscape touchscreen with 20 cards, powered by ESP32-P4.
---

# 8-inch Seeed reTerminal D1001

The **Seeed reTerminal D1001** is an 8-inch touchscreen powered by an **ESP32-P4** processor and ESP32-C6 WiFi co-processor. EspControl uses it in landscape orientation, with a 1280x800 layout and room for **20 cards** on the home screen.

::: warning Community-supported device
This profile is maintained by community contributors rather than officially supported by EspControl. Device-specific testing and help depend on D1001 owners in the community.
:::

## Specifications

| | |
|---|---|
| **Screen size** | 8 inches |
| **Resolution** | 1280 x 800 landscape layout |
| **Native panel resolution** | 800 x 1280 |
| **Orientation** | Landscape |
| **Display interface** | MIPI DSI (JD9365) |
| **Processor** | ESP32-P4 |
| **WiFi** | ESP32-C6 co-processor (2.4 GHz) |
| **Flash** | 16 MB |
| **PSRAM** | Hex mode, 200 MHz |
| **Touch** | GSL3670 capacitive |
| **Buttons** | One physical button (exposed to Home Assistant) |
| **Power** | USB-C |

## Card Grid

<!--@include: ../generated/screens/reterminal-d1001-grid.md-->

## Install

Connect the display to your computer with a USB-C data cable, then click the button below.

<!--@include: ../generated/screens/reterminal-d1001-install.md-->

For a full walkthrough including WiFi setup and Home Assistant pairing, see the [Install guide](/getting-started/install).

::: tip Touch driver
The reTerminal D1001's GSL3670 touch controller uses a GPLv2 (Silead) driver,
vendored in `components/gsl3670` with its own `LICENSE.md` — the same in-tree
pattern as the 10.1" panel's `gsl3680` driver.
:::

::: tip Touch orientation
The initial touch transform follows Seeed's reTerminal D1001 BSP. If your
physical panel responds mirrored after testing, the firmware only needs a small
touchscreen transform adjustment.
:::

## Physical button

The reTerminal D1001 has a physical button next to the display. EspControl
exposes it to Home Assistant as a binary sensor named **Button**, so you can
trigger your own automations from it. EspControl does not bind any built-in
action to the button.

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
    file: devices/seeed-esp32-p4-reterminal-d1001/packages.yaml
    refresh: 1sec
```

## Where to Buy

- **Seeed Studio:** [reTerminal D1001](https://www.seeedstudio.com/reTerminal-D1001-p-6729.html)
