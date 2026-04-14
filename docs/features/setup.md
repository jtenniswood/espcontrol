---
title: Screen Setup
description:
  How to use the built-in web page to configure buttons, icons, display settings, screensaver, and brightness on your Espcontrol panel.
---

# Setting up your device's screen

Your Espcontrol panel has a built-in web page where you can set everything up. Open it by typing the panel's address into any browser on your phone or computer.

![Screen setup page](/images/screen-setup.png)

::: tip Finding the address
The address is shown on the display screen when no buttons are configured yet. You can also find it in your router's connected devices list, or in Home Assistant under **Settings > Devices & Services > ESPHome**.
:::

### Adding a button

Tap any empty space in the grid (shown as a dashed outline with a **+** icon). A settings panel appears below the preview where you configure the button:

![Card settings panel](/images/button-settings.png)

![Toggle card showing a Heater icon](/images/card-toggle.png)

1. **Choose a type** — **Toggle** (the default) to control a device, **Subpage** to create a folder of extra cards, **[Button](/card-types/buttons)** to trigger Home Assistant automations with a single tap, **[Sensor](/card-types/sensors)** to display a live reading, **[Slider](/card-types/sliders)** to control light brightness, or **[Cover](/card-types/covers)** to control blinds and shutters.
2. **Pick the device** you want to control by entering its Home Assistant entity name (for example, `light.living_room` or `switch.garden_lights`). You can find these under **Settings > Devices & Services** in Home Assistant. Subpage buttons don't need a device name.
3. **Choose an icon** — type to search, or select **Auto** to let the panel pick one based on the device type.
4. **Set a label** (optional). If left blank the button uses the device's friendly name from Home Assistant.

**[Subpage](/features/subpages)** — works like a folder that opens a new page of buttons. See [Subpages](/features/subpages) for details.

**[Button](/card-types/buttons)** — a momentary button that fires an event to Home Assistant for use as an automation trigger. See [Buttons](/card-types/buttons) for details.

**[Sensor](/card-types/sensors)** — displays a live reading from a Home Assistant sensor entity. See [Sensors](/card-types/sensors) for details.

**[Slider](/card-types/sliders)** — controls light brightness with a draggable fill bar. See [Sliders](/card-types/sliders) for details.

**[Cover](/card-types/covers)** — controls blinds, shutters, and garage doors with a position slider. See [Covers](/card-types/covers) for details.

### When Entity On

Each toggle button has an optional **When Entity On** setting that changes what the button shows while the device is on:

- **Replace Icon** — show a different icon when the device is on (for example, an outline lightbulb when off and a filled one when on).
- **Sensor Data** — show a live reading when the device is on (for example, temperature, power usage, or a percentage). Pick the **Sensor** entity and a **Unit** (`%`, `°C`, `W`, etc.).

When the device is off, the button reverts to its normal icon.

### Moving buttons

Drag and drop any button to reposition it. If you drop it onto an occupied space, the existing button shifts to the next available slot.

### Double-height buttons

Right-click a button and choose **Double Height** to make it span two rows. To revert, right-click and choose **Single Height**. If a button already occupies the space below, it gets moved automatically.

## Apply Configuration

After making changes, tap **Apply Configuration** at the bottom of the page. The panel restarts and loads your new settings — you'll see a message while it reconnects.
