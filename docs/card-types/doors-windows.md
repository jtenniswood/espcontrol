---
title: "Home Assistant Doors & Windows Cards"
description:
  How to show Home Assistant door and window sensors with open and closed icons on your EspControl panel.
---

# Doors & Windows

A Doors & Windows card is a read-only card for contact sensors. It uses the Sensor card style, but picks suitable icons for open and closed states.

## Setting Up a Doors & Windows Card

1. Select a card and change its type to **Doors & Windows**.
2. Choose **Door** or **Window**.
3. Enter the **Sensor Entity** for the contact sensor, for example `binary_sensor.patio_door`.
4. Set a **Label** if you want custom text. If left blank, the panel uses the entity name from Home Assistant when it can.

The default icon pairs are:

| Subtype | Closed icon | Open icon |
|---|---|---|
| Door | Door | Door Open |
| Window | Window Closed | Window Open |

You can change the closed or open icon if your sensor needs a different look.

## How It Works on the Panel

- The card is read-only, so tapping it does nothing.
- The icon changes when Home Assistant reports an active state such as `on`, `open`, `opening`, `closing`, or `unlocked`.
- By default, **Lit When Open** is on, so the card uses the normal active/on colour when open. Turn it off if you only want the icon to change.
- When the sensor is off, closed, unknown, or unavailable, the card returns to the Sensor card colour and closed icon.
