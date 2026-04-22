---
title: Home Assistant Actions
description:
  How to allow your Espcontrol touchscreen to perform Home Assistant actions so it can control lights, switches, and other devices.
---

# Home Assistant Actions

Espcontrol needs permission to call Home Assistant actions (like toggling lights or switches) on your behalf. Without this, the touchscreen can display the time and screensaver but **buttons won't be able to control your devices**.

Your display will prompt you to do this during first-time setup. Follow the steps below.

## Enable Actions

1. **Open Home Assistant** and go to **Settings > Devices & Services** and add the discovered device (if the device wasn't discovered, find it's IP address and add it as an ESPHome device).

![Home Assistant discovering the Espcontrol device](/images/ha-actions-step-1.png)

2. **Find the ESPHome integration** and click on the top half (ESPHome > ), if you click on the number of devices, you'll end up on a different view.

![ESPHome integration showing connected devices](/images/ha-actions-step-2.png)

3. **Find your Espcontrol device** in the list. Click the **Configure** button (gear icon) next to it.

![Espcontrol device entry with configure button](/images/ha-actions-step-3.png)

4. **Check "Allow the device to perform Home Assistant actions"** and click **Submit**.

![Options dialog with the actions checkbox enabled](/images/ha-actions-step-4.png)

5. **Go back to your display** and tap **Done** on the setup screen. Your buttons will now be able to control Home Assistant devices.

::: tip One-time setup
You only need to do this once per device. The setting persists across firmware updates and device restarts.
:::

## What If I Skip This?

You won't be able to control any devices, it will be in a read only state, and entities such as lights, switches, and scenes, won't do anything when tapped.

## Device Not Showing Up?

If you don't see your Espcontrol device in the ESPHome integration, it may not have been added to Home Assistant yet. Head back to the [Install](/getting-started/install#add-to-home-assistant) guide to add it first.

