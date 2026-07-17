---
title: Card Types
description:
  Quick reference for choosing the right EspControl card type for Home Assistant controls, sensors, local panel actions, and subpages.
---

# Card Types

Cards are the controls and information tiles shown on the EspControl screen. Each card type is built for a specific job: some send Home Assistant actions, some display live state, some control local panel hardware, and some open extra pages.

Use this page when you know what you want the panel to do but are not sure which card type to choose.

## Choose a Card

| Goal | Use this card | Entity or target |
|---|---|---|
| Toggle a light, switch, fan, or helper | [Switch](/card-types/switches) | `light`, `switch`, `input_boolean`, or `fan` |
| Control a light as on/off, brightness, colour temperature, or colour presets | [Lights](/card-types/lights) | `light` |
| Run a scene, script, automation, helper action, button press, or local panel action | [Action](/card-types/actions) | Depends on the selected action |
| Show or control a robot vacuum | [Vacuum](/card-types/vacuum) | `vacuum` |
| Show or control a robotic lawn mower | [Lawn Mower](/card-types/lawn-mower) | `lawn_mower` |
| Pick from a Home Assistant select list | [Option Select](/card-types/option-select) | `select` or `input_select` |
| Fire a custom Home Assistant automation event | [Trigger](/card-types/buttons) | No entity required |
| Call an HTTP URL directly from the panel | [Webhook](/card-types/webhooks) | URL |
| Show a live number, readable duration, text state, or active/inactive icon | [Sensor](/card-types/sensors) | Home Assistant `sensor`, `binary_sensor`, or `text_sensor`; or a Local Sensor source |
| Show a door or window contact sensor | [Doors & Windows](/card-types/doors-windows) | `binary_sensor` or `sensor` |
| Show presence, motion, person, or tracker state | [Presence](/card-types/presence) | `binary_sensor`, `sensor`, `text_sensor`, `person`, or tracker helpers |
| Drag to set light brightness or fan speed | [Slider](/card-types/sliders) | `light` or `fan` |
| Use grouped fan controls | [Fans](/card-types/fans) | `fan` |
| Control blinds, shutters, shades, gates, position, or tilt | [Cover](/card-types/covers) | `cover` |
| Open or close a garage door | [Garage Door](/card-types/garage-doors) | `cover` |
| Open, close, or stop a gate | [Gate](/card-types/gates) | `cover` |
| Lock or unlock a door lock | [Lock](/card-types/locks) | `lock` |
| Arm, disarm, or show an alarm panel | [Alarm](/card-types/alarms) | `alarm_control_panel` |
| Show local date, time, or date and time | [Date & Time](/card-types/calendar) | No entity required for clock modes |
| Show another city or timezone | [World Clock](/card-types/timezones) | No entity required |
| Show current weather or daily high/low temperatures | [Weather](/card-types/weather) | `weather` |
| Show a still image from Home Assistant | [Camera](/card-types/cameras) | `camera` or `image` |
| Control media playback, volume, progress, or now-playing display | [Media](/card-types/media) | `media_player` |
| Control a thermostat or HVAC entity | [Climate](/card-types/climate) | `climate` |
| Control a built-in relay on the panel itself | [Internal Switches](/card-types/internal-relays) | Built-in relay |
| Lock or unlock the touchscreen controls locally | [Screen Lock](/card-types/screen-lock) | No entity required |
| Start the configured panel screensaver immediately | [Screensaver](/card-types/screensaver) | No entity required |
| Open another page of cards | [Subpage](/features/subpages) | No entity required, optional state entity |

## Entity-Based and Local Cards

Most cards use Home Assistant entities. The entity ID is the exact name Home Assistant uses, such as `light.kitchen`, `sensor.outdoor_temperature`, or `media_player.living_room`.

Some cards do not need a Home Assistant entity:

- **Trigger** sends a custom event that Home Assistant automations can listen for.
- **Local Sensor** shows a value supplied by the panel firmware.
- **Webhook** sends a direct HTTP request.
- **Action > Local Action** runs a registered callback on the panel itself.
- **Date & Time**, **World Clock**, **Screen Lock**, **Screensaver**, and **Subpage** can work from the panel itself.
- **Internal Switches** controls built-in relay hardware on supported panels.

## Grouped Card Types

Some names in the setup page group several related modes:

| Setup page name | Modes |
|---|---|
| **Lights** | All Controls, Switch, Brightness, Colour Temperature |
| **Fans** | Switch, Speed, Oscillation, Direction, Preset |
| **Action** | Scene, Script, Automation, Button, Helper, Option Select, Local Action |
| **Vacuum** | Status, Start / Stop, Dock, Pause / Resume, Spot Clean, Locate, Clean Area |
| **Lawn Mower** | Status, Start Mowing, Dock, Pause / Resume |
| **Alarm** | Combined Control, Arm Away, Arm Home, Arm Night, Arm Vacation, Disarm |
| **Date & Time** | Clock, Date, Time & Date, World Clock |
| **Media** | Play/Pause, Previous, Next, Volume, Track Position, Now Playing |
| **Cover** | All Controls, Position, Tilt, Toggle, Open, Close, Stop, Set Position |
| **Subpage** | Generic, Switch, Lights, Climate, Presence, Media, Alarm, Cover, Garage Door, Gate, Lock, Vacuum, Lawn Mower, Weather, Sensor, Camera / Image |

## Permissions

Cards that control Home Assistant need the panel to be allowed to perform Home Assistant actions. If a control card displays correctly but tapping it does nothing, check [Enable Actions](/getting-started/home-assistant-actions).

Read-only display cards such as Sensor, Presence, Date & Time, and current Weather state can still show information without sending control actions. Weather forecast modes also need Home Assistant actions permission because the panel asks Home Assistant for forecast data.

## Current Capability Reference

For a generated table of card type names, saved type values, supported entity domains, subpage support, picker grouping, options, and visibility status, see the [Card Capability Reference](/generated/cards/capabilities).
