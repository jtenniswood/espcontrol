---
title: "Theme"
description: Choose a fixed or automatic light/dark theme for your EspControl panel.
---

# Theme

Open **Screen > Appearance > Theme** on your [device setup page](/features/setup).

Choose a mode:

- **Dark** or **Light** for a fixed appearance.
- **Automatic > Sunrise & Sunset** to switch using the panel's calculated solar times. Set the correct [timezone](/features/clock) first; no Home Assistant sun entity is needed.
- **Automatic > Manual** to set **Light from** and **Dark from** in 24-hour `HH:MM` format. The light period can cross midnight; equal times keep Dark active.

Automatic keeps the last appearance while time or solar data is unavailable, and starts in Dark after a fresh boot until that data is known. The clock screensaver keeps its black background and saved text colour.

Changes are saved on the panel and included in [backups](/features/backup). Check the physical screen after making changes.
