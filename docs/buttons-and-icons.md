---
title: Buttons & Icons
description:
  How to set up buttons on your Espcontrol panel — choosing devices, picking icons, using Auto mode, and adding sensor readouts.
---

# Buttons & Icons

Your panel has **20 button slots**. Each button can control one device in Home Assistant — a light, switch, fan, lock, cover, media player, or anything else that can be toggled.

## Setting up a button

From the [Web UI](/web-ui) **Screen** tab:

1. **Tap an empty slot** to add a new button.
2. **Enter the entity ID** of the Home Assistant device you want to control (for example, `light.living_room` or `switch.garden_lights`). You can find entity IDs in Home Assistant under **Settings > Devices & Services**, or by looking at the entity in the developer tools.
3. **Choose an icon** from the dropdown, or select **Auto** (see below).
4. **Set a label** (optional). If you leave it blank, the button will show the device's friendly name from Home Assistant.

## Shared button settings

These settings apply to all buttons:

- **On colour** — the colour shown when a device is switched on. Default: orange (`FF8C00`).
- **Off colour** — the colour shown when a device is switched off. Default: dark grey (`313131`).
- **Button order** — the arrangement of buttons on the screen. You don't need to edit this manually — just drag and drop buttons in the web page to rearrange them.

## Auto icons

When you set a button's icon to **Auto**, the panel picks an appropriate icon based on the type of device:

| Device type | Icon shown |
| --- | --- |
| Light | Lightbulb |
| Switch | Power plug |
| Fan | Fan |
| Lock | Lock |
| Cover (blinds, shutters) | Horizontal blinds |
| Climate (heating, AC) | Air conditioner |
| Media player | Speaker |
| Camera | Camera |
| Binary sensor (motion, door) | Motion sensor |
| Anything else | Gear (cog) |

If you'd rather pick a specific icon, the dropdown offers hundreds of choices from the [Material Design Icons](https://pictogrammers.com/library/mdi/) set — covering lighting, climate, security, weather, media, and more.

## Sensor readouts

Each button can optionally display a sensor value alongside the icon and label. This is useful for showing a temperature, humidity, or power reading on the same button that controls the device.

- **Sensor** — enter the entity ID of a Home Assistant sensor (for example, `sensor.living_room_temperature`).
- **Sensor unit** — the unit to display (for example, `°C` or `W`).

## Missing an icon?

If the icon you need isn't in the list, [open an issue](https://github.com/jtenniswood/espcontrol/issues) with the icon name and what you'd use it for, and we'll look into adding it.
