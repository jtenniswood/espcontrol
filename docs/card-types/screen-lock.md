---
title: Screen Lock Cards
description:
  How to use screen lock cards on your EspControl panel to lock and unlock local touchscreen controls.
---

# Screen Lock

A Screen Lock card locks and unlocks the panel's touchscreen controls locally. It does not need a Home Assistant entity and does not send a Home Assistant action.

Use this when a panel is in a shared area and you want a quick way to prevent accidental taps.

## Setting Up a Screen Lock Card

1. Select a card and change its type to **Screen Lock**.
2. Set a **Label** if you want custom text. If left blank, the card uses the built-in lock label.
3. Choose the **Locked Icon** and **Unlocked Icon** if you want different icons from the defaults.
4. Apply the configuration.

## How It Works on the Panel

- Tapping the card switches the panel between locked and unlocked states.
- The lock state is local to the panel.
- Other cards are protected while the screen is locked.
- The card can be used on the home screen or inside a subpage.
- It does not depend on Home Assistant availability.

Screen Lock is different from a [Lock](/card-types/locks) card. **Lock** controls a Home Assistant `lock` entity such as a door lock. **Screen Lock** controls the touchscreen's local interaction state.

## When to Use It

Screen Lock is useful for hallway panels, bedside panels, child-accessible panels, or any location where accidental control changes would be annoying.

For security-sensitive actions such as unlocking a door, use the Home Assistant lock's own security features, a Home Assistant script, or card-level confirmation where available. Screen Lock is a local interaction guard, not a replacement for Home Assistant permissions.
