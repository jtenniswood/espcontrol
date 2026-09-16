---
title: Notification Cards
description:
  How to show a Home Assistant message on your EspControl panel and colour it by
  how urgent it is.
---

# Notification

A Notification card shows one message from a Home Assistant entity and colours itself by how urgent that message is. It suits reminders, service alerts, "the garage has been open for an hour" style warnings, and anything else where the important part is a sentence rather than a number.

Notification cards are display cards, laid out like the Date & Time cards: the message sits where those cards put the time, with the level on the label line underneath, where they put the date. The message uses the largest text size the panel has, rather than the value font those cards use, because that font only carries digits. Tapping the card opens the full message, so a long sentence is always readable even on a small card.

## Setting Up a Notification Card

1. Select a card and change its type to **Notification**.
2. Enter a **Message Entity**, for example `sensor.house_notice`.
3. Optionally set a **Name**. It titles the clock bar while the expanded message is open, so you can tell which card you opened.

That is enough for most setups. The remaining settings live under **Notification Settings**:

| Setting | What it does |
|---|---|
| **Level Attribute** | The entity attribute that carries the urgency. Defaults to `notification_level`. |
| **Message Attribute** | Read the message from this attribute instead of the entity's state. Leave blank to use the state. |
| **Acknowledge Action** | A Home Assistant action such as `script.acknowledge_notice`, shown as an **Acknowledge** button in the expanded message. Leave blank for no button. |

## Levels and Colours

The level attribute changes the card colours and the word on the label line; it never changes the message itself.

| Level | Card appearance | Label line |
|---|---|---|
| `information` | Normal card colours. This is also used when the attribute is missing or unrecognised. | Information |
| `warning` | Muted yellow card with dark text. | Warning |
| `alert` | Red card with white text. | Alert |

The level is derived from the entity, so a Notification card has no icon setting of its own.

Common alternatives are recognised too, so you do not have to match the exact word: `warn`, `caution`, and `medium` count as a warning, and `critical`, `error`, `danger`, `severe`, and `high` count as an alert.

The expanded message also names the level in words, so the urgency is still clear if the colours are hard to tell apart.

## Long Messages

The message always stays on one line. When it is wider than the card, it scrolls across as a marquee; when it fits, it simply sits still. Wider card sizes therefore show more of the message at once, and **Ultra Wide** on a five-column screen shows the most. See [Card Sizes](/features/setup#card-sizes).

Tapping the card always shows the whole message, wrapped over as many lines as it needs.

Home Assistant limits an entity's state to 255 characters. If your message is longer than that, put it in an attribute and name that attribute in **Message Attribute**, as shown below.

## Creating the Sensor

The card reads any entity whose state is text, so a Home Assistant template sensor is usually the easiest source. Which shape you want depends on where the message comes from.

### A sensor that describes the current state

Use a plain template sensor when the message can be worked out from entities you already have. It re-evaluates itself whenever one of those entities changes, so there is no automation to write.

```yaml
template:
  - sensor:
      - name: House Notice
        state: >-
          {%- if is_state('binary_sensor.garage_door', 'on') -%}
            Garage door is open
          {%- elif is_state('lock.side_gate', 'unlocked') -%}
            Side gate is unlocked
          {%- else -%}
            All clear
          {%- endif -%}
        attributes:
          notification_level: >-
            {%- if is_state('binary_sensor.garage_door', 'on') -%}alert
            {%- elif is_state('lock.side_gate', 'unlocked') -%}warning
            {%- else -%}information
            {%- endif -%}
```

The `{%-` and `-%}` markers trim the surrounding whitespace, so the state comes out as a clean line rather than one padded with the indentation above.

### A sensor updated by state changes

Use a trigger-based template sensor when the message should be written at a particular moment and then stay put, rather than following an entity minute by minute. The sensor only re-evaluates when one of its triggers fires.

```yaml
template:
  - triggers:
      - trigger: state
        entity_id: binary_sensor.garage_door
        to: "on"
        for: "00:30:00"
    sensor:
      - name: House Notice
        state: "Garage door has been open for 30 minutes"
        attributes:
          notification_level: warning
```

### A sensor updated by events

Use an event trigger when something else in Home Assistant decides what the message should say. The sensor takes the wording and the level straight from the event.

```yaml
template:
  - triggers:
      - trigger: event
        event_type: house_notice
    sensor:
      - name: House Notice
        state: "{{ trigger.event.data.message }}"
        attributes:
          notification_level: "{{ trigger.event.data.level | default('information') }}"
```

Anything that can run an action can then post a notice, without knowing anything about the panel:

```yaml
alias: Tell the panel bin day is tomorrow
triggers:
  - trigger: time
    at: "18:00:00"
conditions:
  - condition: time
    weekday:
      - tue
actions:
  - event: house_notice
    event_data:
      message: Bin day is tomorrow
      level: information
```

A trigger-based sensor has no state until it first fires, so the card reads **No notifications** until then.

### Longer messages than the state allows

Home Assistant limits an entity's state to 255 characters. If your message is longer, keep the state short and put the full text in an attribute:

```yaml
template:
  - sensor:
      - name: House Notice
        state: "Garage door open"
        attributes:
          notification_level: warning
          detail: >-
            The garage door has been open for 45 minutes. The side gate is also
            unlocked, and rain is forecast within the hour.
```

Then set **Message Attribute** to `detail`.

## Showing More Than One Notice

A Notification card shows one message, because it shows one line. When you have several things to say at once, decide in Home Assistant what the card should read, and give it a single sentence.

Building the list in the template keeps that decision in one place. This example joins whatever is currently true and takes its colour from the most urgent of them:

```yaml
template:
  - sensor:
      - name: House Notice
        state: >-
          {%- set ns = namespace(notices=[]) -%}
          {%- if is_state('binary_sensor.severe_weather', 'on') -%}
            {%- set ns.notices = ns.notices + ['Severe thunderstorm warning until 6pm'] -%}
          {%- endif -%}
          {%- if is_state('binary_sensor.garage_door', 'on') -%}
            {%- set ns.notices = ns.notices + ['Garage door is open'] -%}
          {%- endif -%}
          {%- if is_state('input_boolean.bin_day_tomorrow', 'on') -%}
            {%- set ns.notices = ns.notices + ['Bin day is tomorrow'] -%}
          {%- endif -%}
          {{ ns.notices | join(' • ') if ns.notices else 'All clear' }}
        attributes:
          notification_level: >-
            {%- if is_state('binary_sensor.severe_weather', 'on') -%}alert
            {%- elif is_state('binary_sensor.garage_door', 'on') -%}warning
            {%- else -%}information
            {%- endif -%}
```

Three joined notices are longer than one card can show at a time, so the message scrolls. If the joined text runs past the 255-character state limit, move it into an attribute as above and set **Message Attribute**.

The alternative is to give each source its own card. Separate cards keep each message in its own colour and let you acknowledge them separately, which a joined sentence cannot do. A row of narrow cards also reads at a glance, where one long scrolling line does not.

## Troubleshooting

| Problem | What to check |
|---|---|
| The card reads **No notifications** | There is no message to show. A trigger-based template sensor reads as unknown until it first fires; otherwise check that the entity exists in Home Assistant and has a state. |
| The card stays in the normal colours and reads Information | Check that the attribute named in **Level Attribute** exists on the entity and its value is one of the recognised level names. |
| The message is cut off | The message is longer than Home Assistant's 255-character state limit. Move it to an attribute and set **Message Attribute**. |
| The message scrolls when you would rather it sat still | Use a wider card size, or shorten the message, so it fits the card on one line. |
| The **Acknowledge** button is missing | The action must be written as `domain.service`, for example `script.acknowledge_notice`. Anything else is ignored. |
