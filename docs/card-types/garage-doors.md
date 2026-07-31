---
title: Garage Door Cards
description:
  How to use garage door cards on your EspControl panel to open and close Home Assistant cover entities.
---

# Garage Door

A garage door card controls a Home Assistant `cover` entity as a simple toggle, or as dedicated open and close commands.

Unlike a **Cover** card, it does not show a slider. It normally shows your label, then briefly swaps that label for the live door state when the state changes.

## Setting Up a Garage Door

1. Select a card and change its type to **Garage Door**.
2. Choose an **Interaction**.
   - **Toggle** opens or closes the door with one card.
   - **Open** sends only an open command.
   - **Close** sends only a close command.
3. Enter an **Entity** — the Home Assistant garage door cover entity, for example `cover.garage_door`.
4. Set **Label Display** for toggle cards.
   - **Label** shows the card label normally, then briefly shows the live door state when it changes.
   - **Status** keeps the live door state visible on the card.
5. Choose the icons. Toggle cards use closed and open icons, while Open and Close command cards use a single icon.
6. Set a **Label** (optional). If left blank, toggle cards use the entity's friendly name from Home Assistant, and command cards show **Open** or **Close**.
7. Optionally turn on **Confirmation Required** if opening or closing this door by accident would be a problem.

## How It Works on the Panel

- In **Toggle** mode, tapping the card sends a toggle action to Home Assistant.
- In **Open** mode, tapping the card sends `cover.open_cover`.
- In **Close** mode, tapping the card sends `cover.close_cover`.
- If **Confirmation Required** is on, the panel asks before opening or closing the door.
- Toggle cards light up while the door is open, opening, or closing.
- Toggle cards can show the Home Assistant state, such as **Open**, **Closed**, **Opening**, or **Closing**, either briefly or all the time depending on **Label Display**.
- Open and Close command cards briefly flash when tapped. They do not stay highlighted based on the live door state.

## Confirmation

Use **Confirmation Required** for doors where an accidental tap would be a problem, such as a garage that opens onto a busy street.

On a **Toggle** card, enabling it shows three extra fields:

- **When** - choose whether the popup appears when closing, opening, or both.
- **Message** - the text shown in the confirmation popup. Defaults to "Close the garage door?",
  "Open the garage door?", or "Open or close the garage door?" depending on **When**.
- **Confirm Button** - the button that allows the action.
- **Cancel Button** - the button that leaves the door alone.

On an **Open** or **Close** command card, there is only one direction the card can trigger, so enabling **Confirmation Required** shows the **Message**, **Confirm Button**, and **Cancel Button** fields directly, without a **When** choice.

The default behavior is off, so existing cards keep working exactly as before.
