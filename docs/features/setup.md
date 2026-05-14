---
title: EspControl Screen Setup
description:
  How to use the built-in web page to configure cards, icons, display settings, screensaver, and brightness on your EspControl panel.
---

# Setting Up Your Device's Screen

Your EspControl panel has a built-in web page where you can set everything up. Open it by typing the panel's address into any browser on your phone or computer.

![Screen setup page](/images/screen-setup.png)

::: tip Finding the address
The address is shown on the display screen when no cards are configured yet. You can also find it in your router's connected devices list, or in Home Assistant under **Settings > Devices & Services > ESPHome**.
:::

### Adding a Card

Tap any empty space in the grid (shown as a dashed outline with a **+** icon). A settings panel appears below the preview where you configure the card:

![Card settings panel](/images/button-settings.png)

![Switch card showing a Heater icon](/images/card-toggle.png)

The **Type** dropdown uses these card names on the device:

| Type | What it does | Needs an entity? |
|---|---|---|
| **[Switch](/card-types/switches)** | Controls a Home Assistant entity and shows its on/off state. This is the default card type. | Yes |
| **[Lights](/card-types/lights)** | Controls a light as a switch, brightness slider, or colour temperature slider. | Yes, as a light entity |
| **[Action](/card-types/actions)** | Runs a one-tap Home Assistant scene, script, button, or helper action. | Yes |
| **[Option Select](/card-types/option-select)** | Shows and changes the current option on a `select` or `input_select` entity. | Yes, as a select entity |
| **[Trigger](/card-types/buttons)** | Fires an event to Home Assistant for use in automations. | No |
| **[Sensor](/card-types/sensors)** | Shows a live numeric reading or text state. | Yes, as **Sensor Entity** |
| **[Slider](/card-types/sliders)** | Controls light brightness or fan speed with a draggable fill bar. | Yes |
| **[Cover](/card-types/covers)** | Controls blinds, shutters, and similar cover entities with a slider or tap action. | Yes |
| **[Garage Door](/card-types/garage-doors)** | Controls a garage door cover entity with an open/close tap action. | Yes |
| **[Lock](/card-types/locks)** | Locks, unlocks, or toggles a Home Assistant lock entity. | Yes |
| **[Date & Time](/card-types/calendar)** | Shows the local date, date and time, or a world clock. | No |
| **[Weather](/card-types/weather)** | Shows the current condition or today's/tomorrow's forecast from a weather entity. | Yes, as **Weather Entity** |
| **[Media](/card-types/media)** | Controls playback, volume, track position, or now-playing details for a media player. | Yes, as a media player entity |
| **[Climate](/card-types/climate)** | Controls a Home Assistant thermostat or HVAC entity. | Yes, as a climate entity |
| **[Internal Switches](/card-types/internal-relays)** | Controls a built-in relay locally on panels that have relay hardware. | Choose a relay |
| **[Subpage](/features/subpages)** | Opens a folder-like page of extra cards. | No |

For cards that use Home Assistant, enter the entity name from Home Assistant in the **Entity** field, such as `light.living_room`, `switch.garden_lights`, `scene.movie_mode`, or `weather.forecast_home`. Some card types use a more specific label, such as **Sensor Entity**, **Weather Entity**, or **Climate Entity**. You can find entity names under **Settings > Devices & Services** in Home Assistant.

Some card names group several related controls together. **Lights** contains Switch, Brightness, and Colour Temperature options. **Date & Time** contains Date, Time & Date, and World Clock options.

Most cards also let you choose an icon and set a label. If the label is left blank, the panel uses the friendly name from Home Assistant when it can.

### Active Switch Display

Each [Switch](/card-types/switches) card has separate **Off Icon** and **On Icon** settings. The on icon is used while the entity is active.

You can also turn on **Active Display**:

- **Numeric** — show a live reading instead of the icon, for example temperature, power usage, or a percentage. Pick the **Sensor Entity**, **Unit**, and **Unit Precision**.
- **Text** — show a live text state instead of the label, for example a machine status or current mode.

When the entity is not active, the card goes back to its off icon and normal label.

### Moving Cards

Drag and drop any card to reposition it. If you drop it onto an occupied space, the existing card shifts to the next available slot.

### Card Sizes

Right-click a card and open **Size** to choose:

- **Single** - normal one-slot card.
- **Tall** - spans two rows.
- **Wide** - spans two columns.
- **Large** - spans a 2 x 2 area.

If a card already occupies the space needed for a larger size, the setup page tries to move it to the next available slot. If there is not enough room, the size change is not applied.

## Device Settings

The **Settings** tab also includes display, brightness, screensaver, backup, and firmware update controls.

## Apply Configuration

After making changes, tap **Apply Configuration** at the bottom of the page. The panel restarts and loads your new settings — you'll see a message while it reconnects.
