---
title: Climate Cards
description:
  How to use climate cards on your EspControl panel to control Home Assistant climate entities.
---

# Climate

A Climate card controls a Home Assistant `climate` entity, such as a thermostat, heat pump, air conditioner, or radiator thermostat.

![Climate card showing target temperature and current idle status](/images/card-climate.png)

## Setting Up a Climate Card

1. Select a card and change its type to **Climate**.
2. Enter the **Climate Entity**, for example `climate.living_room`.
3. Choose **Label Display**:
   - **Label** shows the card label.
   - **Status** shows the current climate status, such as Off, Heating, Cooling, or Idle.
   - **Actual Temp** shows the current measured temperature.
   - **Target Temp** shows the target temperature.
4. If **Label** is selected, set a **Label** if you want custom text. The default label is **Climate**.
5. Choose **Icon & Temperatures**:
   - **Icon** shows the selected icon instead of a large temperature. Choose separate **Off Icon** and **On Icon** values if you want the icon to change when the climate entity is on.
   - **Actual Temp** shows the current measured temperature as the large number.
   - **Target Temp** shows the target temperature as the large number.
6. Choose **Temperature Display**:
   - **10** shows whole numbers.
   - **10.2** shows one decimal place.
7. Choose **Temperature Step**:
   - **1 degree** changes the target by whole degrees.
   - **0.5 degree** changes the target by half degrees.
   Home Assistant limits still apply, and larger Home Assistant `target_temp_step` values are respected.
8. Use **Advanced** only if you want to override the minimum or maximum temperature range shown on the panel. Negative values are supported, for example `-25` to `5` for a freezer thermostat.

## How It Works on the Panel

The card can show either an icon or a large temperature. It lights up when Home Assistant reports that the climate entity is actively heating, cooling, drying, or running the fan. Idle and off states do not show as active.

Tapping the card opens a climate control popup. From there, you can:

- Change the target temperature.
- Adjust low and high targets for heat/cool mode when the entity supports them.
- Change HVAC mode, such as Off, Heat, Cool, Heat/Cool, Auto, Dry, or Fan.
- Change fan, swing, or preset modes when the entity exposes those options.

The card follows Home Assistant attributes such as current temperature, target temperature, min/max temperature, target step, HVAC mode, fan mode, swing mode, and preset mode.

::: info Requires Home Assistant actions
Climate cards send Home Assistant climate actions from the panel. If controls do not respond, check [Enable Actions](/getting-started/home-assistant-actions).
:::
