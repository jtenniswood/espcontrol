---
title: Climate Cards
description:
  How to use experimental climate cards on your EspControl panel to control Home Assistant climate entities.
---

# Climate

A Climate card controls a Home Assistant `climate` entity, such as a thermostat, heat pump, air conditioner, or radiator thermostat.

Climate cards are currently experimental. They can be useful, but they may change as more real devices are tested.

## Enabling Climate Cards

Climate cards only appear when experimental features are enabled:

1. Open the setup page with `?developer=experimental` at the end of the address, for example `http://192.168.1.50/?developer=experimental`.
2. Go to **Settings > Developer**.
3. Turn on **Developer/Experimental Features**.
4. Return to the **Screen** tab and add a **Climate** card.

If the card appears in the setup page but the panel does not show it after applying the configuration, install the latest [firmware update](/features/firmware-updates).

## Setting Up a Climate Card

1. Select a card and change its type to **Climate**.
2. Enter the **Climate Entity**, for example `climate.living_room`.
3. Set a **Label** if you want custom text. If left blank, the friendly name from Home Assistant is used.
4. Choose an **Icon**.
5. Choose **Unit Precision**:
   - **10** shows whole numbers.
   - **10.2** shows one decimal place.
6. Use **Advanced** only if you want to override the minimum or maximum temperature range shown on the panel.

## How It Works on the Panel

The card shows the current climate target and lights up when the climate entity is actively heating or cooling.

Tapping the card opens a climate control popup. From there, you can:

- Change the target temperature.
- Adjust low and high targets for heat/cool mode when the entity supports them.
- Change HVAC mode, such as Off, Heat, Cool, Heat/Cool, Auto, Dry, or Fan.
- Change fan, swing, or preset modes when the entity exposes those options.

The card follows Home Assistant attributes such as current temperature, target temperature, min/max temperature, target step, HVAC mode, fan mode, swing mode, and preset mode.

::: info Requires Home Assistant actions
Climate cards send Home Assistant climate actions from the panel. If controls do not respond, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
