---
title: Sensor Cards
description:
  How to display live numeric readings, durations, text states, or icon states from Home Assistant or local device sensors on your EspControl panel.
---

# Sensor

A sensor card displays live sensor data. By default it uses Home Assistant entities, and it can also use local sensors running directly on the display device.

It has four Home Assistant types:

- **Numeric** — shows a large number with an optional unit and label. This is the default mode.
- **Time** — turns a numeric duration into compact text such as `36m`, `1h 30m`, or `1d 4h`.
- **Text** — shows a chosen icon and displays the live text state where a normal card label would appear.
- **Icon** — shows an icon and can change to an on icon when the sensor is active.

Sensor cards are read-only — tapping them does nothing.

![Sensor card showing 0 kph wind speed](/images/card-sensor.png)

## Setting Up a Sensor Card

1. Select a card and change its type to **Sensor**.
2. Leave **Source** set to **Home Assistant**. This is the default.
3. Choose **Numeric**, **Time**, **Text**, or **Icon** from the **Type** dropdown. Numeric is selected by default.
4. Enter a **Sensor Entity** — the Home Assistant entity ID of the sensor you want to display.

To display a sensor that runs directly on the device, change **Source** to **Local Sensor**. See [Local Sensor](/card-types/local-sensors) for the local setup details.

For **Icon** mode:

1. Choose an **Icon** for the normal state.
2. Choose an **On Icon** if you want a different icon when the sensor is active.
3. Set a **Label** if you want custom text. If left blank, the entity name from Home Assistant is used.

For **Numeric** mode:

1. Set a **Unit** — the unit label shown next to the value, for example `°C`, `%`, `W`, or `kWh`.
2. Set a **Label** if you want custom text under the value. If left blank, the entity name from Home Assistant is used.
3. Set **Unit Precision** if you want one or two decimal places.
4. On a **Large** card, turn on **Large Sensor Numbers** if you want the top sensor readout scaled much larger.

For **Time** mode:

1. Set a **Label** if you want custom text under the duration. If left blank, the entity name from Home Assistant is used.
2. Leave **Input Unit** set to **Auto** when the entity has a supported Home Assistant unit of measurement. Auto recognises days (`d`), hours (`h`), minutes (`min`), seconds (`s`), milliseconds (`ms`), and microseconds (`µs`).
3. If the entity has no unit, or its unit is not supported, choose **Seconds**, **Minutes**, **Hours**, or **Days** manually. A manual choice overrides the Home Assistant unit.

Time values are rounded down to whole seconds so a remaining-runtime reading is never overstated. The card shows no more than the two largest non-zero parts: `90` seconds becomes `1m 30s`, `28` hours becomes `1d 4h`, and zero becomes `0s`. Unavailable, unknown, malformed, infinite, and negative values leave the duration blank.

::: info Time mode limitations
Time is available only for the **Home Assistant** source. It does not currently support **Large Sensor Numbers**, and its fixed `d`, `h`, `m`, and `s` abbreviations are not translated.
:::

For **Text** mode:

1. Choose an **Icon**. This icon is always shown and does not change based on the sensor value.
2. The live state from Home Assistant is shown where a Switch card label would normally appear.
3. Open **Advanced** if you want to replace raw Home Assistant states with friendlier labels. For example, you can show `Please empty` when the sensor reports `high`, and `Full` when another sensor state reports `low`.

For **Numeric**, **Text**, or **Icon** mode, turn on **Lit When Active** if you want the card background to use the active/on colour while Home Assistant reports an active state. This is useful for status sensors such as a washing machine running or a door being open. The option is not shown in Time mode.

## How It Works on the Panel

- Icon mode treats active Home Assistant states such as `on`, `true`, `home`, `playing`, `open`, or `unlocked` as active and uses the on icon when configured.
- When **Lit When Active** is enabled, Text and Icon cards use the active/on background colour for those active states. Numeric cards treat any positive value as active. All three return to the Sensor card colour when the state is inactive, zero, unknown, or unavailable.
- Numeric mode displays the current value in large text, with the unit beside it and the label underneath.
- Time mode listens for both the value and its Home Assistant unit when **Input Unit** is Auto. A change to either one updates the card. Missing or unsupported unit metadata leaves the value blank rather than guessing.
- Numeric mode normally uses the fixed **tertiary** background colour, so it remains visually distinct from Switch and Trigger cards.
- Text mode normally uses the same tertiary colour as Numeric mode, while keeping the normal Switch-style icon and label layout.
- Text mode capitalises each word in the Home Assistant text and preserves line breaks. Advanced status translation is applied before the text is shown. Very long values are limited to roughly 256 characters so the panel stays responsive.

## Example Sensors

| Entity | Mode | Unit | What it shows |
|---|---|---|---|
| `sensor.living_room_temperature` | Numeric | `°C` | Indoor temperature |
| `sensor.solar_power` | Numeric | `W` | Current solar generation |
| `sensor.humidity` | Numeric | `%` | Relative humidity |
| `sensor.ups_battery_runtime` | Time | Auto | `36m` for a value of `0.6` hours |
| `sensor.timer_remaining` | Time | Seconds | `1m 30s` for a value of `90` |
| `binary_sensor.laundry_running` | Icon |  | Laundry running or idle |
| `text_sensor.washing_machine_status` | Text |  | `Running`, `Rinsing`, or `Finished` |
| `sensor.fan_level` | Text |  | `low`, `medium`, or `high` |
