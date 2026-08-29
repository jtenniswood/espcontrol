---
title: Alarm Cards
description:
  How to use alarm cards on your EspControl panel to arm, disarm, and monitor Home Assistant alarm_control_panel entities.
---

# Alarm

An Alarm card controls a Home Assistant `alarm_control_panel` entity. It can provide **All Controls**, or a one-tap card for **Arm Away**, **Arm Home**, **Arm Night**, **Arm Vacation**, or **Disarm**.

Use Alarm cards for house alarms, zone alarms, and Home Assistant alarm integrations that expose an `alarm_control_panel` entity.

## Setting Up an Alarm Card

1. Select a card and change its type to **Alarm**.
2. Choose the alarm **Type**:
   - **All Controls** opens an alarm control screen with the visible actions you choose.
   - **Arm Away** sends the arm-away action.
   - **Arm Home** sends the arm-home action.
   - **Arm Night** sends the arm-night action.
   - **Arm Vacation** sends the arm-vacation action.
   - **Disarm** sends the disarm action.
3. Enter the **Alarm Entity**, for example `alarm_control_panel.house`.
4. For **All Controls**, choose the visible actions for this panel.
5. For **All Controls**, choose whether the card label shows the alarm name or the current alarm status.
6. For **All Controls**, choose whether the icon is static or follows the current alarm status.
7. Choose whether a PIN is required for arming, disarming, or both.

## All Controls

All Controls is the most complete alarm card mode. Tapping the card opens an alarm screen on the panel, where the available actions can include **Arm Away**, **Arm Home**, **Arm Night**, **Arm Vacation**, and **Disarm**.

The setup page lets you choose up to three actions to appear. This is useful when a panel should allow arming but not disarming, or when you only use certain arming modes such as Night and Away.

The card can show:

- **Name** - the label or Home Assistant friendly name.
- **Status** - the current alarm state, such as Disarmed, Armed Away, Armed Home, Armed Night, Armed Vacation, Pending, Triggered, or Unavailable.
- **Static icon** - the icon you choose.
- **Status icon** - an icon that changes with the alarm state.

## One-Tap Alarm Actions

The one-tap modes create a simpler card for one specific action.

| Mode | Home Assistant action |
|---|---|
| **Arm Away** | `alarm_control_panel.alarm_arm_away` |
| **Arm Home** | `alarm_control_panel.alarm_arm_home` |
| **Arm Night** | `alarm_control_panel.alarm_arm_night` |
| **Arm Vacation** | `alarm_control_panel.alarm_arm_vacation` |
| **Disarm** | `alarm_control_panel.alarm_disarm` |

These modes still track the alarm state so the card can react to the selected alarm entity. If a PIN is required for the selected action, the panel asks for it before sending the command.

## PIN Handling

Alarm cards can require a PIN for arming and disarming. The PIN is entered on the panel when the action is used.

Use the PIN settings to match how you want the wall panel to behave:

- Leave **PIN required for arming** on if you do not want accidental arm actions.
- Leave **PIN required for disarming** on for panels in shared spaces.
- Turn off the arming PIN only when quick arming is safe for that panel location.

## Entry and Exit Delays

When Home Assistant reports an arming or entry delay, All Controls shows a countdown and progress bar. On the **ESP32-P4 86 Panel**, optional delay sounds are available under **Settings → Alarm Audio**. They are off by default.

## How It Works on the Panel

- The card subscribes to the alarm entity state in Home Assistant.
- All Controls opens the alarm control screen when tapped.
- One-tap modes send only their selected arm or disarm action.
- The card can be used on the home screen or inside subpages.
- If Home Assistant reports the alarm as unavailable, the card shows that state instead of pretending the action succeeded.

::: info Requires Home Assistant actions
Alarm cards send Home Assistant alarm actions from the panel. If arming or disarming does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
