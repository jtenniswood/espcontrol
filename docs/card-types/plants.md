---
title: Plant Cards
description:
  How to show Home Assistant plant status and plant sensor readings on your EspControl panel.
---

# Plant

A Plant card displays one Home Assistant `plant` entity. It is read-only, so tapping it does not run any service or change the plant entity.

Plant cards can show either the plant status or one plant reading:

- **Status** shows whether Home Assistant reports the plant as OK or Problem. When Home Assistant provides a `problem` attribute, that text is shown for the issue.
- **Moisture** shows the plant moisture attribute.
- **Battery** shows the plant battery attribute.
- **Temperature** shows the plant temperature attribute.
- **Conductivity** shows the plant conductivity attribute.
- **Brightness** shows the plant brightness attribute.

## Setting Up a Plant Card

1. Select a card and change its type to **Plant**.
2. Enter a **Plant Entity**, such as `plant.monstera`.
3. Choose **Status**, **Moisture**, **Battery**, **Temperature**, **Conductivity**, or **Brightness**.
4. Set a **Label** if you want custom text under the reading.
5. On a **Large** card, turn on **Large Metric Numbers** for metric modes if you want the reading scaled larger.

## How It Works on the Panel

- Status mode follows the plant entity state. `ok` shows `OK`; `problem` shows the plant problem text when Home Assistant provides it, or `Problem` when it does not.
- Metric modes read attributes from the same plant entity, so you do not need to configure separate sensor entities.
- Units come from Home Assistant's `unit_of_measurement_dict` when available. The panel falls back to `%` for moisture and battery, the panel temperature unit for temperature, `µS/cm` for conductivity, and `lx` for brightness.
- Unavailable or unknown metric values show `--`.

## Example Plant Cards

| Entity | Mode | What it shows |
|---|---|---|
| `plant.monstera` | Status | `OK`, `Problem`, or the plant problem text |
| `plant.monstera` | Moisture | Soil moisture percentage |
| `plant.herbs` | Conductivity | Conductivity in `µS/cm` or the unit from Home Assistant |
| `plant.lemon_tree` | Brightness | Brightness in `lx` or the unit from Home Assistant |
