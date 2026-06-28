---
title: Vacuum Cards
description:
  How to show or control a Home Assistant vacuum entity from your EspControl panel.
---

# Vacuum

Vacuum cards are built for robot vacuum controls that make sense as simple touchscreen buttons.

Use a Vacuum card when you want to show the vacuum state, start or stop cleaning, send it back to the dock, pause or resume, spot clean, locate the vacuum, or clean one mapped Home Assistant area.

## Setting Up a Vacuum Card

1. Select a card and change its type to **Vacuum**.
2. Choose a **Type**.
3. Enter the **Vacuum Entity**, such as `vacuum.kitchen`.
4. Set a **Label** if you want custom text on the card.
5. Choose an **Icon**.

For **Clean Area**, also enter the Home Assistant **Area ID**, such as `kitchen`.

## Vacuum Types

| Type | What it does |
|---|---|
| **Status** | Shows the vacuum state, such as cleaning, docked, paused, returning, or error. |
| **Start / Stop** | Starts cleaning when the vacuum is not cleaning, and stops when it is cleaning. |
| **Dock** | Sends the vacuum back to its base. |
| **Pause / Resume** | Pauses while cleaning and resumes when paused. |
| **Spot Clean** | Starts spot cleaning at the vacuum's current location. |
| **Locate** | Asks the vacuum to play a sound or otherwise identify itself. |
| **Clean Area** | Cleans one mapped Home Assistant area. |

## Clean Area

Home Assistant must already know how your vacuum's map areas connect to Home Assistant areas. Configure that in Home Assistant first, then use the area ID on the card.

If you want separate buttons for different rooms, create one Vacuum **Clean Area** card per room.

::: info Requires Home Assistant actions
Vacuum control cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
