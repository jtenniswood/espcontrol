---
title: Alarm Cards
description:
  How to use experimental Alarm cards on your EspControl panel to arm, disarm, and view Home Assistant alarm_control_panel entities.
---

# Alarm

::: warning Experimental
Alarm cards are currently behind **Developer/Experimental Features**. To add one, open the setup page with `?developer=experimental`, enable **Developer/Experimental Features** in Settings, then choose **Alarm** from the card type list.
:::

An Alarm card controls a Home Assistant `alarm_control_panel` entity. The home card shows the current alarm state. Tapping it opens a generated alarm control page with normal EspControl-style action cards.

## Setting Up an Alarm

1. Select a home-screen card and change its type to **Alarm**.
2. Enter the **Alarm Entity**, for example `alarm_control_panel.house`.
3. Set a label and icon if you want custom text or a custom icon.
4. Leave **PIN required for arming** and **PIN required for disarming** on unless your alarm does not require a code.
5. Choose which actions should appear on the alarm control page: **Arm Away**, **Arm Home**, **Arm Night**, and **Disarm**.

Alarm action cards are generated automatically from the Alarm card settings. They are not saved as normal subpage cards and do not use normal card slots.

## PIN Keypad

When an action requires a PIN, the panel opens a keypad. The PIN is shown as `****`, sent as text so leading zeroes work, and cleared when you submit, close, or cancel the keypad.

EspControl does not save the PIN in the card configuration.

## How It Works on the Panel

- **Disarmed** uses the secondary/off card colour.
- **Armed** states use the primary/on card colour.
- **Arming** and **Pending** show that temporary state in the card label.
- **Triggered** shows a red card state.
- **Unknown** or **Unavailable** disables the card and shows that state clearly.

The panel sends the standard Home Assistant alarm actions: `alarm_control_panel.alarm_arm_away`, `alarm_arm_home`, `alarm_arm_night`, and `alarm_disarm`.

::: info Requires Home Assistant actions
Alarm cards send Home Assistant actions from the panel. If tapping a card does nothing, check [Home Assistant Actions](/getting-started/home-assistant-actions).
:::
