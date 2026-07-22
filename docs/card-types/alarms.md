---
title: Alarm Cards
description:
  How to use alarm cards on your EspControl panel to arm, disarm, and monitor Home Assistant alarm_control_panel entities.
---

# Alarm

An Alarm card controls a Home Assistant `alarm_control_panel` entity. It can be a combined control panel card, or a one-tap card for **Arm Away**, **Arm Home**, **Arm Night**, **Arm Vacation**, or **Disarm**.

Use Alarm cards for house alarms, zone alarms, and Home Assistant alarm integrations that expose an `alarm_control_panel` entity.

## Setting Up an Alarm Card

1. Select a card and change its type to **Alarm**.
2. Choose the alarm **Type**:
   - **Combined Control** opens an alarm control screen with the visible actions you choose.
   - **Arm Away** sends the arm-away action.
   - **Arm Home** sends the arm-home action.
   - **Arm Night** sends the arm-night action.
   - **Arm Vacation** sends the arm-vacation action.
   - **Disarm** sends the disarm action.
3. Enter the **Alarm Entity**, for example `alarm_control_panel.house`.
4. For **Combined Control**, choose the visible actions for this panel.
5. For **Combined Control**, choose whether the card label shows the alarm name or the current alarm status.
6. For **Combined Control**, choose whether the icon is static or follows the current alarm status.
7. Choose whether a PIN is required for arming, disarming, or both.

## Combined Control

Combined Control is the most complete alarm card mode. Tapping the card opens an alarm screen on the panel, where the available actions can include **Arm Away**, **Arm Home**, **Arm Night**, **Arm Vacation**, and **Disarm**.

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

If Home Assistant reports an alarm arming delay or pending entry delay, the panel shows the delay state in the alarm screen with a countdown timer. A progress bar under the timer gives a quick visual indication of how much delay time is left before the alarm changes state.

The delay display follows the alarm entity updates from Home Assistant, so it works whether Home Assistant sends the total delay once or keeps sending updated remaining-time values during the countdown.

### Alarm Audio

The **ESP32-P4 86 Panel** can also provide audible entry and exit delay feedback. This is off by default, so updating does not make an existing alarm setup start sounding unexpectedly.

Open **Settings → Alarm Audio** to configure:

- **Alarm Delay Audio** — enables all alarm delay sounds.
- **TTS Announcements** — speaks the configured entry or exit message through the panel using its Home Assistant Assist voice pipeline when Voice Services are enabled. No separate Home Assistant automation is required. Beeping waits briefly for that announcement and starts after it finishes.
- **Entry Announcement** and **Exit Announcement** — the message spoken at the start of each delay.
- **Beep Volume** — the local warning-beep volume.
- **Faster Beeps During Final Seconds** — changes the final countdown period; the default is 10 seconds.

The exit delay uses a lower single tone, while the entry delay uses a higher double tone. The tones are generated locally and sent directly to the panel's audio mixer, avoiding a fresh media-decoder startup for every beep. Beeps repeat every second and change to every 700 ms during the final countdown. They stop when the countdown reaches zero or the alarm leaves `arming`/`pending`, including when it is disarmed, armed, triggered, or unavailable. TTS is skipped silently when Voice Services are off; local beeps continue.

Firmware compile checks confirm that the speaker panel and non-speaker panels build safely, but they cannot prove sound level, tone clarity, or media restoration. After installing the test firmware on an ESP32-P4 86 Panel, physically check both alarm delays, the faster final countdown, immediate stopping after disarm, Voice Services off, and normal media playback after the alarm clears.

## How It Works on the Panel

- The card subscribes to the alarm entity state in Home Assistant.
- Combined Control opens the alarm control screen when tapped.
- One-tap modes send only their selected arm or disarm action.
- The card can be used on the home screen or inside subpages.
- If Home Assistant reports the alarm as unavailable, the card shows that state instead of pretending the action succeeded.

::: info Requires Home Assistant actions
Alarm cards send Home Assistant alarm actions from the panel. If arming or disarming does nothing, check [Enable Actions](/getting-started/home-assistant-actions).
:::
