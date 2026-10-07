---
title: "Appearance Settings"
description:
  Choose the panel theme and customise its active colour.
---

# Appearance

Find these controls in **Settings → Appearance** on the [Setup](/features/setup) page. Your selection is saved on the panel and applies without a reboot. Existing panels default to Dark.

## Theme

- **Dark** and **Light** select a fixed appearance.
- **Auto → Time** uses the panel's local clock. Set **Light from** and **Dark from** in 24-hour `HH:MM` format. The Light period can cross midnight. If both times are the same, Dark remains active.
- **Auto → Sunrise / Sunset** switches to Light at sunrise and Dark at sunset. Separate sunrise and sunset offsets from −180 to +180 minutes move each transition: negative is before the event, positive is after it. The panel uses its existing on-device sunrise/sunset calculation based on the configured timezone and approximate location. Set the correct timezone under **Settings → Time**. A Home Assistant `sun.sun` entity is not required.

When clock or solar data is temporarily unavailable, Auto keeps the last active appearance. A fresh boot falls back to Dark until the required data is known. The **Screen: Active Theme** Home Assistant sensor reports the effective Dark or Light palette even while Auto is selected.

Home Assistant can also change the same saved settings as the setup page through **Screen: Theme Mode**, **Screen: Theme Auto Method**, **Screen: Theme Light Start**, **Screen: Theme Dark Start**, **Screen: Theme Sunrise Offset**, and **Screen: Theme Sunset Offset**. Changes apply without a reboot. The setup-page preview follows the chosen manual theme or, in Auto, the reported Active Theme.

The theme changes the panel's neutral backgrounds, controls and text, including setup and loading screens. Primary/accent, artwork, camera images, cover art, QR codes and status colours keep their own colours. The clock screensaver keeps a black background and its saved text colour in both themes; theme changes do not wake the panel or reset navigation.

Backups include the theme mode, automatic method, times and offsets. Older backups without these fields leave the panel's current theme settings unchanged. Older Schedule and Sun selections migrate to Auto with the corresponding method.

## Active colour

- **Primary** — the colour cards show when an entity is active. Use the colour picker or type a colour code (for example, `FF8C00` for orange).
- Secondary inactive cards and tertiary information cards use fixed panel colours so setup stays simpler and modal styling remains consistent.

Disabled Home Assistant cards keep their background colour and show muted labels and icons. Their normal text and icon colours return when the card becomes available again.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. **Reset colours** applies the default colour in the same way.
