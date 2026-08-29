---
title: Presence Cards
description:
  How to show room, motion, or occupancy sensors on your EspControl panel.
---

# Presence

A Presence card is a read-only card for showing whether someone or something is detected. It is useful for room presence sensors, motion sensors, and template sensors that represent occupancy.

## Setting Up a Presence Card

1. Select a card and change its type to **Presence**.
2. Enter the **Sensor Entity**, for example `binary_sensor.living_room_presence`.
3. Set a **Label** if you want custom text. If left blank, the panel uses the entity name from Home Assistant when it can.
4. Choose the **Clear Icon** and **Detected Icon** if the defaults do not match your sensor.
5. Leave **Lit When Detected** on if you want the card to use the active colour when presence is detected.

## How It Works on the Panel

- The card is read-only, so tapping it does nothing.
- The card uses the clear icon when the entity is off, away, not detected, unknown, or unavailable.
- The card uses the detected icon when Home Assistant reports an active state such as `on`, `detected`, `home`, `open`, or `playing`.
- When **Lit When Detected** is off, only the icon changes; the card does not switch to the active colour.

## Example Presence Sensors

| Entity | What it shows |
|---|---|
| `binary_sensor.living_room_presence` | Room presence from a mmWave or motion sensor |
| `sensor.office_occupancy` | Occupancy reported by a room sensor |
| `text_sensor.guest_room_status` | A text-based occupancy status |
