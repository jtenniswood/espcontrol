---
title: Sensors
description:
  How to display live Home Assistant sensor data on your Espcontrol panel.
---

# Sensors

A sensor card displays a live reading from a Home Assistant sensor entity. It shows a large value with an optional unit and a label underneath — useful for temperatures, humidity, power usage, or any numeric sensor.

Sensor cards are read-only — tapping them does nothing.

![Sensor card showing 0 kph wind speed](/images/card-sensor.png)

## Setting up a sensor card

1. Select a card and change its type to **Sensor**.
2. Enter a **Sensor Entity** — the Home Assistant entity ID of the sensor you want to display (for example, `sensor.living_room_temperature`).
3. Set a **Unit** — the unit label shown next to the value (for example, `°C`, `%`, `W`, or `kWh`).
4. Set a **Label** (optional) — shown below the value. If left blank, the entity ID is used.
5. Set **Unit precision** (optional) — choose how many decimal places to show. Options are `10` (whole number, the default), `10.2` (one decimal place), or `10.21` (two decimal places).

## How it works on the panel

- The card displays the sensor's current value in large text, with the unit beside it and the label underneath.
- The value updates in real time as Home Assistant reports new readings.
- By default, numeric values are rounded to a whole number. Use the **Unit precision** setting to show one or two decimal places instead.
- The card uses the **tertiary** colour (configurable in [Appearance](/features/appearance)) as its background, so sensor cards are visually distinct from toggles and buttons.

## Example sensors

| Entity | Unit | Label | What it shows |
|---|---|---|---|
| `sensor.living_room_temperature` | `°C` | Living Room | Indoor temperature |
| `sensor.solar_power` | `W` | Solar | Current solar generation |
| `sensor.humidity` | `%` | Humidity | Relative humidity |
| `sensor.daily_energy` | `kWh` | Today | Energy used today |
