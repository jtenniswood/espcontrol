---
title: Screen Lock Cards
description:
  How to use screen lock cards on your EspControl panel to start the configured screensaver.
---

# Screen Lock

A Screen Lock card sends the panel to its configured screensaver action immediately. It does not need a Home Assistant entity and does not send a Home Assistant action.

Use this when you want a local card that turns the screen off, dims it, or shows the clock using the same **Screensaver** setting as the automatic idle timer.

## Setting Up a Screen Lock Card

1. Select a card and change its type to **Screen Lock**.
2. Set a **Label** if you want custom text. If left blank, the card uses the built-in lock label.
3. Choose the **Locked Icon** and **Unlocked Icon** if you want different icons from the defaults.
4. Apply the configuration.

## How It Works on the Panel

- Tapping the card immediately starts the configured screensaver action: Display Off, Screen Dimmed, or Clock.
- If **Require PIN after wake** is enabled and a PIN is set in Screensaver settings, the panel asks for that PIN after waking.
- Other cards stay blocked until the correct PIN is entered.
- The card can be used on the home screen or inside a subpage.
- It does not depend on Home Assistant availability.

Screen Lock is different from a [Lock](/card-types/locks) card. **Lock** controls a Home Assistant `lock` entity such as a door lock. **Screen Lock** controls the touchscreen's local interaction state.

## When to Use It

Screen Lock is useful for hallway panels, bedside panels, child-accessible panels, or any location where you want a quick local sleep button.

For security-sensitive actions such as unlocking a door, use the Home Assistant lock's own security features, a Home Assistant script, or card-level confirmation where available. Screensaver PIN protection is a local touchscreen guard, not a replacement for Home Assistant permissions or web/admin security.
