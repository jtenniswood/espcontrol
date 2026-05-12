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
4. Choose the icons. Toggle cards use closed and open icons, while Open and Close command cards use a single icon.
5. Set a **Label** (optional). If left blank, toggle cards use the entity's friendly name from Home Assistant, and command cards show **Open** or **Close**.

## How It Works on the Panel

- In **Toggle** mode, tapping the card sends a toggle action to Home Assistant.
- In **Open** mode, tapping the card sends `cover.open_cover`.
- In **Close** mode, tapping the card sends `cover.close_cover`.
- Toggle cards light up while the door is open, opening, or closing.
- Toggle cards temporarily show the Home Assistant state, such as **Open**, **Closed**, **Opening**, or **Closing**.
- Open and Close command cards briefly flash when tapped. They do not stay highlighted based on the live door state.
