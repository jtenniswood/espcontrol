# Guition ESP32-P4 JC1060P470 (7")

7-inch 1024x600 touch LCD panel with ESP32-P4, running ESPHome and LVGL for Home Assistant control. Up to 20 configurable buttons toggle any Home Assistant entity, with a live clock, indoor/outdoor temperature display, screensaver, and over-the-air firmware updates. Everything is configured through the built-in web UI — no YAML editing required after the initial flash.

![Guition ESP32-P4 JC1060P470](../images/guition-esp32-p4-jc1060p470.jpg)

## Features

- **20 button slots** — toggle any Home Assistant entity (lights, switches, fans, locks, covers, media players, etc.)
- **Drag-and-drop button ordering** — rearrange buttons from the web UI
- **19 built-in icons** plus Auto mode that picks an icon based on the entity domain
- **Custom labels** — or leave blank to use the entity's friendly name from Home Assistant
- **Configurable on/off button colours**
- **Indoor and outdoor temperature** — displayed in the top bar from any HA sensor entity
- **Clock** — synced from Home Assistant, shown in the top bar
- **Screensaver** — adjustable idle timeout (30s–30min) with snow effect; optional presence sensor to auto-wake
- **Adjustable backlight** brightness
- **OTA firmware updates** — automatic or manual, with configurable update frequency
- **Captive portal** — WiFi setup fallback if the network is unavailable
- **On-screen setup guides** — for WiFi connection and initial button configuration

## Getting Started

1. Copy the contents of [esphome.yaml](esphome.yaml) into a new device config in the ESPHome dashboard (or CLI) and set your device name and WiFi credentials
2. Flash the firmware to the device
3. The device boots with a loading screen and connects to WiFi
4. If WiFi fails, a setup screen guides you to connect to the device's fallback hotspot
5. Once online, if no buttons are configured, the screen shows the device URL
6. Open `http://<device-ip>` in your browser to configure buttons, display, and screensaver
7. Press **Apply Configuration** to restart with your settings

## Web UI

The built-in web interface has three tabs:

### Screen

- Live preview of the button layout at device resolution
- Add, remove, and reorder buttons with drag-and-drop
- Per-button settings: **Entity ID**, **Label** (optional), and **Icon** (dropdown with Auto option)

### Settings

- **Appearance** — button on/off colours (hex), display backlight brightness
- **Temperature** — enable/disable indoor and outdoor temperature, set the sensor entity IDs
- **Screensaver** — idle timeout (30–1800s in 30s steps), optional presence sensor entity to auto-wake the display

### Logs

- Live device log stream with level-based colour coding

> **Need an icon that isn't listed?** [Open an issue](https://github.com/jtenniswood/espcontrol/issues) on this repo and we'll look into adding it.

## Icons

Buttons support 19 icons from Material Design Icons (v7.4.47):

Lightbulb, Power Plug, Fan, Lock, Garage, Blinds Open, Blinds Closed, Thermometer, Speaker, Television, Camera, Motion Sensor, Door, Window, Water Heater, Air Conditioner, Battery, LED Strip, Power

Set a button's icon to **Auto** and it will be chosen based on the entity domain (e.g. `light.*` → Lightbulb, `fan.*` → Fan, `lock.*` → Lock).

## Firmware Updates

OTA updates are delivered via an HTTP manifest hosted on GitHub Pages. Update behaviour is controlled through Home Assistant entities:

- **Auto Update** — toggle automatic installation of new firmware
- **Update Frequency** — Hourly, Daily, Weekly, or Monthly
- **Check for Update** — trigger a manual check

## Where to Buy

- **Panel**: [AliExpress](https://s.click.aliexpress.com/e/_c335W0r5) (~£40)

## Stand

- **Desk mount** (3D printable): [MakerWorld](https://makerworld.com/en/models/2387421-guition-esp32p4-jc1060p470-7inch-screen-desk-mount#profileId-2614995)

## Folder Structure

```
guition-esp32-p4-jc1060p470/
├── addon/          # Connectivity, time, backlight, network, firmware update
├── assets/         # Fonts and icons
├── config/         # User-configurable button, display, and screensaver settings
├── device/         # Hardware, LVGL UI, sensors, setup screens
├── theme/          # Button and UI styling
└── esphome.yaml    # ESPHome config template
```
