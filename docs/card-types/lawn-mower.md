---
title: Lawn Mower Cards
description:
  How to show or control a Home Assistant lawn mower entity from your EspControl panel.
---

# Lawn Mower

Lawn Mower cards are built for robotic lawn mower controls that make sense as simple touchscreen buttons.

Use a Lawn Mower card when you want to show the mower state, start mowing, send it back to the dock, or pause and resume mowing.

## Setting Up a Lawn Mower Card

1. Select a card and change its type to **Lawn Mower**.
2. Choose a **Type**.
3. Enter the **Lawn Mower Entity**, such as `lawn_mower.backyard`.
4. Set a **Label** if you want custom text on the card.
5. Choose an **Icon**.

## Lawn Mower Types

| Type | What it does |
|---|---|
| **Status** | Shows the mower state, such as mowing, docked, paused, returning, error, unavailable, or unknown. |
| **All Controls** | Opens a modal with Start / Resume, Pause, and Dock controls plus the current mower state. |
| **Start Mowing** | Sends `lawn_mower.start_mowing`. |
| **Dock** | Sends `lawn_mower.dock`. |
| **Pause / Resume** | Sends `lawn_mower.pause` while the mower is mowing, and `lawn_mower.start_mowing` otherwise. |

Control cards and modal controls are disabled when the mower entity is unavailable or unknown.

::: info Requires Home Assistant actions
Lawn Mower control cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
