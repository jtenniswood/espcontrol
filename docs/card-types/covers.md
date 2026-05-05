---
title: Cover Cards
description:
  How to use cover cards on your EspControl panel to control blinds, shutters, and other cover entities from Home Assistant.
---

# Cover

A cover card lets you control a Home Assistant cover entity — blinds, shutters, roller shades, gates, or garage doors — as a slider, a toggle, or a one-tap command.

<!-- ![Cover card showing a blinds icon with a position fill bar](/images/card-cover.png) -->

## Setting Up a Cover

1. Select a card and change its type to **Cover**.
2. Choose the interaction:
   - **Slider: Position** lets you drag to a precise cover position.
   - **Slider: Tilt** lets you drag to a precise cover tilt position.
   - **Toggle** opens or closes the cover with a tap.
   - **Open**, **Close**, and **Stop** send that exact cover command.
   - **Set Position** sends the cover to the fixed percentage you enter.
3. Your Home Assistant cover entity needs to support tilt for **Slider: Tilt** mode to work.
4. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
5. Enter an **Entity ID** — the Home Assistant cover entity you want to control (for example, `cover.office_blind`).
6. Choose icons:
   - Slider and Toggle modes use **Closed Icon** and **Open Icon**.
   - Open, Close, Stop, and Set Position use one **Icon**.

## How It Works on the Panel

### Slider Interaction

- **Drag** the slider to set the selected cover value from 0 to 100.
- In **Position** mode, releasing the slider sends the new position to Home Assistant via `cover.set_cover_position`.
- In **Tilt** mode, releasing the slider sends the new tilt value to Home Assistant via `cover.set_cover_tilt_position`.
- The **fill bar** is always vertical and represents how much the cover is closed — a fully closed cover shows a full bar, and a fully open cover shows an empty bar. This inverted fill matches blinds or shutters blocking a window.
- The fill bar updates in real time as the cover moves, tracking `current_position` in **Position** mode and `current_tilt_position` in **Tilt** mode.

### Toggle Interaction

- **Tap** the card to toggle the cover through Home Assistant.
- The card lights up while the cover is closed or closing.
- When the cover state changes, the label temporarily shows the Home Assistant state, such as **Open**, **Closed**, **Opening**, or **Closing**.
- After the state settles, the card changes back to showing the configured label.

### Command Interactions

- **Open** sends `cover.open_cover`.
- **Close** sends `cover.close_cover`.
- **Stop** sends `cover.stop_cover`.
- **Set Position** sends `cover.set_cover_position` with the configured position from 0 to 100.
- Command cards briefly flash when tapped. They do not stay highlighted based on the live cover state.

## Cover Icons

Slider and Toggle cover cards use two icons: one for the closed state and one for the open or partially open state. Command cover cards use one icon.
