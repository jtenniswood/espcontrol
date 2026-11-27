---
title: "Flightradar24 Flights Cards"
description:
  How to show aircraft flying near your home on your EspControl panel using the Flightradar24 integration for Home Assistant.
---

# Flights

A flights card shows how many aircraft are flying near your home and names the closest one. Tap the card to see a map of nearby aircraft. Switch to **List** to see their route, distance, altitude, speed, and whether they are climbing or descending.

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
          bounds: >-
            map:{{ state_attr('sensor.flightradar24_current_in_area', 'bounds') or '' }}
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
                  1 if f.on_ground else 0,
                  f.id | default('', true),
                  f.latitude | default(''),
                  f.longitude | default(''),
                  f.heading | default('')
                ] -%}
                {%- set ns.rows = ns.rows + [fields | map('string') | map('replace', '|', '/') | map('replace', ';', ',') | join('|')] -%}
              {%- endif -%}
            {%- endfor -%}
            {{ ns.rows | join(';') }}
```

This creates `sensor.espcontrol_flights`. Its state is the number of flights in the area, and its `flights` attribute is the short list the card reads. The map uses the integration’s monitored bounds and the position and heading included with each flight.

If you named your Flightradar24 sensor differently, replace every occurrence of `sensor.flightradar24_current_in_area`. If you already made the template for the list-only card, update it and reload **Template entities** to enable the map.

## Setting Up a Flights Card

1. Add a new card and select **Flights**.
2. Enter the **Flights Sensor** — the template sensor from above, normally `sensor.espcontrol_flights`.
3. Set a **Label** (optional). It is shown when there are no flights nearby. If left blank, the sensor's name from Home Assistant is used.
4. Choose **Units**:
   - **Metric** shows distance in km, altitude in metres, and speed in km/h.
   - **Imperial** shows distance in miles, altitude in feet, and speed in mph.

## How It Works on the Panel

- The card shows the number of flights in your area as a large number.
- Below it, the card names the closest flight and where it is going, for example `BA123 › Madrid`. When the sky is clear it shows the label instead.
- The card is highlighted while at least one flight is nearby.
- Tap the card to open a north-up map of the monitored area. It shows up to 10 aircraft, with heading and movement estimated from the last reported direction and speed for up to 30 seconds. Tap a plane for its details.
- Use **List** for the distance-ordered view, and **Map** to return. The list remains available if the template sensor has no map bounds.
- The map uses [Natural Earth coastlines](https://www.naturalearthdata.com/downloads/50m-physical-vectors/50m-coastline/) and works without a map-tile service. Inland areas show the monitored area outline and grid.
- The list updates while open and shows:
  - flight number and aircraft type
  - airline and route
  - distance, altitude, speed, and whether the aircraft is climbing, descending, level, or on the ground

The card updates whenever the Flightradar24 integration refreshes, which is every 20 seconds by default.

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
