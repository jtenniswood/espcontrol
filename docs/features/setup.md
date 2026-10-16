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
| **System** | Set language and time, rename the panel, back up, and update firmware. | [Preferences](/settings/preferences), [System](/settings/maintenance) |

<span id="adding-a-card"></span>
<span id="active-switch-display"></span>
<span id="moving-cards"></span>
<span id="card-sizes"></span>

## Set Up Your First Controls

1. [Install EspControl](/getting-started/install) and add the panel to Home Assistant.
2. [Enable Home Assistant actions](/getting-started/home-assistant-actions) so the panel can control devices.
3. Open **Layout**, select an empty **+** slot, and choose **Lights** or **Switch**.
4. Enter your Home Assistant entity ID, such as `light.kitchen`, and choose a label and icon.
5. Choose **Apply Configuration**, wait for the panel to reconnect, then tap the card on the physical screen to check it works.

Use the [card guide](/card-types/) to choose other controls, or [Subpages](/features/subpages) to group them into rooms and pages.

## Apply Configuration

After editing, choose **Apply Configuration** at the bottom of any tab. This restarts the panel and loads the saved settings. Wait for it to reconnect before making further changes. Some settings, such as colours and brightness, also update immediately while you edit.

## Device Settings

For **Screen**, follow [Appearance](/features/appearance), [Display](/settings/display), or [Sleep & Schedule](/settings/sleep). For **System**, follow [Preferences](/settings/preferences) or [system controls](/settings/maintenance). Model-specific controls appear only on supported panels; missing controls may also need newer [firmware](/settings/maintenance#firmware).

## Naming Your Panel

Use **System > Device Name > Save & Restart**. Follow [Rename your panel](/settings/maintenance#device-name), including the notes about its new address and Home Assistant automations.

## Home Assistant Settings

Use **System > Home Assistant Settings** if camera images or media artwork fail to load. Follow [Connect images and artwork](/settings/maintenance#home-assistant-settings).

## Restart From Home Assistant

Open your EspControl device in Home Assistant and press **Restart** under its configuration controls. This keeps your saved settings. See [Restart your panel](/settings/maintenance#restart) for automation use.
