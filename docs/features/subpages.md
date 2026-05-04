---
title: Subpage Cards
description:
  How to use Subpage cards on your EspControl panel to organise cards into folders.
---

# Subpage

![Subpage screen showing Back button and cover position buttons](/images/screen-subpage.png)

A Subpage card works like a folder. Tapping it on the panel opens a new page with its own set of cards. This is useful for grouping related controls together, such as all the lights in one room, without filling up the home screen.

A subpage has one fewer usable slot than the home screen because it includes a **Back** card. Subpage cards on the home screen show a small arrow badge so you can spot them easily.

## Setting Up a Subpage

1. Select a card on the home screen and change its type to **Subpage**.
2. Set a **Label** and **Icon** if you want them.
3. Click **Edit Subpage** in the card settings, or right-click the card and choose **Edit Subpage**.
4. The preview switches to the subpage. Add and arrange cards here the same way you would on the home screen.
5. Click the **Back** card to return to the home screen.

You can also right-click an empty space on the home screen and choose **Create Subpage**.

Subpages can contain Switch, Action, Trigger, Sensor, Slider, Cover, Garage Door, Date, World Clock, Weather, and Internal cards. Media cards can also be used when **Developer/Experimental Features** is enabled. Subpages cannot contain another Subpage card.

## Display State

Turn on **Display State** if you want the Subpage card on the home screen to show when something inside the subpage is active.

When **Display State** is on:

- The Subpage card lights up if any active-capable card inside it is on, open, playing, unlocked, or otherwise active.
- You can also enable **Change Icon When On** to use a different icon while anything inside the subpage is active.
- Read-only cards such as Sensor, Date, World Clock, and Weather do not affect the Subpage card state.

::: info Experimental
When **Developer: Experimental Features** is enabled, Subpage cards use **Show State** instead of **Display State** and can show state in three ways:

- **Icon** keeps the existing active indicator behavior, with separate Off Icon and On Icon choices.
- **Numeric** shows a Home Assistant sensor value in the large number style used by Sensor cards.
- **Text** shows a Home Assistant sensor state where the card label normally appears.

Numeric and Text modes use the sensor entity you enter on the Subpage card. They do not automatically count the cards inside the subpage; use a Home Assistant helper or template sensor for that.
:::

## Moving Cards Between Pages

You can cut, copy, and paste cards between the home screen and subpages. Right-click a card, choose **Cut** or **Copy**, then right-click an empty space on the destination page and choose **Paste**.
