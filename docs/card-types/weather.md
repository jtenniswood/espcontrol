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
   - **Daily Forecast** shows up to eight upcoming days, reflowing into a row, grid or vertical list as the card changes size.

All forecast modes are available on supported panels, including the 4-inch Guition 4848S040 (ESP32-S3). The editor shows sample data; the panel loads the actual forecast from Home Assistant.

Existing **Temperatures Today** and **Temperatures Tomorrow** cards automatically use the new forecast layout. Their saved day selection is retained. The old custom label and large temperature number controls no longer apply. Older configurations containing a separate **Weather Forecast** card load as **Tomorrow's Forecast**.

::: tip Home Assistant actions permission
Forecasts need **Allow the device to perform Home Assistant actions** enabled so the panel can request forecast data. See [Home Assistant actions](/getting-started/home-assistant-actions).
:::

## How It Works on the Panel

**Current Conditions** watches the weather entity's current state. The icon uses the normalized weather condition, while the label keeps provider-specific text, such as **Light Rain**. Unknown or unavailable states show a fallback icon and readable label.

Forecast modes request the daily forecast from Home Assistant. **Today's Forecast** and **Tomorrow's Forecast** each show exactly one day, even on a wide card. They select the requested local date; if the provider omits that date, the card shows **Forecast unavailable**. **Daily Forecast** shows **Today** or short weekday names with whole-degree high/low temperatures. Its layout uses both card width and height: single-height cards use a horizontal row, larger cards use a grid, and tall narrow cards use a vertical list with icons beside the day and temperatures when there is enough width. Fonts stay readable rather than shrinking to fit more days. The grid balances its rows and columns when a provider returns fewer days. Short cards use compact rows when needed to leave room for **Last forecast**.

On the 480×480 S3 with the default font sizes and eight available days:

| Card size | Layout | Days shown |
|---|---|---|
| 1w × 1h | One day | 1 |
| 2w × 1h | Horizontal row | 2 |
| 3w × 1h | Horizontal row | 4 |
| 1w × 2h | Vertical list | 4 |
| 2w × 2h | 2×2 grid | 4 |
| 3w × 2h | 4×2 grid | 8 |
| 1w × 3h | Vertical list | 6 |
| 2w × 3h | 2×3 grid | 6 |
| 3w × 3h | 3×3 grid, one space unused | 8 |

The exact count depends on the panel's fonts, available forecast days, translated labels and stale-data status. The editor preview uses the same layout rules and updates when the card is resized.

Forecasts refresh hourly and after reconnecting. Providers must supply a daily forecast and may return fewer than eight days.

While loading, forecast cards show a single cloud-and-clock icon and **Loading forecast**. The day layout appears when data arrives. A failed refresh keeps the last available forecast and marks it **Last forecast**; if no forecast is available, the card shows **Forecast unavailable**.

Weather cards use the fixed **tertiary** background colour, like Sensor, Date, Clock, and World Clock cards.

All forecast modes share one compact eight-day forecast per weather entity. The panel supports up to eight forecast entities and sixteen forecast cards across the home screen and subpages. It creates widgets only for visible days, and the modal shows up to six day panels. Configurations beyond these limits show an unavailable forecast.

## Forecast Details

Tap any forecast card to open a **3×2 grid** of up to six forecast days. Each day panel shows its day and weather icon in white, with whole-degree high/low temperatures, wind speed and rain data in grey. Lines use equal spacing, with wind above the rain chance and rainfall row, such as **80% / 2.4 mm**. Rainfall and wind keep the provider's units. Missing values are omitted and zero values remain visible. Short displays abbreviate labels where needed to keep values and units readable; short landscape panels place the icon beside the day and temperatures.

Day panels have no selection highlight or tap action, and there is no separate rain/humidity footer. The grey **Last forecast** notice appears below the grid when data is stale. The layout fits supported displays in both orientations without scrolling or paging controls and has no separate Weather heading.

The modal allocates only available day panels, up to six, with one reusable two-line wind/rain label per day. It shares the card's bounded forecast snapshot and uses existing fonts.

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
