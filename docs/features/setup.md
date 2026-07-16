---
title: EspControl Screen Setup
description:
  How to use the built-in web page to configure cards, icons, display settings, screensaver, and brightness on your EspControl panel.
---

# Setting Up Your Device's Screen

Your EspControl panel has a built-in web page where you can set everything up. Open it by typing the panel's address into any browser on your phone or computer.

![Screen setup page](/images/screen-setup.png)

![Webserver card grid with examples of lights, climate, media, weather, covers, locks, actions, sensors, and date cards](/images/webserver-card-gallery.png)

::: tip Finding the address
The address is shown on the display screen when no cards are configured yet. You can also find it in your router's connected devices list, or in Home Assistant under **Settings > Devices & Services > ESPHome**.
:::

### Adding a Card

Tap any empty space in the grid (shown as a dashed outline with a **+** icon). A settings panel appears below the preview where you configure the card:

![Card settings panel](/images/button-settings.png)

![Light card settings panel showing card type, entity, label, and icon fields](/images/settings-panel-light-card.png)

![Switch card showing a Heater icon](/images/card-toggle.png)

The setup page uses these card names and grouped modes on the device. For a quick comparison, see the [Card Types overview](/card-types/).

| Type | What it does | Needs an entity? |
|---|---|---|
| **[Switch](/card-types/switches)** | Controls a Home Assistant entity and shows its on/off state. This is the default card type. | Yes |
| **[Lights](/card-types/lights)** | Controls a light as a full control popup, switch, brightness slider, or colour temperature slider. | Yes, as a light entity |
| **[Action](/card-types/actions)** | Runs a one-tap Home Assistant scene, script, button, helper action, Option Select picker, or local panel action. | Depends on the selected action |
| **[Option Select](/card-types/option-select)** | Opens a live `select` or `input_select` option list through the Action card. | Yes, as a select entity |
| **[Webhook](/card-types/webhooks)** | Calls an HTTP URL directly from the panel for other automation platforms and webhook services. | URL |
| **[Trigger](/card-types/buttons)** | Fires an event to Home Assistant for use in automations. | No |
| **[Sensor](/card-types/sensors)** | Shows a live numeric reading, readable duration, text state, or icon state from Home Assistant or a local device sensor. | Yes for Home Assistant, local sensor key for Local Sensor source |
| **[Doors & Windows](/card-types/doors-windows)** | Shows a door or window contact sensor with open and closed icons. | Yes, as **Sensor Entity** |
| **[Presence](/card-types/presence)** | Shows whether a person, room, or motion sensor is active. | Yes, as **Sensor Entity** |
| **[Slider](/card-types/sliders)** | Controls light brightness or fan speed with a draggable fill bar. | Yes |
| **[Fans](/card-types/fans)** | Controls supported fan switch, speed, oscillation, direction, and preset features. | Yes, as a fan entity |
| **[Vacuum](/card-types/vacuum)** | Shows vacuum status or controls start/stop, dock, pause/resume, spot clean, locate, and clean area. | Yes, as a vacuum entity |
| **[Lawn Mower](/card-types/lawn-mower)** | Shows mower status or controls start mowing, dock, and pause/resume. | Yes, as a lawn mower entity |
| **[Cover](/card-types/covers)** | Controls blinds, shutters, and similar cover entities with a full control popup, slider, or tap action. | Yes |
| **[Garage Door](/card-types/garage-doors)** | Controls a garage door cover entity with an open/close tap action. | Yes |
| **[Lock](/card-types/locks)** | Locks, unlocks, or toggles a Home Assistant lock entity. | Yes |
| **[Alarm](/card-types/alarms)** | Arms, disarms, or shows a Home Assistant alarm control panel. | Yes, as an alarm control panel entity |
| **[Date & Time](/card-types/calendar)** | Shows the local clock, date, date and time, or a world clock. | No |
| **[Weather](/card-types/weather)** | Shows the current condition or today's/tomorrow's forecast from a weather entity. | Yes, as **Weather Entity** |
| **[Media](/card-types/media)** | Controls playback, volume, track position, or now-playing details for a media player. | Yes, as a media player entity |
| **[Climate](/card-types/climate)** | Controls a Home Assistant thermostat or HVAC entity. | Yes, as a climate entity |
| **[Internal Switches](/card-types/internal-relays)** | Controls a built-in relay locally on panels that have relay hardware. | Choose a relay |
| **[Screen Lock](/card-types/screen-lock)** | Locks and unlocks local touchscreen controls on the panel. | No |
| **[Subpage](/features/subpages)** | Opens a folder-like page of extra cards. | No |

For cards that use Home Assistant, enter the entity name from Home Assistant in the **Entity** field, such as `light.living_room`, `switch.garden_lights`, `scene.movie_mode`, or `weather.forecast_home`. Some card types use a more specific label, such as **Sensor Entity**, **Weather Entity**, or **Climate Entity**. You can find entity names under **Settings > Devices & Services** in Home Assistant.

Some card names group several related controls together. **Lights** contains All Controls, Switch, Brightness, and Colour Temperature options. **Fans** contains Switch, Speed, Oscillation, Direction, and Preset options. **Action** contains scene, script, helper, Option Select, and Local Action modes. **Sensor** contains Home Assistant and Local Sensor sources. **Vacuum** contains Status, Start / Stop, Dock, Pause / Resume, Spot Clean, Locate, and Clean Area options. **Lawn Mower** contains Status, Start Mowing, Dock, and Pause / Resume options. **Cover** contains All Controls, Position, Tilt, Toggle, Open, Close, Stop, and Set Position options. **Alarm** contains Combined Control, Arm Away, Arm Home, Arm Night, Arm Vacation, and Disarm options. **Date & Time** contains Clock, Date, Time & Date, and World Clock options.

For the generated list of current card domains, subpage support, grouping, and options, see the [Card Capability Reference](/generated/cards/capabilities).

Most cards also let you choose an icon and set a label. If the label is left blank, the panel uses the friendly name from Home Assistant when it can.

### Active Switch Display

Each [Switch](/card-types/switches) card has separate **Off Icon** and **On Icon** settings. The on icon is used while the entity is active.

You can also turn on **Active Display**:

- **Numeric** — show a live reading instead of the icon, for example temperature, power usage, or a percentage. Pick the **Sensor Entity**, **Unit**, and **Unit Precision**.
- **Text** — show a live text state instead of the label, for example a machine status or current mode.

When the entity is not active, the card goes back to its off icon and normal label.

### Moving Cards

Drag and drop any card to reposition it. If you drop it onto an occupied space, the existing card shifts to the next available slot.

To copy cards to another controller without replacing its other settings, use **Copy Code** and **Paste Code**. See [Copying Cards Between Controllers](/features/subpages#copying-cards-between-controllers) for the steps and compatibility notes.

### Card Sizes

Right-click a card and open **Size** to choose:

- **Single** - normal one-slot card.
- **Tall** - spans two rows.
- **Extra Tall** - spans three rows.
- **Wide** - spans two columns.
- **Extra Wide** - spans three columns.
- **Large** - spans a 2 x 2 area.
- **Extra Large** - spans a 3 x 3 area and is available for Media cover-art cards.
- **Max wide** - spans a 3 x 2 area and is available for Camera cards.
- **Max tall** - spans a 2 x 3 area and is available for Camera cards.
- **Portrait** - spans a 3 x 4 area and is available for Camera and Media cover-art cards on both 10.1-inch panels.

If a card already occupies the space needed for a larger size, the setup page tries to move it to the next available slot. If there is not enough room, the size change is not applied.

## Device Settings

The **Settings** tab also includes display, brightness, screensaver, backup, and firmware update controls.

![Settings tab showing appearance, backlight, schedule, clock, and firmware controls](/images/settings-tab-display.png)

Open **Settings > System > Home Assistant Settings** to change **Home Assistant Port** if your Home Assistant instance does not use the default `8123` port. Camera/image cards and media artwork downloads use this port when loading images from Home Assistant.

## Changes Apply Automatically

Edits take effect on the panel by themselves a few seconds after you make them — no apply step and no restart. Layout changes (moving or resizing cards, grid size, rotation) show up immediately.

## Restart From Home Assistant

Each EspControl device has an enabled **Restart** button in Home Assistant. Open **Settings > Devices & services > ESPHome**, choose your EspControl device, and look under its configuration controls. Pressing **Restart** safely restarts the display without changing its cards, brightness, schedules, or other saved settings.

You can also press the button from a Home Assistant automation. Replace `button.your_panel_restart` with the Restart entity shown for your display:

```yaml
actions:
  - action: button.press
    target:
      entity_id: button.your_panel_restart
```

If the automation runs when Home Assistant starts, wait until the Restart entity is available before pressing it:

```yaml
actions:
  - wait_template: >
      {{ states.button.your_panel_restart is not none
         and not is_state('button.your_panel_restart', 'unavailable') }}
  - action: button.press
    target:
      entity_id: button.your_panel_restart
```

Setup-page changes apply automatically; use the Home Assistant **Restart** entity when you need to restart the display itself.
