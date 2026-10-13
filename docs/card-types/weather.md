---
title: "Home Assistant Weather Cards"
description:
  How to show current Home Assistant weather conditions, today or tomorrow forecasts, and multi-day forecasts on your EspControl panel.
---

# Weather

A weather card shows current Home Assistant conditions or a daily forecast. Forecast cards use a left-aligned layout with the condition icon at the top, and the day and high/low temperatures at the bottom. Temperatures share one line and are rounded to whole degrees, such as **18°/2°**, and use the panel's **Temperature Unit** setting. Forecast cards have no separate label or low-temperature row.

## Setting Up a Weather Card

1. Select a card and change its type to **Weather**.
2. Enter a **Weather Entity**, for example `weather.forecast_home`.
3. Choose **Type**:
   - **Current Conditions** shows the live weather condition icon and label.
   - **Today's Forecast** shows today's condition and high/low temperatures.
   - **Tomorrow's Forecast** shows tomorrow's condition and high/low temperatures.
   - **Daily Forecast** shows up to seven upcoming days, adding more days as the card gets wider.

All forecast modes are available on supported panels, including the 4-inch Guition 4848S040 (ESP32-S3). The editor shows sample data; the panel loads the actual forecast from Home Assistant.

Existing **Temperatures Today** and **Temperatures Tomorrow** cards automatically use the new forecast layout. Their saved day selection is retained. The old custom label and large temperature number controls no longer apply. Older configurations containing a separate **Weather Forecast** card load as **Tomorrow's Forecast**.

::: tip Home Assistant actions permission
Forecasts need **Allow the device to perform Home Assistant actions** enabled so the panel can request forecast data. See [Home Assistant actions](/getting-started/home-assistant-actions).
:::

## How It Works on the Panel

**Current Conditions** watches the weather entity's current state. The icon uses the normalized weather condition, while the label keeps provider-specific text, such as **Light Rain**. Unknown or unavailable states show a fallback icon and readable label.

Forecast modes request the daily forecast from Home Assistant. **Today's Forecast** and **Tomorrow's Forecast** each show exactly one day, even on a wide card. They select the requested local date; if the provider omits that date, the card shows **Forecast unavailable**. **Daily Forecast** shows **Today** or short weekday names in columns with padding and a gap between days. A minimum column width keeps the icon, weekday and temperature readable. As the card widens, it adds more days, up to seven, and shares the remaining width evenly between the visible columns. Once all available days fit, the columns continue stretching to fill the card.

Forecasts refresh hourly and after reconnecting. Providers must supply a daily forecast and may return fewer than seven days.

While loading, forecast cards show a single cloud-and-clock icon and **Loading forecast**. The day layout appears when data arrives. A failed refresh keeps the last available forecast and marks it **Last forecast**; if no forecast is available, the card shows **Forecast unavailable**.

Weather cards use the fixed **tertiary** background colour, like Sensor, Date, Clock, and World Clock cards.

All forecast modes share one compact forecast per weather entity. The panel supports up to eight forecast entities and sixteen forecast cards across the home screen and subpages. It creates widgets only for visible days, and the modal holds one day's detail view at a time. Configurations beyond these limits show an unavailable forecast.

## Forecast Details

Tap any forecast card to open a **3×2 grid** of up to six forecast days. Each day panel includes its condition icon, day, whole-degree high/low temperatures in smaller grey text, rain chance and wind speed. Wind uses the provider's units, and rainfall is shown when the provider gives an amount without a rain percentage. Missing values are omitted and zero values remain visible. Short displays abbreviate labels where needed to keep values and units readable; short landscape panels place the icon beside the day and temperatures.

Today and Tomorrow cards start with their selected day highlighted. Tap a day to update a compact footer showing its rainfall amount and humidity. The **Last forecast** notice shares that footer when data is stale. The layout fits supported displays in both orientations without scrolling or paging controls and has no separate Weather heading.

The modal allocates only available day panels, up to six, with one reusable two-line rain/wind label per day. It shares the card's bounded forecast snapshot and uses existing fonts.

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
