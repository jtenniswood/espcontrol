---
title: "Home Assistant Sensor Cards"
description:
  How to display live readings, durations, text, or icon states from Home Assistant on EspControl.
---

# Sensor

A Sensor card is read-only. It displays a Home Assistant `sensor`, `binary_sensor`, or `text_sensor`; it can also use a [Local Sensor](/card-types/local-sensors) from the panel itself.

![Sensor card showing 0 kph wind speed](/images/card-sensor.png)

## Set Up a Sensor Card

1. Select a card and change its type to **Sensor**.
2. Leave **Source** as **Home Assistant**, choose the display type, and enter the sensor entity.
3. Set a label, unit, and icon as needed.

| Type | What it shows |
|---|---|
| **Numeric** | A live number, optional unit, and label. **Large Sensor Numbers** is available on Large cards. |
| **Time** | A compact duration such as `36m` or `1h 30m`. Leave **Incoming Value Unit** on Auto unless the entity does not report a usable unit. |
| **Text** | The live state beside a chosen icon. Advanced settings can replace up to two raw states with friendlier text. |
| **Icon** | A normal and optional active icon for a status-style sensor. |

Open **Custom Colours** and choose a **Colour mode**. **Single colour** uses the same swatch picker as other cards. **Lit When Active** uses that colour with a lighter active shade; numeric values above zero count as active, while Text and Icon cards follow recognised Home Assistant active states.

Choose **Custom conditions**, then select a comparison, enter a value, and pick a colour from the round swatches. A checkmark identifies the selected colour. Previously saved colours outside the standard palette remain available as an extra swatch. For example, choose **Below**, enter `8`, and choose red to colour the card whenever the sensor drops below 8. **Above**, **Exactly**, **Between**, and **Text is** cover other values and states. Between includes both the **From** and **To** values. Add up to six conditions. They are checked in order, and the first match wins; use the arrow buttons to change priority.

The always-visible **Use value from** selector defaults to **Main Entity**, the sensor displayed on the card. Choose **Another sensor** to colour a card from a second Home Assistant sensor, such as showing a battery percentage while charge or discharge power selects the colour. Entity IDs must be lowercase, as they are in Home Assistant. Text states match without regard to case, including Unicode text, consistently in the preview and on the panel. If no rule matches, or the colour source is unknown or unavailable, the card returns to the theme default. The **Default colour** row always stays below the conditions: choose **Custom colour** there to use a fallback swatch instead. This fallback is separate from the single card colour and does not count towards the six-condition limit. A value is compared before display rounding, so a reading such as `-0.04` still matches a range below zero even if the card displays `0`.

Open the **Test your conditions** panel to enter a sample value or text state. It starts closed. The card preview shows the resulting background, updating as you edit. The test state does not change Home Assistant and is not saved. The reset icon in the **Custom Colours** header clears the single colour, conditions and fallback colour without closing the section. **Save** commits the change; **Cancel** keeps the saved configuration. Custom conditions require panel firmware that advertises support; update older firmware before using them. The six-rule limit applies to each card. Existing configuration-size limits still apply, and the editor rejects an oversized save rather than dropping rules.

## Useful Details

- A Time card needs a value in days, hours, minutes, seconds, milliseconds, or microseconds. You can select the unit manually when Auto cannot identify it.
- Unknown, unavailable, or invalid values are left blank rather than guessed.
- Changes made in Home Assistant update the panel automatically.
- To show a device-local sensor, change **Source** to **Local Sensor** and follow the [Local Sensor](/card-types/local-sensors) setup.

| Example entity | Suggested type |
|---|---|
| `sensor.living_room_temperature` | Numeric |
| `sensor.ups_battery_runtime` | Time |
| `binary_sensor.laundry_running` | Icon |
| `text_sensor.washing_machine_status` | Text |
