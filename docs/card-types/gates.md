---
title: Gate Cards
description:
  How to use gate cards on your EspControl panel to open, close, and stop Home Assistant cover entities.
---

# Gate

A gate card controls a Home Assistant `cover` entity (such as an electric driveway gate) as a simple toggle, or as dedicated open, close, and stop commands.

Unlike a **Cover** card, it does not show a slider. It normally shows your label, then briefly swaps that label for the live gate state when the state changes.

## Setting Up a Gate

1. Select a card and change its type to **Gate**.
2. Choose an **Interaction**.
   - **Toggle** opens or closes the gate with one card.
   - **Open** sends only an open command.
   - **Close** sends only a close command.
   - **Stop** sends only a stop command, useful for gates that support stopping mid-travel.
3. Enter an **Entity** — the Home Assistant gate cover entity, for example `cover.driveway_gate`.
4. Set **Label Display** for toggle cards.
   - **Label** shows the card label normally, then briefly shows the live gate state when it changes.
   - **Status** keeps the live gate state visible on the card.
5. Choose the icons. Toggle cards use closed and open icons, while Open, Close, and Stop command cards use a single icon.
6. Set a **Label** (optional). If left blank, toggle cards use the entity's friendly name from Home Assistant, and command cards show **Open**, **Close**, or **Stop**.

## How It Works on the Panel

- In **Toggle** mode, tapping the card sends a toggle action to Home Assistant.
- In **Open** mode, tapping the card sends `cover.open_cover`.
- In **Close** mode, tapping the card sends `cover.close_cover`.
- In **Stop** mode, tapping the card sends `cover.stop_cover`.
- Toggle cards light up while the gate is open, opening, or closing.
- Toggle cards can show the Home Assistant state, such as **Open**, **Closed**, **Opening**, or **Closing**, either briefly or all the time depending on **Label Display**.
- Open, Close, and Stop command cards briefly flash when tapped. They do not stay highlighted based on the live gate state.
