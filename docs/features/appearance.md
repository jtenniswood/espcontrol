---
title: "Appearance Settings"
description:
  Choose the panel theme and customise its active colour.
---

# Appearance

Find these controls in **Settings → Appearance** on the [Setup](/features/setup) page. Your selection is saved on the panel and applies without a reboot. Existing panels default to Dark.

## Theme

- **Dark** and **Light** select a fixed appearance.
- Under **Automatic theme**, **Time** uses the panel's local clock. Set **Light from** and **Dark from** in 24-hour `HH:MM` format. The Light period can cross midnight. If both times are the same, Dark remains active.
- Under **Automatic theme**, **Automatic** switches to Light at sunrise and Dark at sunset. The panel shows the calculated sunrise and sunset times when this method is selected. It uses its existing on-device calculation based on the configured timezone and approximate location. Set the correct timezone under **Settings → Time**. A Home Assistant `sun.sun` entity is not required.

When clock or solar data is temporarily unavailable, Automatic keeps the last active appearance. A fresh boot falls back to Dark until the required data is known. The **Screen: Active Theme** Home Assistant sensor reports the effective Dark or Light palette even while Automatic is selected.

Home Assistant can also change the saved settings through **Screen: Theme Mode**, **Screen: Theme Auto Method**, **Screen: Theme Light Start**, **Screen: Theme Dark Start**, **Screen: Theme Sunrise Offset**, and **Screen: Theme Sunset Offset**. Changes apply without a reboot. The setup-page preview follows the chosen manual theme or, in Automatic, the reported Active Theme.

The theme changes the panel's neutral backgrounds, controls and text, including setup and loading screens. Primary/accent, artwork, camera images, cover art, QR codes and status colours keep their own colours. The clock screensaver keeps a black background and its saved text colour in both themes; theme changes do not wake the panel or reset navigation.

Backups include the theme mode, automatic method, times and offsets. Older backups without these fields leave the panel's current theme settings unchanged. Older Schedule and Sun selections migrate to Automatic with the corresponding method.

## Active colour

- **Primary** — the colour cards show when an entity is active. Choose from the same compact grid of 20 round swatches used in individual card settings. A checkmark shows the selected colour. The orange swatch is `FF8C00`, the panel's default active colour.
- Secondary inactive cards and tertiary information cards use fixed panel colours so setup stays simpler and modal styling remains consistent.

Text and icons stay white on mid-tone backgrounds, including orange, blue, green and teal. Cyan also keeps white content in its lighter active state. Pale backgrounds use dark content, with the same rules on the panel and web preview.

Custom card colours stay in place when changing a card’s controls, reopening its editor or switching themes. Media Cover Art uses the chosen card colour while artwork is loading or unavailable; visible artwork keeps white text.

Disabled Home Assistant cards keep their background colour and show muted labels and icons. Their normal text and icon colours return when the card becomes available again.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. The reset icon beside the **Appearance** arrow applies the default orange in the same way.
