---
title: Timer Cards
description:
  How to use Timer cards on your EspControl panel with Home Assistant timer helpers.
---

# Timer

A Timer card controls an existing Home Assistant `timer` helper. It shows a live countdown on the card and opens a full-screen timer control when tapped.

EspControl does not create the timer helper for you. Create the helper in Home Assistant first, then select that `timer.*` entity in the EspControl setup page.

## Setting Up a Timer Card

1. In Home Assistant, create a Timer helper.
2. In EspControl, select a card and change its type to **Timer**.
3. Enter the **Timer Entity**, for example `timer.kitchen`.
4. Set **Default Hours** and **Default Minutes**. This is the duration shown when the timer is idle.
5. Set a label or icon if you want to override the defaults.

## Panel Controls

Tapping the card opens the Timer control.

- **Set Timer** starts or restarts the Home Assistant timer with the selected duration.
- **Pause** appears while the timer is active.
- **Resume** appears while the timer is paused.
- **Cancel** and **Finish** appear while the timer is active or paused.

The card counts down locally while the timer is active, so the visible time keeps moving between Home Assistant state updates.

::: info Requires Home Assistant actions
Timer cards send Home Assistant timer actions from the panel. If controls do not respond, check [Enable Actions](/getting-started/home-assistant-actions).
:::
