---
title: Screen Lock Cards
description:
  How to use screen lock cards on your EspControl panel to lock and unlock local touchscreen controls, with screensaver or screen-off display, and immediate, slide, or PIN unlock.
---

# Screen Lock

A Screen Lock card locks and unlocks the panel's touchscreen controls locally. Locking protects every other card from taps, so the panel cannot be operated until it is unlocked.

Use this when a panel is in a shared area, a bedroom, or a child's room and you want to prevent accidental — or unwanted — control changes.

## Setting Up a Screen Lock Card

1. Select a card and change its type to **Screen Lock**.
2. Set a **Label** if you want custom text. If left blank, the card uses the built-in lock label.
3. Choose the **Locked Icon** and **Unlocked Icon** if you want different icons from the defaults.
4. Choose **When Locked** — what the panel shows while it is locked:
   - **Screensaver** — the panel drops to its screensaver (clock / dimmed / cover art, per your Screensaver settings).
   - **Screen Off** — the backlight turns off entirely.
5. Choose **Unlock With** — how the panel is unlocked:
   - **Immediate** — a single tap unlocks (the classic behaviour).
   - **Slide** — a slide-to-unlock control appears; drag the handle from the bottom to the top to unlock. Releasing early leaves the panel locked.
   - **PIN** — a numeric keypad appears; the panel unlocks only when the correct PIN is entered.
6. If you chose **PIN**, enter the **PIN** (digits only). Leaving it blank falls back to immediate unlock so you can never lock yourself out.
7. Apply the configuration.

## How It Works on the Panel

- Tapping the card while unlocked locks the panel immediately.
- While locked, every other card is protected from taps.
- Unlocking follows the **Unlock With** method (immediate tap, slide gesture, or PIN).
- The lock state is local to the panel and does not depend on Home Assistant availability.
- The card can be used on the home screen or inside a subpage.

> **Placement:** put the Screen Lock card on the **home screen**. Locking drops
> the panel to the screensaver or screen-off, which returns to the home screen —
> so a lock card placed *only* inside a subpage cannot be navigated back to once
> locked, and could then only be unlocked from Home Assistant.
>
> **Use one Screen Lock card.** If you add more than one, they share a single
> configuration (the last one saved wins), so give them all the same settings.

## Home Assistant

Unlike earlier firmware, the screen lock now also exposes two entities to Home Assistant:

- A **Screen Lock** switch — turn it on/off to lock or unlock the panel remotely.
- A **Screen Locked** binary sensor — reports whether the panel is currently locked.

On-device locking and unlocking (tap, slide, or PIN) keeps these entities in sync, so automations can both read and drive the lock. The local lock still works even when Home Assistant is unavailable.

Screen Lock is different from a [Lock](/card-types/locks) card. **Lock** controls a Home Assistant `lock` entity such as a door lock. **Screen Lock** controls the touchscreen's local interaction state.

## When to Use It

Screen Lock is useful for hallway panels, bedside panels, child-accessible panels, or any location where accidental control changes would be annoying — or where you want a deliberate slide or PIN before the panel can be used again.

The PIN is stored locally in the card configuration and is intended to deter casual or accidental unlocking, not as strong security. For security-sensitive actions such as unlocking a door, use the Home Assistant lock's own security features. Screen Lock is a local interaction guard, not a replacement for Home Assistant permissions.
