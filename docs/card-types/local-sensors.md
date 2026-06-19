---
title: Local Sensor Cards
description:
  How to display live sensor readings directly from the ESP32 on EspControl cards without going through Home Assistant.
---

# Local Sensor

A local sensor card displays the value of any sensor component running on the device itself. It has two modes:

- **Numeric** — shows a large number with an optional unit and label. This is the default mode.
- **Text** — shows a chosen icon and displays the live text state where a normal card label would appear.

Local sensor cards are read-only — tapping them does nothing.

::: tip Works without Home Assistant
Because the value comes from the device itself, local sensor cards update and display correctly even when Home Assistant is offline or unreachable.
:::

## Setting Up a Local Sensor Card

No firmware changes are needed. Define your sensors in your ESPHome YAML as normal — EspControl discovers them automatically.

1. Select a card and change its type to **Local Sensor**.
2. Open the **Sensor** dropdown — the device is queried for its available sensors.
3. Select the sensor you want to display. The **Label** and **Unit** fields auto-fill from the sensor's name and unit of measurement. You can edit them freely.
4. Choose **Numeric** or **Text** mode. This is set automatically based on the sensor type but can be changed.

For **Numeric** mode:

1. Edit **Label** if you want different text under the value.
2. Edit **Unit** to change the unit label shown next to the value.
3. Set **Unit Precision** if you want one or two decimal places.

For **Text** mode:

1. Choose an **Icon**. This icon is always shown and does not change based on the sensor value.
2. The live state is shown where a Switch card label would normally appear.

### Internal Sensors

By default the dropdown shows only your own sensors. Check **Show internal sensors** to also see diagnostic sensors such as WiFi signal strength, uptime, and temperature calibration values.

### Device Unreachable

If the web UI cannot reach the device while you are configuring a card, a manual **Sensor Key** text field appears as a fallback. Enter the ESPHome `object_id` of the sensor (the lowercase, underscore-separated form of its name).

## How It Works on the Panel

- The device subscribes directly to the sensor's value updates — the display refreshes automatically whenever the sensor reports a new reading.
- The refresh rate is controlled by the sensor's own `update_interval` in your ESPHome YAML.
- Before the first value is received (for example, immediately after boot), the card shows `--`.
- Numeric mode uses the **tertiary** colour from [Appearance](/features/appearance), so it remains visually distinct from Switch and Trigger cards.
- Text mode uses the same tertiary colour while keeping the normal Switch-style icon and label layout.

## Example Sensors

| Sensor | Mode | Unit | What it shows |
|---|---|---|---|
| DHT22 temperature | Numeric | `°C` | Ambient temperature |
| DHT22 humidity | Numeric | `%` | Relative humidity |
| SCD40 CO₂ | Numeric | `ppm` | CO₂ concentration |
| BH1750 illuminance | Numeric | `lx` | Light level |
| Template text sensor | Text | | `Open`, `Closed`, or `Ajar` |

## Advanced: Pushing Values Manually

For values that are not ESPHome sensor entities — computed results, aggregations, or readings you do not want to expose as entities — you can push updates directly using `send_local_sensor_update()` in an `on_value:` lambda. The sensor key must match the value entered in the **Sensor Key** field (manual fallback mode).

```yaml
sensor:
  - platform: adc
    pin: GPIO34
    on_value:
      then:
        - lambda: send_local_sensor_update("raw_voltage", x);

text_sensor:
  - platform: template
    lambda: |-
      if (id(door_contact).state) return {"Open"};
      return {"Closed"};
    on_value:
      then:
        - lambda: send_local_sensor_update("door_state", x.c_str());
```

## When to Use It

Use local sensor cards when the sensor is part of the device's ESPHome config and you want to display its value without relying on Home Assistant. Use [Sensor](/card-types/sensors) cards when the data is already in Home Assistant and you want to pull it from there.
