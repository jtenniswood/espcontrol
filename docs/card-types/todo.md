---
title: Todo Cards
description:
  How to use Todo cards on your EspControl panel with Home Assistant todo lists.
---

# Todo

Todo cards show a Home Assistant `todo.*` list on the panel. The card shows the number of incomplete items. Tapping the card opens the list, where each visible item can be tapped to mark it complete.

## Setting Up a Todo Card

1. Select a card and change its type to **Todo**.
2. Choose an **Entity**, such as `todo.shopping`.
3. Set **Label** if you want a custom list name.
4. Use **Card Label** to choose whether the card shows the list name or the incomplete item count.
5. Use **Status** to choose whether the card shows the selected icon, the item counter, or the top task.
6. Use **Show Completed Items** to choose whether items checked off on the panel stay visible in a completed section.

## How It Works

Home Assistant todo entities only expose the incomplete item count as their normal state. EspControl requests the actual item list from Home Assistant when you open the card.

The panel uses Home Assistant actions to:

- fetch incomplete items from the selected todo list
- mark a tapped item as complete

Make sure the device is allowed to perform Home Assistant actions. See [Home Assistant Actions](/getting-started/home-assistant-actions) if list loading or completion does not work.

## Current Limits

This first test version is intentionally small:

- it shows incomplete items, with locally checked-off items optional
- it can complete items
- it can optionally show and restore completed items
- it cannot add, delete, or rename items
- very long lists are capped on the panel to protect memory
