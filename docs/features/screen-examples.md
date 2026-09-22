---
title: Example Screen Layouts
description:
  Living-room, hallway, and bedside layout ideas for EspControl, with illustrated examples and links to the card settings.
---

# Example Screen Layouts

Use these examples as a starting point for arranging your own panel. Each combines existing card types in a grid supported by the display shown.

These are **rendered interface examples** using the real card components, bundled fonts and icons, and spacing and font sizes read from each device’s firmware configuration. Names and readings are sample data. They are browser renders rather than device screenshots, so text rasterisation and some alignment may differ on hardware. Build the layouts in [Setup](/features/setup); they are not downloadable configurations.

## Living Room: Controls and Readings

A landscape [7-inch panel](/screens/jc1060p470) has a five-column, three-row grid. This layout uses 14 cards, with the now-playing card taking two slots.

![Illustrative living-room layout: an orange active light, kitchen light, idle thermostat, wide now-playing card, blinds, lock, scene, weather, sensors, volume, lights subpage, and date.](/images/examples/living-room-render.png)

| Cards in the example | How to create them |
|---|---|
| Living Room and Kitchen | [Lights](/card-types/lights), set to Switch. The orange tile represents a light that is on. |
| Idle | [Climate](/card-types/climate), with Target as the main temperature and Status as the label. |
| Evening Mix and Volume | [Media](/card-types/media), using Now Playing on a Wide card and Volume on a separate Single card (showing an example level of 42). The smaller now-playing line is example artist text. |
| Blinds, Front Door, and Movie | [Cover](/card-types/covers), [Lock](/card-types/locks), and [Action](/card-types/actions) set to Scene. |
| Today | [Weather](/card-types/weather), set to Temperatures Today. |
| Room, Humidity, and Power | Numeric [Sensor](/card-types/sensors) cards with °C, %, and W units. |
| Lights and September | A [Subpage](/features/subpages) for more lights, and a [Date & Time](/card-types/calendar) card set to Date. |

## Hallway: Everyday Essentials

A portrait [4.3-inch panel](/screens/jc4880p443) has a two-column, three-row grid. Six Single cards keep common controls and arrival information together.

<img src="/images/examples/hallway-render.png" alt="Illustrative hallway layout with six cards: active Hall Light, Front Door lock, orange open Patio contact, Hall presence, 12°C outside reading, and Away scene." width="360" loading="lazy" />

| Cards in the example | How to create them |
|---|---|
| Hall Light and Front Door | [Lights](/card-types/lights) set to Switch, and a [Lock](/card-types/locks) card. |
| Patio | [Doors & Windows](/card-types/doors-windows) set to Door. Orange represents an open contact with Lit When Open enabled. |
| Hall | A [Presence](/card-types/presence) card for a hallway occupancy sensor. |
| Outside | A numeric [Sensor](/card-types/sensors) card with °C as its unit. |
| Away | An [Action](/card-types/actions) card connected to your own Home Assistant scene or script. |

The Patio and Hall cards only show sensor state. They do not open a door or control occupancy. Configure what the Away action does in Home Assistant.

## Bedside: Clock and Night Controls

A square [4-inch P4-86 panel](/screens/p4-86) has a three-column, three-row grid. A Wide clock leaves seven Single cards for the remaining controls and readings.

<img src="/images/examples/bedside-render.png" alt="Illustrative bedside layout with a wide 22:15 clock, Lamp, Blinds, Goodnight scene, 19.5°C room sensor, September date, Volume, and Lights subpage." width="560" loading="lazy" />

| Cards in the example | How to create them |
|---|---|
| 22:15 and September | [Date & Time](/card-types/calendar): a Wide Clock with Large Clock enabled, plus a separate Single Date card. |
| Lamp and Blinds | [Lights](/card-types/lights) set to Switch, and a [Cover](/card-types/covers) card. |
| Goodnight | An [Action](/card-types/actions) card for your own bedtime scene or script. |
| Room | A numeric [Sensor](/card-types/sensors) card with °C as its unit. |
| Volume and Lights | [Media](/card-types/media) set to Volume, and a [Subpage](/features/subpages) containing additional lights. |

## Build Your Own

Open the panel's [Setup page](/features/setup), choose the card types above, and use your own Home Assistant entities. Right-click a card to change its [size](/features/setup#card-sizes), then drag cards into place.

The examples use an orange [primary colour](/features/appearance). The [clock bar](/features/clock-bar) shows one configured temperature, the local time, and connectivity. Choose **Apply Configuration** after editing your cards.
