---
title: "Flightradar24 Flights Cards"
description:
  How to show aircraft flying near your home on your EspControl panel using the Flightradar24 integration for Home Assistant.
---

# Flights

A Flights card shows aircraft currently in your monitored area, or aircraft that just entered or exited it. The tile shows the count and the card title, with the Home Assistant sensor name as the default. Tap the card for the flight list.

The data comes from the [Flightradar24 integration](https://github.com/AlexandrErohin/home-assistant-flightradar24) for Home Assistant, through a small template sensor that you add once.

## Before You Start

1. Install the **Flightradar24** integration from HACS and set up the area you want to watch. It creates a sensor called `sensor.flightradar24_current_in_area`.
2. Add the template sensor below to Home Assistant.

### Why a Template Sensor Is Needed

The integration stores every flight in the area with dozens of details and a flight-path history. With a busy sky, that is far more data than the panel can receive in one update. The template keeps only the nearest 10 flights and the details the card shows, so the card keeps working however busy it gets.

### Add the Template Sensor

Paste this into your Home Assistant `configuration.yaml`. If you already have a `template:` section, add the `- sensor:` entry under it instead of adding a second `template:` line. Then restart Home Assistant, or reload **Template entities** from **Developer tools → YAML**.

```yaml
template:
  - sensor:
      - name: EspControl Flights
        unique_id: espcontrol_flights
        icon: mdi:airplane
        availability: "{{ has_value('sensor.flightradar24_current_in_area') }}"
        state: "{{ states('sensor.flightradar24_current_in_area') | int(0) }}"
        attributes:
          flights: >-
            {%- set ns = namespace(rows=[]) -%}
            {%- set source = state_attr('sensor.flightradar24_current_in_area', 'flights') or [] -%}
            {%- for f in source | selectattr('distance', 'number') | sort(attribute='distance') -%}
              {%- if loop.index <= 10 -%}
                {%- set fields = [
                  f.flight_number or f.callsign,
                  f.airline_short or f.airline,
                  f.aircraft_model or f.aircraft_code,
                  f.airport_origin_city or f.airport_origin_code_iata,
                  f.airport_destination_city or f.airport_destination_code_iata,
                  f.distance | round(1),
                  f.altitude,
                  f.ground_speed,
                  f.vertical_speed,
                  1 if f.on_ground else 0
                ] -%}
                {%- set ns.rows = ns.rows + [fields | map('string') | map('replace', '|', '/') | map('replace', ';', ',') | join('|')] -%}
              {%- endif -%}
            {%- endfor -%}
            {{ ns.rows | join(';') }}
```

This creates `sensor.espcontrol_flights` for **Current in area**. Its state is the number of flights and its `flights` attribute is the short list the card reads.

For **Entered area** or **Exited area**, duplicate the sensor entry under the same `sensor:` list. Set the name and unique ID from the table, and replace each occurrence of `sensor.flightradar24_current_in_area` with the matching source. Use the actual entity ID from your Home Assistant installation if it differs.

| Area view | Flightradar24 source | Template sensor name | Unique ID |
| --- | --- | --- | --- |
| Current in area | `sensor.flightradar24_current_in_area` | EspControl Flights | `espcontrol_flights` |
| Entered area | `sensor.flightradar24_entered_area` | EspControl Flights Entered | `espcontrol_flights_entered` |
| Exited area | `sensor.flightradar24_exited_area` | EspControl Flights Exited | `espcontrol_flights_exited` |

## Setting Up a Flights Card

1. Add a new card and select **Flights**.
2. Choose **Area view**. Existing cards use **Current in area**.
3. Set **Flights Sensor** to the matching template entity. The editor suggests `sensor.espcontrol_flights`, `sensor.espcontrol_flights_entered`, or `sensor.espcontrol_flights_exited` when you switch views.
4. Set a **Title** (optional). It appears below the count on the tile and above the flight list. If left blank, the sensor’s name from Home Assistant is used.
5. Choose **Units**:
   - **Metric** shows distance in km, altitude in metres, and speed in km/h.
   - **Imperial** shows distance in miles, altitude in feet, and speed in mph.

## How It Works on the Panel

- The card shows the count for the selected view, with the title beneath it.
- The card keeps its sensor background as the flight count changes.
- Tap the card to open the flight list. It shows up to 10 flights, closest first, and updates while it is open:
  - flight number and aircraft type
  - airline and route
  - distance, altitude, speed, and whether the aircraft is climbing, descending, level, or on the ground

Entered and exited counts show the flights reported on the latest integration refresh. They do not accumulate over time.

## Opening the List from Home Assistant

The flight list works with the panel's [`open_modal` action](/features/subpages), so an automation can show it at the right moment. For example, open the list on a panel named `hall_panel` whenever a plane enters your area:

```yaml
triggers:
  - trigger: event
    event_type: flightradar24_entry
actions:
  - action: esphome.hall_panel_open_modal
    data:
      entity_id: sensor.espcontrol_flights
```
