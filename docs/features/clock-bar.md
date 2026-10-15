---
title: "Clock Bar"
description:
  Add sensor readings and arrange the status elements at the top of your EspControl panel.
---

# Clock Bar

The clock bar is the narrow status area at the top of the panel. Enable it in **Screen > Appearance > Clock Bar**, then edit its elements in the **Layout** preview.

## Add and arrange elements

Tap the dashed **+** slot at the end of the clock bar in the **Layout** preview to add an element. It stays available while a card or clock bar element is selected. The slot is an editor control and is not shown on the physical panel; it becomes disabled at the 12-element limit. With no card selected, **Manage clock bar** above the preview also lists all configured elements. Select an existing element to **Edit**, **Duplicate**, or **Delete** it. Right-clicking an element offers the same actions.

Choose **Left**, **Middle**, or **Right** in the element editor. Elements sit side by side. Drag an element within its group to reorder it, or into another group to move it. **Move earlier** and **Move later** provide the same ordering controls without dragging.

The middle group is centred in the space between the outer groups. All pages share one layout. On subpages and while a card control or Settings is open, the page or control title replaces the left group and can temporarily displace middle elements. Closing the control restores the normal arrangement.

Long readings are truncated. When the bar is full, middle elements are omitted first, followed by left elements nearest the middle, then right elements nearest the middle. These elements remain configured and return when space is available. **Manage clock bar** lists them even when they are hidden in the preview. The preview uses example sensor values; check the physical panel with your actual readings.

## Available elements

- **Sensors:** Temperature, Humidity, Battery level, Power, Energy, Illuminance, Pressure and CO₂. Choose **Home Assistant** and a `sensor` entity, or **Local sensor** and a numeric sensor on the panel. Local readings continue working without Home Assistant. The local picker includes internal diagnostics; you can also enter the sensor's ESPHome `object_id` manually.
- **Clock:** follows the device's [time format and timezone](/features/clock).
- **Connectivity / settings:** shows network status. Tap it on the panel to open Settings.
- **Night mode:** shows a moon while the [Night Schedule](/features/screen-schedule) is in its night period. Its space closes up when inactive.
- **Device battery status:** available on battery-capable hardware.

Sensor elements support an optional icon, short label, unit suffix and decimal places. Choose an icon using the same searchable picker as cards; it appears to the left of the label and reading. Choose **None** to remove it. Set the unit to match the sensor's reading; changing the suffix does not convert its value. All styling follows the panel theme. Missing or unavailable readings show `--`.

A bar supports up to 12 elements, including duplicates. Every built-in element can be deleted and added again. If you delete every connectivity/settings element, add one back through the web editor to restore that settings entry point on the panel.

Existing clock bar arrangements are preserved until you edit them. The complete editable layout is saved on the panel and included in configuration backups. Older firmware retains its fixed-layout controls; update the firmware to use the editable bar.

## Panel Settings

Tap a connectivity/settings element to open Settings, including while another control is open. Settings closes that control first; **Back** returns to the underlying page. The cards use the device's normal grid and show **Back**, **IP address**, **Wi-Fi quality** on Wi-Fi builds, **Build**, then the device name.

Once a PIN has been saved in **Pin Protection** or a [Screen Lock card](/card-types/screen-lock), Settings also shows **Screen Lock**. It uses the same PIN and remains available with **Require PIN after wake** off. Clearing the saved PIN removes the button.
