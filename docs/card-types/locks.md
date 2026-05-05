---
title: Lock Cards
description:
  How to use lock cards on your EspControl panel to lock, unlock, and view Home Assistant lock entities.
---

# Lock

A Lock card controls a Home Assistant `lock` entity and shows whether it is locked, unlocked, opening, or in a problem state.

## Setting Up a Lock

1. Select a card and change its type to **Lock**.
2. Enter an **Entity ID** - the Home Assistant lock entity, for example `lock.front_door`.
3. Choose the locked and unlocked icons. These default to **Lock** and **Lock Open**.
4. Set a **Label** if you want custom text. If left blank, the friendly name from Home Assistant is used.

## How It Works on the Panel

- Tapping a locked Lock card sends `lock.unlock` to Home Assistant.
- Tapping an unlocked, open, or jammed Lock card sends `lock.lock`.
- If the panel does not know the current state yet, tapping sends `lock.lock` rather than unlocking.
- The card lights up while the lock is unlocked, unlocking, open, opening, or jammed.
- When the lock state changes, the label temporarily shows the Home Assistant state, such as **Locked**, **Unlocked**, **Unlocking**, **Open**, or **Jammed**.

## Opening or Unlatching

Some locks support Home Assistant's optional `lock.open` action, usually to unlatch a door. Use an [Action](/card-types/actions) card with **Open Lock** when you need that command.

## Locks That Need a Code

EspControl does not store lock PINs or codes on the panel. If your lock requires a code, create a Home Assistant script that handles the code securely, then use an [Action](/card-types/actions) card to run that script.

::: info Requires Home Assistant actions
Lock cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
