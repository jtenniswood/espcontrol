---
title: Timer Cards
description:
  How to use timer cards on your EspControl panel to start, cancel, and resume Home Assistant timer entities.
---

# Timer

A timer card displays a Home Assistant `timer.*` entity as a live countdown. Tap to start the timer, tap again while running to cancel it, and tap a paused timer to resume.

## Setting Up a Timer Card

1. Select a card and change its type to **Timer**.
2. Enter a **Timer Entity**, such as `timer.kitchen`.
3. Set a **Label** if you want custom text at the bottom of the card. If left blank, the timer's friendly name from Home Assistant is used.
4. Optionally enable **Confirm before cancel** to require a second tap before cancelling a running timer.
5. If confirmation is enabled, set the **Confirm Timeout** in seconds.

## How It Works on the Panel

- The card shows the time remaining as a large number, with the label below.
- Format auto-switches between `M:SS` under one hour and `H:MM` for one hour or more.
- When the timer is **idle**, the card displays its configured `duration`.
- When the timer is **active**, the card counts down to `0:00`.
- When the timer is **paused**, the card freezes at the remaining value.

## Tap Behaviour

| State | Tap |
| --- | --- |
| Idle | Calls `timer.start` |
| Active | Calls `timer.cancel` with confirmation if enabled |
| Paused | Calls `timer.start` to resume |

## Confirmation

When **Confirm before cancel** is enabled, tapping a running timer replaces the card's label with `Confirm?` and arms a timeout. Tap a second time before the timeout expires to cancel the timer. Otherwise the prompt disappears and the timer keeps running.

The timer's `duration` attribute is read from Home Assistant. To change how long the timer runs, edit the timer entity in Home Assistant.
