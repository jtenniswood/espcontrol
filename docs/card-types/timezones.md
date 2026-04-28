---
title: Timezone
description:
  How to show the current time for another city on your Espcontrol panel.
---

# Timezone

A timezone card shows the current local time for a selected city. The large value is the time, and the label underneath is the city name.

Timezone cards are read-only — tapping them does nothing.

## Setting Up a Timezone Card

1. Select a card and change its type to **Timezone**.
2. Choose the city/timezone from the **Timezone** dropdown.
3. Apply the configuration so the panel restarts with the new card.

## How It Works on the Panel

- The dropdown uses the same timezone list as the main [Clock](/features/clock) setting.
- The card follows the panel's 12-hour or 24-hour clock format.
- The time updates once per minute from the panel's own synced time source.
- The card uses the **tertiary** colour from [Appearance](/features/appearance), like Sensor, Date, and Weather cards.
- If the panel has not synced time yet, the card shows `--:--` until time becomes available.
