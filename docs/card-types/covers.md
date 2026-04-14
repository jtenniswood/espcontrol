---
title: Covers
description:
  How to use cover cards on your Espcontrol panel to control blinds, shutters, and garage doors from Home Assistant.
---

# Covers

A cover card lets you control the position of a Home Assistant cover entity — blinds, shutters, roller shades, or garage doors — by sliding on the card.

<!-- ![Cover card showing a blinds icon with a position fill bar](/images/card-cover.png) -->

## Setting up a cover

1. Select a card and change its type to **Cover**.
2. Enter an **Entity ID** — the Home Assistant cover entity you want to control (for example, `cover.office_blind`).
3. Choose an **Icon** (defaults to a blinds icon when set to **Auto**).
4. Set a **Label** (optional) — shown at the bottom of the card. If left blank, the entity's friendly name from Home Assistant is used.
5. Pick a **Direction** — **Vertical** (default) or **Horizontal**.

## How it works on the panel

- **Drag** the slider to set the cover position from 0 (closed) to 100 (fully open). Releasing the slider sends the new position to Home Assistant via `cover.set_cover_position`.
- The **fill bar** represents how much the cover is closed — a fully closed cover shows a full bar, and a fully open cover shows an empty bar. This inverted fill matches blinds or shutters blocking a window.
- The fill bar updates in real time as the cover moves, tracking the `current_position` attribute from Home Assistant.

## Change Icon When Open

Enable **Change Icon When Open** to show a different icon while the cover is open. For example, you could use a closed blinds icon when shut and an open blinds icon when open.

When the cover is fully closed, the card reverts to the default icon.