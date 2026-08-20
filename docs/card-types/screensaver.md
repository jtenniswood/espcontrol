---
title: Screensaver Cards
description:
  How to add a local card that immediately starts the configured screensaver.
---

# Screensaver

A Screensaver card immediately starts the panel's configured screensaver action. It works locally on the panel, needs no Home Assistant entity, and can be placed on the home screen or a subpage.

## Setting Up a Screensaver Card

1. Select a card and change its type to **Screensaver**.
2. Optionally change the label, for example to **Screen Off**.
3. Optionally choose a different icon.
4. Apply the configuration.

Tapping the card follows the action selected in **Setup → Settings → Screensaver**: **Display Off**, **Screen Dimmed**, or **Clock**.

If **Require PIN after wake** is enabled and a PIN has been set, the shared scrambled keypad appears when the panel wakes. See [Screensaver](/features/screensaver) for setup and security details.

This card is different from [Screen Lock](/card-types/screen-lock). Screensaver starts the configured sleep display; Screen Lock keeps the current display visible while blocking accidental card presses.
