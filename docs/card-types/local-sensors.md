---
title: Local Sensor Cards
description:
  How to display readings from ESPHome sensors on the panel itself.
---

# Local Sensor

Local Sensor is a **Sensor** card source for a sensor or text sensor running on the display device. It is read-only and continues to work while Home Assistant is unavailable.

## Set Up a Local Sensor

1. Define the sensor normally in the device's ESPHome YAML.
2. Select a card, change its type to **Sensor**, then set **Source** to **Local Sensor**.
3. Choose the sensor. The label and unit are filled in for you and can be changed.
4. Choose **Numeric** for a value or **Text** for a live text state.

The picker normally shows your own sensors. Turn on **Show internal sensors** to include diagnostics such as Wi-Fi signal strength. If the setup page cannot reach the panel, enter the ESPHome sensor `object_id` as the **Sensor Key**.

The card updates at the sensor's normal ESPHome update rate. It shows `--` until the first reading arrives.

For values already in Home Assistant, keep the standard [Sensor](/card-types/sensors) source instead.
