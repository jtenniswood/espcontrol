---
title: Subpage Cards
description:
  How to use Subpage cards on your EspControl panel to organise cards into folders.
---

# Subpage

![Subpage screen showing Back button and cover position buttons](/images/screen-subpage.png)

A Subpage card works like a folder. Tapping it on the panel opens a new page with its own set of cards. This is useful for grouping related controls together, such as all the lights in one room, without filling up the home screen.

A subpage has one fewer usable slot than the home screen because it includes a **Back** card. Subpage cards on the home screen can show a small chevron marker so you can spot them easily. You can turn this marker on or off with **Screen: Subpage Chevron** in the Clock Bar settings.

## Setting Up a Subpage

1. Select a card on the home screen and change its type to **Subpage**.
2. Choose a subpage **Type**. **Generic** is a normal folder. The other presets make the home-screen Subpage tile look and behave like the thing it represents, such as **Lights**, **Switch**, **Alarm**, **Cover**, **Garage Door**, **Lock**, **Vacuum**, **Lawn Mower**, **Weather**, **Sensor**, or **Camera / Image**, before opening the detailed subpage.
3. Set a **Label** and **Icon** if you want them.
4. Click **Edit Subpage** in the card settings, or right-click the card and choose **Edit Subpage**.
5. The preview switches to the subpage. Add and arrange cards here the same way you would on the home screen.
6. Click the **Back** card to return to the home screen.

You can also right-click an empty space on the home screen and choose **Create Subpage**.

Subpages can contain Switch, Lights, Action, Local Action, Option Select, Webhook, Trigger, Sensor, Local Sensor, Doors & Windows, Presence, Slider, Fans, Vacuum, Lawn Mower, Cover, Garage Door, Lock, Alarm, Date & Time, Clock, World Clock, Weather, Camera, Media, Climate, Internal Switches, and Screen Lock cards. Subpages cannot contain another Subpage card.

## Open or Activate a Target From Home Assistant

You can ask Home Assistant to wake the panel and open or activate something on the home screen. This is useful in automations, scripts, dashboards, or voice routines where you want the panel to jump to a relevant page or open a card's normal control popup.

This Home-Assistant-to-panel action is disabled on the ESP32-S3 4-inch panel because it can stop Home Assistant completing the panel startup registration on that lower-memory model. Tapping Subpage cards on the panel still works normally.

Use the ESPHome action named after your device:

```yaml
action: esphome.<device_name>_navigate
data:
  target: "Lights"
```

Replace `<device_name>` with the ESPHome device name shown in Home Assistant. For example, if the device is called `hall_panel`, the action is:

```yaml
action: esphome.hall_panel_navigate
data:
  target: "Lights"
```

### Test It in Home Assistant

Before using the action in an automation or dashboard button, test it from Home Assistant:

1. Go to **Developer Tools**.
2. Open the **Actions** tab.
3. Search for `navigate` or your panel name, such as `hall_panel`.
4. Select the ESPHome action for your panel.
5. Enter the target page and click **Perform action**.

For example:

```yaml
action: esphome.hall_panel_navigate
data:
  target: "Lights"
```

To return to the home screen, use:

```yaml
action: esphome.hall_panel_navigate
data:
  target: "home"
```

The action is not an entity, so it will not appear in the entity list. It only appears in **Developer Tools** > **Actions** after the panel firmware has registered it with Home Assistant. If Home Assistant shows `Action not found` or `Unknown action selected`, update the panel firmware and reload or restart the ESPHome integration.

The `target` value can be:

- `home` or `main` to open the home screen.
- The **Label** you set on a home-screen card, such as `Lights`, `Heating`, `Camera`, or `Media`. Matching is not case-sensitive, so `lights` and `Lights` work the same way.
- `slot:3` to activate the card in home-screen slot 3.
- `voice`, `mic`, `microphone`, `speaker`, `volume`, or `device_volume` on voice-enabled ESP32-P4-86 firmware to open the device volume and microphone control popup when **Voice Services** are enabled.

You do not need to know a page number. Use the same label you gave the card on the home screen.

If two home-screen cards use the same label, the first matching displayed slot is used. To avoid surprises, give cards you want to target a unique label. If Home Assistant sends a label or slot that does not exist, the panel logs a warning and stays on the current page.

Targeting a normal home-screen card is the same as tapping it on the panel. Camera or image, climate, media volume, light control, cover, alarm, option-select, todo, and similar cards open their normal popup. Action, toggle, webhook, lock, garage, cover command, vacuum, mower, and other command cards can send real Home Assistant commands, so target those carefully.

The panel wakes before navigating, so the action works when the screen is off, dimmed, or showing the clock screensaver. It does not change long-press behavior. If you use the [Home screen timeout](/features/idle), the panel will still return to the home screen using that normal setting.

## Show State

Turn on **Show State** if you want the Subpage card on the home screen to show state.

Subpage cards can show state in three ways:

- **Icon** uses the card's **Icon** as the off icon and shows an **On Icon** when active. Enter a **State Entity** to track a specific Home Assistant entity, or leave it blank to keep the existing automatic behavior where the Subpage card lights up if any active-capable card inside it is on, open, playing, unlocked, or otherwise active.
- **Numeric** shows a Home Assistant sensor value in the large number style used by Sensor cards. Choose a **Sensor Entity**, **Unit**, and **Unit Precision**.
- **Text** shows a Home Assistant sensor state where the card label normally appears. Choose a **Sensor Entity**.

Read-only cards such as Sensor, Date, Clock, World Clock, and Weather do not affect Icon mode. Numeric and Text modes use the sensor entity you enter on the Subpage card. They do not automatically count the cards inside the subpage; use a Home Assistant helper or template sensor for that.

## Moving Cards Between Pages

You can cut, copy, and paste cards between the home screen and subpages. Right-click a card, choose **Cut** or **Copy**, then right-click an empty space on the destination page and choose **Paste**.

## Copying Cards Between Controllers

To copy a card to another EspControl panel:

1. Right-click the card and choose **Copy Card Code**. If you selected several cards, choose **Copy Cards as Code**.
2. Use **Copy Code**. Most EspControl pages use a local HTTP address, so your browser may instead select the code and ask you to press **Ctrl+C** or **Command+C**.
3. Open the setup page for the other controller, right-click an empty position, and choose **Paste Card Code**.
4. Paste the code into the box and choose **Paste**.

Card codes include the card size and any attached subpage. When the destination screen is a different size, EspControl finds suitable empty positions and may reduce a large card to a single tile. The complete group is checked before anything is saved, so a multi-card transfer is not partly applied when there is insufficient room.

Cards that use an internal relay, local action, or local sensor may need to be edited for the destination controller. Card codes can also contain private webhook URLs or headers, so keep them private and do not post them publicly.
