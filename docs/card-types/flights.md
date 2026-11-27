---
title: "Flightradar24 Flights Cards"
description:
  How to show aircraft flying near your home on your EspControl panel using the Flightradar24 integration for Home Assistant.
---

# Flights

A Flights card shows the number of aircraft currently in your monitored area, or the number that just entered or exited it. The card uses a sensor from the [Flightradar24 integration](https://github.com/AlexandrErohin/home-assistant-flightradar24) for Home Assistant.

## Setting Up a Flights Card

1. Install the **Flightradar24** integration from HACS and set up the area you want to watch.
2. Add a **Flights** card and choose **Area view**.
3. Set **Flights Sensor** to the corresponding Home Assistant entity. Use the actual entity ID shown in Home Assistant if it differs from the suggestion.
4. **Title** starts with the selected view name in the panel’s language. Enter your own title or clear and save it to use the sensor’s name from Home Assistant.
5. Choose an **Icon**, or leave it on **Auto** to use the icon for the selected view.

| Area view | Flightradar24 sensor |
| --- | --- |
| Current in area | `sensor.flightradar24_current_in_area` |
| Entered area | `sensor.flightradar24_entered_area` |
| Exited area | `sensor.flightradar24_exited_area` |

Entered and exited counts show flights reported on the latest integration refresh. Existing cards using a Home Assistant template sensor can keep using it as their count source.
