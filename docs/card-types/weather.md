---
title: Weather Cards
description:
  How to show current Home Assistant weather conditions, daily high / low temperatures, or multi-day and hourly forecast strips on your EspControl panel.
---

# Weather

A weather card displays weather information from a Home Assistant weather entity. It can show the current condition, such as **Sunny**, **Cloudy**, or **Rainy**, the high / low temperatures for today or tomorrow, such as **18/10°C**, a combined **Hero** view, or a horizontal **forecast strip** of the next several days or hours.

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
   - **Hero (current + today)** combines the live condition icon and label with today's high / low temperature in one card.
   - **Daily Strip (5 days)** shows a horizontal row of the next five days, each with a weekday, condition icon, and high / low temperature.
   - **Hourly Strip (12 hours)** shows a horizontal row of the next twelve hours, each with the hour, condition icon, and temperature.
4. For temperature and forecast displays, optionally enter a **Label** to override the default card label.
5. On a **Large** card, turn on **Large Temperature Numbers** if you want the high / low reading scaled much larger. (Not available for the strip modes.)
6. The strip modes need extra room on the grid:
   - **Daily Strip** needs a **wide** slot — add `w` to the card in Grid Order (for example `3w`).
   - **Hourly Strip** needs an **extra-wide** slot — add `x` to the card in Grid Order (for example `3x`).
   The card settings show a reminder if the slot is too small; if a strip card ends up in a slot that is too small, it falls back to a simpler layout (**Daily Strip** shows today's temperatures, **Hourly Strip** shows current conditions).

## How It Works on the Panel

- In **Current Conditions** mode, the card watches the weather entity's current state.
- In **Current Conditions** mode, the icon changes automatically and the label uses the condition name from Home Assistant.
- In **Temperatures Today** and **Temperatures Tomorrow** modes, the card asks Home Assistant for the daily forecast for the configured weather entity.
- In temperature modes, the unit label comes from the panel's **Temperature Unit** setting.
- In temperature modes, the card label defaults to **Today** or **Tomorrow**, unless you set your own label.
- In **Hero** mode, the card watches the current condition and also requests today's forecast, showing the condition icon and label alongside today's high / low.
- In **Daily Strip** and **Hourly Strip** modes, the card requests the daily or hourly forecast and refreshes it periodically; each column shares the panel's **Temperature Unit** setting.
- In strip modes, the temperature values are shown slightly smaller than a single reading so negative temperatures (for example **-5/-12**) still fit within each column.
- If Home Assistant reports `unknown`, `unavailable`, or an unexpected current condition, the card shows a fallback weather icon and a readable label.
- If the requested forecast is missing or unavailable, the card shows **--/--** instead of leaving the card blank.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like Sensor, Date, Clock, and World Clock cards.

::: tip Home Assistant actions permission
The temperature displays need the same **Allow the device to perform Home Assistant actions** setting as control cards. EspControl uses that permission to request forecast data from Home Assistant.
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
