---
title: Battery
description: Display Home Assistant battery charge as an icon and percentage.
---

# Battery

A Battery card shows a Home Assistant battery sensor as a percentage and an icon
that changes with the charge level. It works on the home screen and in subpages.
The card is read-only: tapping it does not send an action to Home Assistant.

## Set up a Battery card

1. Select a card and choose **Battery** as its type.
2. Choose a **Battery Entity**, such as `sensor.phone_battery`.
3. Save the card.

Use a sensor whose numeric state reports charge from `0` to `100`. The card
rounds to the nearest whole percent and limits readings to that range. A missing,
unavailable, or invalid reading shows an unknown battery icon and `--%`.

The icon updates automatically, so there is no separate icon or label setting.
The web editor uses an example `80%` preview; the panel shows the live reading.

For the panel's own battery indicator, see [Battery settings](/features/battery).
