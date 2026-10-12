---
title: "Lock the Touchscreen"
description:
  How to use screen lock cards on your EspControl panel to lock and unlock local touchscreen controls.
---

# Screen Lock

A Screen Lock card locks and unlocks the panel's touchscreen controls locally. It does not need a Home Assistant entity and does not send a Home Assistant action.

Use this when a panel is in a shared area and you want to require a PIN before its controls can be used.

## Setting Up a Screen Lock Card

1. Select a card and change its type to **Screen Lock**.
2. In **Pin Protection**, enter a numeric PIN (1–16 digits) and choose **Save**. If a PIN is already set, the field shows masked dots; you can keep it or enter a replacement.
3. Save the card and apply the configuration. A saved PIN is required before the card can be saved.

The card uses the same PIN and keypad layout preference as **Settings > Sleep & Schedule > Pin Protection**. Changes made in either place apply to both. The card always requires a PIN and shows **PIN**, **Save**, then **Randomize keypad layout** (off by default). Automatic wake protection and clearing the shared PIN are managed in the screensaver settings; clearing the PIN there releases the lock. The card works with automatic wake protection off.

Screen Lock does not need an entity. Its label and icon change automatically to show whether the touchscreen is locked or unlocked. The PIN is stored on the panel, never in the card configuration or backups.

## How It Works on the Panel

- **Screensaver on lock**, in the card's **Card Settings**, is off by default. Tapping the card locks the controls while keeping the current screen visible. The normal return-to-home timeout still applies. Any touch opens the PIN keypad instead of activating a control; that first touch does not enter a digit.
- Turn **Screensaver on lock** on to start the configured screensaver immediately when locking. Wake the panel to show the PIN keypad.
- Enter the shared PIN and press the checkmark to unlock.
- **X** clears entered digits. With no digits entered, it returns to the previous cards or screensaver view while keeping the screen locked.
- A wrong PIN keeps the panel locked. Tapping the card cannot bypass PIN entry.
- If the screensaver starts while manually locked, waking shows the same keypad. A successful PIN entry releases both the screensaver lock and the card lock.
- The lock state is local to the panel.
- Other cards and local controls are protected while the screen is locked; the keypad remains responsive.
- The card can be used on the home screen or inside a subpage.
- It does not depend on Home Assistant availability.

Screen Lock is different from a [Lock](/card-types/locks) card. **Lock** controls a Home Assistant `lock` entity such as a door lock. **Screen Lock** controls the touchscreen's local interaction state.

## When to Use It

Screen Lock is useful for hallway panels, bedside panels, child-accessible panels, or any location where accidental control changes would be annoying.

For security-sensitive actions such as unlocking a door, use the Home Assistant lock's own security features, a Home Assistant script, or card-level confirmation where available. Screen Lock is a local interaction guard, not a replacement for Home Assistant permissions.
