---
title: Weather Cards
description:
  How to show the current Home Assistant weather condition or tomorrow's forecast on your EspControl panel.
---

# Weather

A weather card displays weather information from a Home Assistant weather entity. It can show either the current condition, such as **Sunny**, **Cloudy**, or **Rainy**, or tomorrow's high / low forecast, such as **18/10°C**.

Weather cards are read-only — tapping them does nothing.

Older cards that were created as **Weather Forecast** cards still work. They now load as **Weather** cards with **Display** set to **Tomorrow**.

## Setting Up a Weather Card

1. Select a card and change its type to **Weather**.
2. Enter a **Weather Entity** — the Home Assistant weather entity ID you want to display, for example `weather.forecast_home`.
3. Choose **Display**:
   - **Current** shows the live weather condition icon and label.
   - **Tomorrow** shows tomorrow's forecast high / low temperature.

## How It Works on the Panel

- In **Current** mode, the card watches the weather entity's current state.
- In **Current** mode, the icon changes automatically and the label uses the condition name from Home Assistant.
- In **Tomorrow** mode, the card asks Home Assistant for the daily forecast for the configured weather entity.
- In **Tomorrow** mode, the unit label comes from the panel's **Temperature Unit** setting.
- In **Tomorrow** mode, the card label on the panel always says **Tomorrow**.
- If Home Assistant reports `unknown`, `unavailable`, or an unexpected current condition, the card shows a fallback weather icon and a readable label.
- If tomorrow's forecast is missing or unavailable, the card shows **--/--** instead of leaving the card blank.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like Sensor, Date, and World Clock cards.

::: tip Home Assistant actions permission
The **Tomorrow** display needs the same **Allow the device to perform Home Assistant actions** setting as control cards. EspControl uses that permission to request forecast data from Home Assistant.
:::

## Supported Conditions

| Home Assistant state | What the card shows |
|---|---|
| `sunny` | Sunny |
| `clear-night` | Clear night |
| `partlycloudy` | Partly cloudy |
| `cloudy` | Cloudy |
| `fog` | Fog |
| `hail` | Hail |
| `lightning` | Lightning |
| `lightning-rainy` | Lightning and rain |
| `pouring` | Pouring |
| `rainy` | Rainy |
| `snowy` | Snowy |
| `snowy-rainy` | Snowy and rain |
| `windy` | Windy |
| `windy-variant` | Windy and cloudy |
| `exceptional` | Exceptional |
| `unknown` | Unknown |
| `unavailable` | Unavailable |
