---
title: Weather
description:
  How to show the current Home Assistant weather condition on your Espcontrol panel.
---

# Weather

A weather card displays the current condition from a Home Assistant weather entity. It shows a weather icon and a label such as **Sunny**, **Cloudy**, or **Rainy**.

Weather cards are read-only — tapping them does nothing.

## Setting Up a Weather Card

1. Select a card and change its type to **Weather**.
2. Enter a **Weather Entity** — the Home Assistant weather entity ID you want to display, for example `weather.forecast_home`.

## How It Works on the Panel

- The card watches the weather entity's current state.
- The icon changes automatically when the weather condition changes.
- The label uses the condition name from Home Assistant.
- If Home Assistant reports `unknown`, `unavailable`, or an unexpected condition, the card shows a fallback weather icon and a readable label.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like sensor cards.

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
