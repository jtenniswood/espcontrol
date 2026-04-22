---
title: Buttons
description:
  How to use buttons on your Espcontrol panel to trigger Home Assistant automations.
---

# Buttons

A button is a momentary button with no on/off state. When tapped, it flashes the highlight colour and fires an event to Home Assistant that you can use as an automation trigger.

Buttons are useful for things like triggering scenes, sending notifications, activating scripts, or anything else you'd start with a single tap.

![Button card with a tap gesture icon labelled Doorbell](/images/card-button.png)

## Setting Up a Button

1. Select a button and change its type to **Button**.
2. Set a **Label** — this is shown on the button and also sent to Home Assistant as part of the event data.
3. Choose an **Icon** (defaults to a tap gesture icon).

Buttons don't need an entity ID — they don't control a device directly.

## How It Works on the Panel

When you tap a button:

- The button instantly flashes the **on colour** (orange by default).
- The colour fades smoothly back to the **off colour** over 400 ms.
- An event is fired to Home Assistant with the button's label.

## Setting Up an Automation in Home Assistant

Buttons fire an event called `esphome.push_button_pressed` on the Home Assistant event bus. The event includes the button's **label** and **slot number**.

To create an automation:

1. In Home Assistant, go to **Settings > Automations & Scenes** and create a new automation.
2. Add a trigger and search for **event**. Select **Manual event received**.

![Add trigger dialog with "event" search showing Manual event received](/images/push-button-add-trigger.png)

3. Set **Event type** to `esphome.push_button_pressed`.
4. Under **Event data**, enter the label of your button:

```yaml
label: Front Door
```

![Event trigger configured with esphome.push_button_pressed and label Front Door](/images/push-button-event-trigger.png)

5. Add whatever actions you want — turn on a light, send a notification, run a script, etc.

::: tip Label-based triggers are resilient
Because the automation triggers on the button's **label** rather than its position, you can freely move the button to a different slot on the grid without breaking any automations.
:::

### Verifying Events Are Firing

If you want to confirm that events are being sent, go to **Developer Tools > Events** in Home Assistant, type `esphome.push_button_pressed` in the "Listen to events" field, and click **Start listening**. Press the button on your panel — the event should appear with the label and slot number.

### Example Event Data

When a button labelled "Doorbell" on slot 3 is pressed, Home Assistant receives:

```yaml
event_type: esphome.push_button_pressed
data:
  label: "Doorbell"
  slot: "3"
```

::: info Requires Home Assistant actions
The panel must be allowed to perform Home Assistant actions for button events to work. See [Home Assistant Actions](/getting-started/home-assistant-actions) for setup instructions.
:::
