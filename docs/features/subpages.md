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

Subpages can contain Switch, Lights, Action, Trigger, Sensor, Slider, Cover, Garage Door, Lock, Date & Time, World Clock, Weather, Media, Climate, and Internal Switches cards. Experimental Fans cards can also be used in subpages when Developer/Experimental Features are enabled. Subpages cannot contain another Subpage card.

## Show State

Turn on **Show State** if you want the Subpage card on the home screen to show state.

Subpage cards can show state in three ways:

- **Icon** uses the card's **Icon** as the off icon and shows an **On Icon** when active. Enter a **State Entity** to track a specific Home Assistant entity, or leave it blank to keep the existing automatic behavior where the Subpage card lights up if any active-capable card inside it is on, open, playing, unlocked, or otherwise active.
- **Numeric** shows a Home Assistant sensor value in the large number style used by Sensor cards. Choose a **Sensor Entity**, **Unit**, and **Unit Precision**.
- **Text** shows a Home Assistant sensor state where the card label normally appears. Choose a **Sensor Entity**.

Read-only cards such as Sensor, Date, World Clock, and Weather do not affect Icon mode. Numeric and Text modes use the sensor entity you enter on the Subpage card. They do not automatically count the cards inside the subpage; use a Home Assistant helper or template sensor for that.

## Moving Cards Between Pages

You can cut, copy, and paste cards between the home screen and subpages. Right-click a card, choose **Cut** or **Copy**, then right-click an empty space on the destination page and choose **Paste**.
