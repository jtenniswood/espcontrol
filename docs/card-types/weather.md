---
title: "Home Assistant Weather Cards"
description:
  How to show current Home Assistant weather conditions or daily high / low temperatures on your EspControl panel.
---

# Weather

A weather card displays weather information from a Home Assistant weather entity. It can show either the current condition, such as **Sunny**, **Cloudy**, or **Rainy**, or the high / low temperatures for today or tomorrow, such as **18/10°C**.

Weather cards display information without changing anything in Home Assistant. In **Daily Forecast** mode, tap the card to open forecast details. The other weather modes do nothing when tapped.

![Weather card showing today's high and low temperatures](/images/card-weather.png)

## Setting Up a Weather Card

1. Select a card and change its type to **Weather**.
2. Enter a **Weather Entity** — the Home Assistant weather entity ID you want to display, for example `weather.forecast_home`.
3. Choose **Type**:
   - **Current Conditions** shows the live weather condition icon and label.
   - **Temperatures Today** shows today's high / low temperature.
   - **Temperatures Tomorrow** shows tomorrow's high / low temperature.
   - **Daily Forecast** shows the upcoming forecast, adding more days as the card gets wider.
4. For Today and Tomorrow temperatures, optionally enter a **Label** to override the default card label.
5. For Today and Tomorrow temperatures, on a **Large** card, turn on **Large Temperature Numbers** if you want the high / low reading scaled much larger.

Today and Tomorrow temperature displays are available on all supported panels, including the 4-inch Guition 4848S040 (ESP32-S3). After updating an S3 panel that previously only offered Current Conditions, select the temperature type again and save the card.

## How It Works on the Panel

- In **Current Conditions** mode, the card watches the weather entity's current state.
- In **Current Conditions** mode, the icon uses the normalized weather condition, while the label keeps provider-specific text from Home Assistant (for example, `light rain` is shown as **Light Rain** rather than being reduced to **Rainy**).
- In **Temperatures Today** and **Temperatures Tomorrow** modes, the card asks Home Assistant for the daily forecast for the configured weather entity.
- In temperature modes, the unit label comes from the panel's **Temperature Unit** setting.
- In temperature modes, the card label defaults to **Today** or **Tomorrow**, unless you set your own label.
- If Home Assistant reports `unknown`, `unavailable`, or an unexpected current condition, the card shows a fallback weather icon and a readable label.
- If the requested forecast is missing or unavailable, the card shows **--/--** instead of leaving the card blank.
- The card uses the fixed **tertiary** background colour, like Sensor, Date, Clock, and World Clock cards.

::: tip Home Assistant actions permission
The temperature displays need the same **Allow the device to perform Home Assistant actions** setting as control cards. EspControl uses that permission to request forecast data from Home Assistant.
:::

## Temperatures Tomorrow

To show tomorrow's forecast high and low, create a **Weather** card and set **Type** to **Temperatures Tomorrow**. Enable [Home Assistant actions](/getting-started/home-assistant-actions) so the panel can request the forecast.

Older configurations containing a separate **Weather Forecast** card load as a Weather card with this setting. You do not need to recreate them.

## Daily Forecast

Choose **Daily Forecast** to show up to seven upcoming days from your weather entity. Each column is left-aligned, with its condition icon at the top and **Today** or the weekday plus the high temperature at the bottom. Temperatures are rounded to whole degrees (for example, **18°**). Wider columns leave space between days, and widening the card adds more days when they fit. Daily cards have no separate label or low-temperature row. The editor shows sample data; the panel loads the actual forecast from Home Assistant.

Tap the card to open a details modal. Use the previous and next buttons to browse all available days, including days that do not fit on the card. The modal shows the condition, high / low temperatures, and any rain chance, rainfall amount, wind speed, and humidity supplied by your provider. Missing details are omitted. Rainfall and wind use the units supplied by Home Assistant; temperatures use the panel's Temperature Unit setting.

The weather entity must provide a **daily** forecast, and Home Assistant actions must be enabled. While waiting for its first response, the card shows a single weather icon in the top-left corner, then switches to day columns when data arrives. It uses actual forecast dates, so a provider whose forecast starts tomorrow is not shown as Today. Providers may return fewer than seven days. An empty forecast shows **Forecast unavailable**. After a request fails or Home Assistant disconnects, the panel keeps the last successful forecast and labels it **Last forecast** until it receives fresh data. Forecasts refresh hourly and after reconnecting.

To keep memory use bounded, daily cards share one compact forecast per weather entity. The panel supports up to eight different daily forecast entities and sixteen daily cards across the home screen and subpages. Widgets are created only for the days that fit on each card; the modal holds one day's detail view at a time. A configuration beyond those limits shows an unavailable forecast instead of allocating more memory.

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
