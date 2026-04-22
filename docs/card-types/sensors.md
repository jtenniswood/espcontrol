---
title: Sensors
description:
  How to display live numeric readings or text states from Home Assistant on your Espcontrol panel.
---

# Sensors

A sensor card displays live Home Assistant data. It has two modes:

- **Numeric** — shows a large number with an optional unit and label. This is the default mode.
- **Text** — shows a chosen icon and displays the live text state where a normal card label would appear.

Sensor cards are read-only — tapping them does nothing.

![Sensor card showing 0 kph wind speed](/images/card-sensor.png)

## Setting Up a Sensor Card

1. Select a card and change its type to **Sensor**.
2. Choose **Numeric** or **Text** from the mode tabs. Numeric is selected by default.
3. Enter a **Sensor Entity** — the Home Assistant entity ID of the sensor you want to display.

For **Numeric** mode:

1. Set a **Unit** — the unit label shown next to the value, for example `°C`, `%`, `W`, or `kWh`.
2. Set a **Label** if you want custom text under the value. If left blank, the entity name from Home Assistant is used.
3. Set **Unit Precision** if you want one or two decimal places.

For **Text** mode:

1. Choose an **Icon**. This icon is always shown and does not change based on the sensor value.
2. The live state from Home Assistant is shown where a toggle label would normally appear.

## How It Works on the Panel

- Numeric mode displays the current value in large text, with the unit beside it and the label underneath.
- Numeric mode uses the **tertiary** colour from [Appearance](/features/appearance), so it remains visually distinct from toggles and buttons.
- Text mode uses the same tertiary colour as Numeric mode, while keeping the normal toggle-style icon and label layout.
- Text mode formats states in sentence case. For example, `not_home` displays as `Not home`.

## Example Sensors

| Entity | Mode | Unit | What it shows |
|---|---|---|---|
| `sensor.living_room_temperature` | Numeric | `°C` | Indoor temperature |
| `sensor.solar_power` | Numeric | `W` | Current solar generation |
| `sensor.humidity` | Numeric | `%` | Relative humidity |
| `text_sensor.washing_machine_status` | Text |  | `Running`, `Rinsing`, or `Finished` |
| `sensor.fan_level` | Text |  | `low`, `medium`, or `high` |
