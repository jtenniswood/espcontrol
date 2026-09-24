---
title: Choose an ESP32 Touchscreen for Home Assistant
description: Compare supported EspControl touchscreens by size, card capacity, cameras, networking, and hardware revision before buying or installing.
---

# Choose a Home Assistant Touchscreen

EspControl turns a supported ESP32 touchscreen into a dedicated Home Assistant controller. Choose the **4-inch S3 for a compact, lower-cost panel** or a **7- or 10.1-inch P4 for more controls at once**. All use the same browser-based card editor.

You need a working Home Assistant installation, a supported screen, a suitable power supply, and a USB data cable for the first install. EspControl has its own interface; it cannot display or import your existing Lovelace dashboard cards.

## Compare Supported Screens

These are the standard home-screen layouts. A larger card occupies several slots. Each [subpage](/features/subpages) reserves one slot for Back.

<p class="screen-comparison-hint">Swipe the table sideways to compare card capacity and image slots.</p>

| Screen and installation guide | Processor | Resolution and default layout | Home slots | Shared image slots |
|---|---|---|---:|---:|
| [4-inch Guition 4848S040](/screens/4848s040) | ESP32-S3 | 480 × 480, square | 9 | 2 |
| [4.3-inch Guition JC4880P443](/screens/jc4880p443) | ESP32-P4 | 480 × 800, portrait | 6 | 6 |
| [4-inch P4 86 Panel, ETH-2RO](/screens/p4-86) | ESP32-P4 | 720 × 720, square | 9 | 6 |
| [7-inch Guition JC1060P470](/screens/jc1060p470) | ESP32-P4 | 1024 × 600, landscape | 15 | 6 |
| [10.1-inch Guition JC8012P4A1](/screens/jc8012p4a1) | ESP32-P4 | 1280 × 800, landscape | 20 | 6 |

Camera and Media Cover Art cards share the image-slot limit **across all pages**. Moving one into a subpage does not free a slot. Cameras display still snapshots, not live video. Read the [camera limits](/card-types/cameras#practical-limits) before planning a camera-heavy layout.

## S3 or P4?

The S3 supports lights, climate, media controls, speaker groups, and camera snapshots. It suits a bedside panel or a small set of everyday controls. Its two image slots allow one Camera card alongside one Media Cover Art card.

Choose a P4 when you need more image cards or a larger screen. The 4.3-inch model fits a narrow portrait layout; the 7- and 10.1-inch models provide more room for rooms, scenes, and sensor readings. Media cards control external Home Assistant players.

## Networking and Power

All standard browser installs use **2.4 GHz WiFi**. The JC1060P470 and P4 86 Panel also have documented **advanced Ethernet-only builds** on their model guides. These require manual ESPHome installation, disable WiFi setup, and use different update settings. Switch between networking variants over USB.

Use the model's USB programming port and a data-capable cable for installation. For everyday power, check the exact board's labelled input, voltage, required current, and connector before buying a supply. USB connector shape does not establish support for another voltage or power input.

The Waveshare **ESP32-P4-86-Panel-ETH-2RO** supports USB-C power or its separate DC 6–30 V input, as documented in the [manufacturer's power guidance](https://docs.waveshare.com/ESP32-P4-WIFI6-Touch-LCD-4B/FAQ). This specification applies to that exact board, not the Guition panels. An Ethernet socket does not by itself establish PoE support. Battery measurement is only documented for [compatible JC8012P4A1 battery hardware](/features/battery).

## Identify the Hardware Revision Before Installing

- **10.1-inch JC8012P4A1:** check chip information first. ESP32-P4 v3.x production silicon needs [V3 firmware](/screens/jc8012p4a1-v3), regardless of the case date. Otherwise, rear-case markings `2627` or lower use [V1](/screens/jc8012p4a1-v1), and `2628` or higher use [V2](/screens/jc8012p4a1-v2).
- **7-inch JC1060P470:** a case marked `V2`, or a screen-board date `2622` or higher, identifies [V2](/screens/jc1060p470-v2). An unmarked case with an earlier board date identifies [V1](/screens/jc1060p470-v1). Follow the [identification guide](/screens/jc1060p470) if uncertain.
- **4-inch P4:** EspControl targets **ESP32-P4-86-Panel-ETH-2RO**. Other rear boards or similarly named camera variants are not automatically supported.

Matching screen size, resolution, or processor is not sufficient. The common Cheap Yellow Display and other unlisted boards do not have ready-to-install EspControl firmware.

## Buy, Mount, and Install

Each model guide above links to its seller and installation instructions. Seller prices and revisions can change; confirm the exact model before ordering. Match [printable stands and mounts](/reference/3d-printable-stands) to the rear board and cable clearance as well as the screen size.

Once you have the correct screen, follow **[Install EspControl](/getting-started/install)**. After pairing it with Home Assistant, add one control and confirm it works before building the rest of your layout.
