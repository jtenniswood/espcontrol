---
title: Firmware Updates
description:
  How the Espcontrol panel checks for and installs firmware updates over the air, and how to control update behaviour.
---

# Firmware Updates

Your panel can update its firmware over the air — no USB cable or computer needed after the initial install. When a new version is available, the panel downloads and installs it automatically (if enabled) or waits for you to trigger the update manually.

## Update Settings

These are configured from the **Settings** tab in the [Setup](/features/setup) under the **Firmware** section. They also appear as controls in Home Assistant.

- **Version** — the firmware version currently running on your panel (read-only).
- **Auto Update** — turn this on to let the panel install new versions automatically. When off, you'll need to trigger updates manually.
- **Beta Channel** — turn this on to check for pre-release firmware as well as stable releases. It is off by default.
- **Update Frequency** — how often the panel checks for updates: **Hourly**, **Daily**, **Weekly**, or **Monthly**.
- **Check for Update** — press this button to check for a new version right now, regardless of the automatic schedule. It checks stable releases, and also checks beta releases when **Beta Channel** is on.

## What Happens During an Update

1. The panel checks the update server for a newer stable version. If **Beta Channel** is on, it also checks the beta channel.
2. Stable updates install automatically when **Auto Update** is on. Beta updates are shown after a check so you can choose when to install them.
3. When an update is installed, the panel restarts with the new firmware. Your settings (buttons, colours, temperatures, etc.) are preserved.

The update usually takes a minute or two. The display may show a loading screen briefly during the restart.

## Checking Updates from Home Assistant

You can also manage updates from Home Assistant. The **Auto Update** toggle, **Beta Channel** toggle, **Update Frequency** selector, and **Check for Update** button all appear as entities that you can control from the Home Assistant dashboard or use in automations.

The standard Home Assistant **Update** entity may also appear, depending on your Home Assistant version.
