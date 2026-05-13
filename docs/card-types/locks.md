---
title: Lock Cards
description:
  How to use lock cards on your EspControl panel to lock, unlock, and view Home Assistant lock entities.
---

# Lock

A Lock card controls a Home Assistant `lock` entity. It can work as a state-aware toggle, or as a one-tap **Lock** or **Unlock** command.

## Setting Up a Lock

1. Select a card and change its type to **Lock**.
2. Choose the lock **Type**:
   - **Toggle** shows lock state and chooses lock or unlock based on that state.
   - **Lock** always sends `lock.lock`.
   - **Unlock** always sends `lock.unlock`.
3. Enter an **Entity** - the Home Assistant lock entity, for example `lock.front_door`.
4. Choose icons. Toggle cards use locked and unlocked icons; Lock and Unlock command cards use one icon.
5. Set a **Label** if you want custom text. If left blank, Toggle uses the friendly name from Home Assistant, while command cards use **Lock** or **Unlock**.

## How It Works on the Panel

- In **Toggle** mode, tapping a locked Lock card sends `lock.unlock` to Home Assistant.
- In **Toggle** mode, tapping an unlocked, open, or jammed Lock card sends `lock.lock`.
- If the panel does not know the current state yet, Toggle mode sends `lock.lock` rather than unlocking.
- Toggle cards light up while the lock is unlocked, unlocking, open, opening, or jammed.
- When the lock state changes, Toggle cards temporarily show the Home Assistant state, such as **Locked**, **Unlocked**, **Unlocking**, **Open**, or **Jammed**.
- In **Lock** or **Unlock** mode, tapping the card sends only that exact command. The card does not stay highlighted based on the live lock state.

## Opening or Unlatching

Lock cards handle locking and unlocking. If your lock hardware has a separate open or unlatch command, create a Home Assistant script for that command and run the script with an [Action](/card-types/actions) card.

## Locks That Need a Code

EspControl does not store lock PINs or codes on the panel. If your lock requires a code, create a Home Assistant script that handles the code securely, then use an [Action](/card-types/actions) card to run that script.

::: info Requires Home Assistant actions
Lock cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
