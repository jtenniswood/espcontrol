---
title: Weather Cards
description:
  How to show current Home Assistant weather conditions, daily high / low temperatures, or a 3-day forecast on your EspControl panel.
---

# Weather

A weather card displays weather information from a Home Assistant weather entity. It can show the current condition, such as **Sunny**, **Cloudy**, or **Rainy**, the high / low temperatures for today or tomorrow, such as **18/10°C**, or a compact 3-day high / low forecast.

Weather cards are read-only — tapping them does nothing.

Older cards that were created as **Weather Forecast** cards still work. They now load as **Weather** cards with **Display** set to **Temperatures Tomorrow**.

![Weather card showing today's high and low temperatures](/images/card-weather.png)

## Setting Up a Weather Card

1. Select a card and change its type to **Weather**.
2. Enter a **Weather Entity** — the Home Assistant weather entity ID you want to display, for example `weather.forecast_home`.
3. Choose **Display**:
   - **Current Conditions** shows the live weather condition icon and label.
   - **Temperatures Today** shows today's high / low temperature.
   - **Temperatures Tomorrow** shows tomorrow's high / low temperature.
   - **3-Day Forecast** shows three daily high / low rows.
4. For temperature displays, optionally enter a **Label** to override the default card label.
5. On a **Large** card, turn on **Large Temperature Numbers** if you want the Today or Tomorrow high / low reading scaled much larger.

## How It Works on the Panel

- In **Current Conditions** mode, the card watches the weather entity's current state.
- In **Current Conditions** mode, the icon changes automatically and the label uses the condition name from Home Assistant.
- In **Temperatures Today**, **Temperatures Tomorrow**, and **3-Day Forecast** modes, the card asks Home Assistant for the daily forecast for the configured weather entity.
- **3-Day Forecast** mode shows the next three daily forecast entries as short weekday high / low rows.
- In temperature modes, the unit label comes from the panel's **Temperature Unit** setting.
- In temperature modes, the card label defaults to **Today**, **Tomorrow**, or **3-Day Forecast**, unless you set your own label.
- If Home Assistant reports `unknown`, `unavailable`, or an unexpected current condition, the card shows a fallback weather icon and a readable label.
- If the requested forecast is missing or unavailable, the card shows **--/--** instead of leaving the card blank.
- The card uses the fixed **tertiary** background colour, like Sensor, Date, Clock, and World Clock cards.

::: tip Home Assistant actions permission
The forecast displays need the same **Allow the device to perform Home Assistant actions** setting as control cards. EspControl uses that permission to request forecast data from Home Assistant.
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
