---
title: "Layout Settings"
description: Add and edit EspControl cards, arrange and resize your layout, and organise controls into subpages from the Layout tab.
---

# Layout Settings

Open your [device setup page](/features/setup) and choose **Layout**. The preview shows the cards and clock bar on your panel.

## Add or Edit a Card

1. Select an empty **+** slot and choose a [card type](/card-types/). To edit an existing card, select it and choose **Edit**.
2. Enter the Home Assistant entity ID, such as `light.kitchen`. Find it under **Settings > Devices & services** in Home Assistant. Some cards use a specific field such as **Sensor Entity** or **Weather Entity**.
3. Choose the available controls, label, and icon. A blank label uses the Home Assistant friendly name where available.
4. Choose **Apply Configuration**, wait for the panel to reconnect, and test the card on the touchscreen.

To choose another type for a new card, close its settings and start again from the empty slot. Local cards such as Clock, Screen Lock, and Internal Switches do not need a Home Assistant entity; check the [card guide](/card-types/) for requirements.

![Light card settings with entity, label, and icon controls](/images/settings-panel-light-card.png)

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

Select **Temperature**, **Clock**, or **Connectivity** in the preview to edit or hide that item. Hidden items stay greyed in the preview so you can show them again.

- **Temperature**: choose a Home Assistant sensor and whether to show the degree symbol.
- **Clock / Connectivity**: choose **Hide** or **Show**.
- To hide the entire bar, use **Screen > Appearance > Clock Bar**.

The bar has fixed positions. Set [time and timezone](/features/clock) and [temperature units](/features/temperature) under **System > Preferences**.

## Choose More Controls

Use the [card guide](/card-types/) to compare card types. Follow [Speaker Groups](/features/speaker-groups) for multi-room audio or the [practical guides](/guides/) for room controls, cameras, and photos.

If a card shows a value but cannot control its device, check [Home Assistant action permissions](/getting-started/home-assistant-actions).
