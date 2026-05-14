---
title: Home Assistant Actions for EspControl
description:
  How to allow your EspControl touchscreen to perform Home Assistant actions so it can control lights, switches, and other devices.
---

# Home Assistant Actions

EspControl needs permission to call Home Assistant actions (like toggling lights, running scripts, changing media volume, or adjusting climate targets) on your behalf. Without this, the touchscreen can display read-only information but **cards won't be able to control your devices or request forecast data**.

Your display will prompt you to do this during first-time setup. Follow the steps below.

## Enable Actions

1. **Open Home Assistant** and go to **Settings > Devices & Services** and add the discovered device (if the device wasn't discovered, find its IP address and add it as an ESPHome device).

![Home Assistant discovering the EspControl device](/images/ha-actions-step-1.png)

2. **Find the ESPHome integration** and click on the top half (ESPHome > ), if you click on the number of devices, you'll end up on a different view.

![ESPHome integration showing connected devices](/images/ha-actions-step-2.png)

3. **Find your EspControl device** in the list. Click the **Configure** button (gear icon) next to it.

![EspControl device entry with configure button](/images/ha-actions-step-3.png)

4. **Check "Allow the device to perform Home Assistant actions"** and click **Submit**.

![Options dialog with the actions checkbox enabled](/images/ha-actions-step-4.png)

5. **Go back to your display** and tap **Done** on the setup screen. Your cards will now be able to control Home Assistant devices.

::: tip One-time setup
You only need to do this once per device. The setting persists across firmware updates and device restarts.
:::

## What If I Skip This?

You won't be able to control any devices, it will be in a read-only state, and entities such as lights, switches, fans, scenes, scripts, helpers, covers, locks, media players, and climate devices won't do anything when tapped. Weather cards set to **Temperatures Today** or **Temperatures Tomorrow** also won't be able to fetch the daily forecast.

## Device Not Showing Up?

If you don't see your EspControl device in the ESPHome integration, it may not have been added to Home Assistant yet. Head back to the [Install](/getting-started/install#add-to-home-assistant) guide to add it first.
