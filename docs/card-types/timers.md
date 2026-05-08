---
title: Timer Cards
description:
  How to use timer cards on your EspControl panel to start, cancel, and resume Home Assistant timer entities.
---

# Timer

A timer card displays a Home Assistant `timer.*` entity as a live countdown. Tap to start the timer, tap again while running to cancel it, and tap a paused timer to resume.

## Setting Up a Timer Card

1. Select a card and change its type to **Timer**.
2. Enter a **Timer Entity** — the Home Assistant timer entity you want to control (for example, `timer.kitchen`).
3. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
4. Optionally enable **Confirm before cancel** to require a second tap before cancelling a running timer.
5. If confirmation is enabled, set the **Confirm Timeout** in seconds. After this delay the prompt disappears and the timer keeps running.

## How It Works on the Panel

- The card shows the time remaining as a large number, with the label below.
- Format auto-switches between `M:SS` (under one hour) and `H:MM` (one hour or more — seconds are dropped so the value fits the card).
- When the timer is **idle**, the card displays its configured `duration` — the value it will start at on the next tap.
- When **active**, the card counts down once per second to `0:00`.
- When **paused**, the card freezes at the remaining value.

## Tap Behaviour

| State | Tap |
| --- | --- |
| Idle | Calls `timer.start` |
| Active | Calls `timer.cancel` (with confirmation if enabled) |
| Paused | Calls `timer.start` (resumes from the remaining value) |

## Confirmation

When **Confirm before cancel** is enabled, tapping a running timer replaces the card's label with `Confirm?` and arms a one-shot timeout. Tap a second time within the timeout window to cancel the timer; otherwise the prompt disappears and the timer keeps running.

The timer's `duration` attribute is read from Home Assistant — there is no per-card duration setting. To change how long the timer runs, edit the timer entity in Home Assistant.
