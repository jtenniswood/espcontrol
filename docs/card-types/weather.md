---
title: "Home Assistant Weather Cards"
description:
  How to show current Home Assistant weather conditions, today or tomorrow forecasts, and multi-day forecasts on your EspControl panel.
---

# Weather

![Illustrative ESP32-P4-86 device showing current conditions, today, tomorrow and an eight-day weather forecast](/images/weather-device-mockup.webp)

*Illustrative screen with sample data and custom card colours: current conditions, today and tomorrow above an eight-day forecast.*

Show current conditions or daily forecasts from a Home Assistant weather entity. Tap a forecast card for wind and rain details.

## Setting Up a Weather Card

1. Select a card and change its type to **Weather**.
2. Enter a **Weather Entity**, for example `weather.forecast_home`.
3. Choose **Type**:
   - **Current Conditions** — the live condition icon and label.
   - **Today's Forecast** — today's icon and high/low temperatures.
   - **Tomorrow's Forecast** — tomorrow's icon and high/low temperatures.
   - **Daily Forecast** — up to eight days in a row or grid.
4. Resize the card in the preview, optionally choose **Custom Colours**, then press **Save**.

::: tip Home Assistant actions permission
Enable **Allow the device to perform Home Assistant actions** so the panel can request forecasts. See [Enable Actions](/getting-started/home-assistant-actions).
:::

### Configuration Examples

The admin webserver examples below use `weather.forecast_home`; replace it with your own entity. The preview uses sample data. Your panel loads real conditions and forecasts from Home Assistant.

<div class="weather-configuration-grid">
  <figure>
    <img src="/images/weather-admin-current.png" alt="Weather editor with Type set to Current Conditions and Weather Entity set to weather.forecast_home" loading="lazy" width="844" height="708">
    <figcaption><strong>Current Conditions</strong></figcaption>
  </figure>
  <figure>
    <img src="/images/weather-admin-today.png" alt="Weather editor with Type set to Today's Forecast and Weather Entity set to weather.forecast_home" loading="lazy" width="844" height="708">
    <figcaption><strong>Today's Forecast</strong></figcaption>
  </figure>
  <figure>
    <img src="/images/weather-admin-tomorrow.png" alt="Weather editor with Type set to Tomorrow's Forecast and Weather Entity set to weather.forecast_home" loading="lazy" width="844" height="708">
    <figcaption><strong>Tomorrow's Forecast</strong></figcaption>
  </figure>
  <figure>
    <img src="/images/weather-admin-daily.png" alt="Weather editor with Type set to Daily Forecast and Weather Entity set to weather.forecast_home" loading="lazy" width="844" height="708">
    <figcaption><strong>Daily Forecast</strong></figcaption>
  </figure>
</div>

<style>
.weather-configuration-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 24px;
  margin: 24px 0;
}
.weather-configuration-grid figure { margin: 0; min-width: 0; }
.weather-configuration-grid img { width: 100%; height: auto; border-radius: 12px; }
.weather-configuration-grid figcaption { margin-top: 10px; font-size: 14px; line-height: 1.6; }
@media (max-width: 640px) {
  .weather-configuration-grid { grid-template-columns: 1fr; }
}
</style>

## How It Works on the Panel

Forecasts show whole-degree high/low temperatures, such as **18°/2°**, using the panel's **Temperature Unit** setting. Today and Tomorrow always show one day. Daily Forecast adapts to the card's size: a row on wide cards, a grid on larger cards, or a column on tall narrow cards. Enlarge the card to show more days; the count also depends on your provider's data and font sizes.

Forecasts refresh hourly and after reconnecting. **Loading** appears while fetching data. If a refresh fails, the previous forecast remains with **Last forecast**; if no data is available for the requested date, the card shows **Forecast unavailable**. Your Home Assistant provider must support daily forecasts.

All forecast modes work on supported panels, including the 4-inch ESP32-S3. The panel supports up to eight weather entities and sixteen forecast cards across the home screen and subpages.

## Forecast Details

Tap any forecast card to open up to six days in a **3×2 grid**, showing conditions, high/low temperatures, wind speed, rain chance and rainfall where available. Wind and rainfall retain the provider's units. **Last forecast** marks stale data. Current Conditions cards do not open forecast details.

## Supported Conditions

Standard Home Assistant weather states use matching icons. Current Conditions keeps provider-specific labels, such as **Light Rain**; unknown or unavailable states show a fallback icon and readable label.

::: details Older configurations
**Temperatures Today** and **Temperatures Tomorrow** cards retain their day selection and use the current forecast layout. A separate **Weather Forecast** card loads as **Tomorrow's Forecast**. The old custom label and large temperature number controls no longer apply.
:::
