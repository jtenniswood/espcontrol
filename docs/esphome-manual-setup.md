---
title: ESPHome Manual Setup
description:
  How to install Espcontrol using the ESPHome Dashboard or CLI instead of the browser-based installer — for users who already run ESPHome and want to compile firmware themselves.
---

# ESPHome Manual Setup

If you already use ESPHome and prefer to compile firmware yourself, you can install Espcontrol as a remote package instead of using the [browser-based installer](/install). This gives you full control over the build and lets you extend the configuration with your own customisations.

## What you need

- **ESPHome** — either the [Home Assistant add-on](https://esphome.io/guides/getting_started_hassio.html) or the [CLI](https://esphome.io/guides/getting_started_command_line.html)
- **Guition ESP32-P4 JC1060P470** (7-inch display)
- **USB-C data cable** for the first flash (OTA updates work after that)

## Create the config file

Create a new YAML file in your ESPHome config directory (e.g. `office-screen.yaml`) with the following contents:

```yaml
substitutions:
  name: "office-screen"
  friendly_name: "Office Screen"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/jtenniswood/espcontrol/
    file: devices/guition-esp32-p4-jc1060p470/packages.yaml
    refresh: 1sec
```

Change `name` and `friendly_name` to whatever you like. The name must be lowercase with hyphens (no spaces) — it becomes the device hostname on your network.

::: tip Refresh interval
`refresh: 1sec` tells ESPHome to check for package updates on every compile. You can increase this (e.g. `1d` for daily) once you're happy with the setup.
:::

## Build and flash

### ESPHome Dashboard (Home Assistant add-on)

1. Open the ESPHome Dashboard in Home Assistant.
2. Click **+ New Device**, choose **Skip** on the wizard, and paste the YAML above.
3. Save the file and click **Install → Plug into this computer**.
4. Select the serial port for your display and wait for the build to finish.

### ESPHome CLI

```bash
esphome run office-screen.yaml
```

ESPHome compiles the firmware, pulls in all remote packages from GitHub, and flashes the device over USB. After the first flash, you can use `--device OTA` or let the dashboard push updates wirelessly.

## After flashing

The display behaves exactly as if you'd used the browser installer:

1. It creates a WiFi hotspot if it can't connect to your network — follow the [WiFi setup steps](/install#connect-to-wifi).
2. Home Assistant discovers it automatically — follow [Add to Home Assistant](/install#add-to-home-assistant).
3. Configure buttons, display, and settings from the built-in [Web UI](/web-ui).

## Adding your own customisations

Because the remote package is just a standard ESPHome `packages` include, you can add extra YAML below it. Anything you define in your local file merges with (or overrides) the remote config.

```yaml
substitutions:
  name: "office-screen"
  friendly_name: "Office Screen"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

packages:
  setup:
    url: https://github.com/jtenniswood/espcontrol/
    file: devices/guition-esp32-p4-jc1060p470/packages.yaml
    refresh: 1sec

# Your own additions below
sensor:
  - platform: uptime
    name: "${friendly_name} Uptime"
```

See the [Package Layout](/package-layout) page for details on how the remote packages are structured.

## Related

- [Install](/install) — browser-based installer (no ESPHome required)
- [Package Layout](/package-layout) — how the firmware packages are organised
- [Firmware Updates](/firmware-updates) — OTA update settings
