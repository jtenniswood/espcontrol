---
title: "Configure Your EspControl Device"
description: Open your EspControl device in a browser and use Layout, Screen, and System to set up cards, display behaviour, and device preferences.
---

# Configure Your Device

Your panel has a built-in setup page. Open its IP address or `.local` address in a browser on your phone or computer connected to the same network.

Find the address on the panel's first-time setup screen, in your router's device list, or in Home Assistant under **Settings > Devices & services > ESPHome**. On a configured panel, tap the connectivity icon in the top bar to see its IP address and firmware version.

## Choose the Right Tab

The documentation follows the tabs on your device:

| Tab | Use it to | Start here |
|---|---|---|
| **Layout** | Add, arrange, resize, and edit cards and subpages. | [First controls](#set-up-your-first-controls) |
| **Screen** | Change appearance, brightness, rotation, and sleep behaviour. | [Appearance](/features/appearance), [Display](/settings/display), [Sleep & Schedule](/settings/sleep) |
| **System** | Set language and time, rename the panel, back up, and update firmware. | [System settings](/settings/system) |

<span id="adding-a-card"></span>
<span id="active-switch-display"></span>
<span id="layout-settings"></span>

## Set Up Your First Controls

1. [Install EspControl](/getting-started/install) and add the panel to Home Assistant.
2. [Enable Home Assistant actions](/getting-started/home-assistant-actions) so the panel can control devices.
3. Open **Layout**, select an empty **+** slot, and choose **Lights** or **Switch**.
4. Enter your Home Assistant entity ID, such as `light.kitchen`, and choose a label and icon.
5. Choose **Apply Configuration**, wait for the panel to reconnect, then tap the card on the physical screen to check it works.

Use the [card guide](/card-types/) to choose other controls, or [Subpages](/features/subpages) to group them into rooms and pages.

<span id="add-or-edit-a-card"></span>

## Edit a Card

Select an existing card and choose **Edit** to change its entity, label, icon, or available controls. Find entity IDs under **Settings > Devices & services** in Home Assistant; some cards use a specific field such as **Sensor Entity** or **Weather Entity**. A blank label uses the Home Assistant friendly name where available.

To choose another type for a new card, close its settings and start again from the empty slot. Local cards such as Clock, Screen Lock, and Internal Switches do not need a Home Assistant entity; check the [card guide](/card-types/) for requirements.

<span id="moving-cards"></span>
<span id="card-sizes"></span>

## Arrange and Resize

| Task | What to do |
|---|---|
| Move a card | Drag it to another position. An occupied card moves to the next available slot. |
| Resize a card | Hover over it and drag the bottom-right handle. Release to save, or press **Escape** to cancel. |
| Rename a card | Click its title, type a name, and press **Enter**. Press **Escape** or click away to cancel. |
| Change its colour | Right-click the card and open **Change Colour**, or edit **Custom Colours**. |
| Copy between pages | Right-click and choose **Cut** or **Copy**, then **Paste** in an empty slot on the destination page. |
| Copy to another panel | Use **Copy Code** and **Paste Code**; follow the [transfer steps](/features/subpages#copying-cards-between-controllers). |

Sizes depend on the card type, panel, and orientation. The handle snaps to supported sizes that fit. If nearby cards cannot move to make room, the size change is not applied. Live text such as dates and sensor readings cannot be renamed directly.

## Organise Controls into Subpages

Add a **Subpage** card for a room or group of devices, then choose **Edit Subpage** to add its controls. Use **Back** to return home. Subpages cannot contain other subpages. See [Subpages](/features/subpages) for state indicators, copying, and navigation automations.

## Edit the Clock Bar

Select **Temperature**, **Clock**, or **Connectivity** in the preview to edit or hide that item. Hidden items stay greyed so you can show them again. See [Clock Bar](/features/appearance#clock-bar) for the available controls and [System settings](/settings/system#preferences) for time and temperature units.

<span id="choose-more-controls"></span>

For multi-room audio, follow [Speaker Groups](/features/speaker-groups). If a card shows a value but cannot control its device, check [Home Assistant action permissions](/getting-started/home-assistant-actions).

## Apply Configuration

After editing, choose **Apply Configuration** at the bottom of any tab. This restarts the panel and loads the saved settings. Wait for it to reconnect before making further changes. Some settings, such as colours and brightness, also update immediately while you edit.

## Device Settings

For **Screen**, follow [Appearance](/features/appearance), [Display](/settings/display), or [Sleep & Schedule](/settings/sleep). For **System**, follow [System settings](/settings/system). Model-specific controls appear only on supported panels; missing controls may also need newer [firmware](/settings/system#firmware).

## Naming Your Panel

Use **System > Device Name > Save & Restart**. Follow [Rename your panel](/settings/system#device-name), including the notes about its new address and Home Assistant automations.

## Home Assistant Settings

Use **System > Home Assistant Settings** if camera images or media artwork fail to load. Follow [Connect images and artwork](/settings/system#home-assistant-settings).

## Restart From Home Assistant

Open your EspControl device in Home Assistant and press **Restart** under its configuration controls. This keeps your saved settings. See [Restart your panel](/settings/system#restart) for automation use.
