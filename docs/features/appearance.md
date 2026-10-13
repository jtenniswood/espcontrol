---
title: "Appearance Settings"
description:
  Choose the panel theme and customise its active colour.
---

# Appearance

Cards use larger rounded corners, with the same proportions on the panel and
setup-page preview. Corner sizes scale for each display and are fixed by the
firmware; there is no corner-radius setting.

The 10-inch JC8012P4A1 V3 and 4-inch P4-86 use continuous corners that ease
gradually into the card edges, with nominal sizes of 22 px and 35 px respectively.
Images, artwork and slider fills follow the same outline. The web preview
matches this shape in browsers that support continuous corners; older
browsers show ordinary rounded corners.

Find these controls in **Settings → Appearance** on the [Setup](/features/setup) page. Your selection is saved on the panel and applies without a reboot. Existing panels default to Dark.

## Theme

- **Dark** and **Light** select a fixed appearance.
- Under **Automatic theme**, **Time** uses the panel's local clock. Set **Light from** and **Dark from** in 24-hour `HH:MM` format. The Light period can cross midnight. If both times are the same, Dark remains active.
- Under **Automatic theme**, **Automatic** switches to Light at sunrise and Dark at sunset. The panel shows the calculated sunrise and sunset times when this method is selected. It uses its existing on-device calculation based on the configured timezone and approximate location. Set the correct timezone under **Settings → Time**. A Home Assistant `sun.sun` entity is not required.

When clock or solar data is temporarily unavailable, Automatic keeps the last active appearance. A fresh boot falls back to Dark until the required data is known. The **Screen: Active Theme** Home Assistant sensor reports the effective Dark or Light palette even while Automatic is selected.

Home Assistant can also change the saved settings through **Screen: Theme Mode**, **Screen: Theme Auto Method**, **Screen: Theme Light Start**, **Screen: Theme Dark Start**, **Screen: Theme Sunrise Offset**, and **Screen: Theme Sunset Offset**. Changes apply without a reboot. The setup-page preview follows the chosen manual theme or, in Automatic, the reported Active Theme.

The theme changes the panel's neutral backgrounds, controls and text, including setup and loading screens. Primary/accent, artwork, camera images, cover art, QR codes and status colours keep their own colours. The clock screensaver keeps a black background and its saved text colour in both themes; theme changes do not wake the panel or reset navigation.

Backups include the theme mode, automatic method, times and offsets. Older backups without these fields leave the panel's current theme settings unchanged. Older Schedule and Sun selections migrate to Automatic with the corresponding method.

## Wallpaper

Upload a JPEG, PNG or WebP image under **Wallpaper** to place it behind the cards on the physical panel and in the browser preview. Images are resized to a maximum of 480 pixels on the longest side and centred to fill the screen. Choose an image with a similar aspect ratio to your display for less cropping.

Use **Card transparency** to choose how much wallpaper shows through the cards: **0%** gives solid fills and **100%** makes the fills completely clear. Labels and icons keep their normal colours and remain visible. The default is **20%** transparency (an 80% fill). Camera images, cover art and QR codes keep their own content. The wallpaper also appears on subpages; setup screens, modals and screensavers keep their existing backgrounds.

Uploads and **Remove wallpaper** apply immediately. The image and transparency setting are stored on the display and survive restarts. Transparency is remembered when replacing or removing the wallpaper; it applies when a wallpaper is present. Removing it restores solid card backgrounds. Wallpapers and their transparency setting are separate from configuration backups and are cleared by resetting customisation or factory settings. Older firmware or partition layouts without image storage show the wallpaper control as unavailable.

## Active colour

- **Primary** — the colour cards show when an entity is active. Choose from the same compact grid used in individual card settings. It contains 19 preset colours and a final transparent, dashed swatch with an eyedropper icon that opens your browser's native colour picker for any colour, including white. A checkmark shows the selected preset; an accent outline highlights a custom colour. The orange swatch is `FF8C00`, the panel's default active colour.
- Secondary inactive cards and tertiary information cards use fixed panel colours so setup stays simpler and modal styling remains consistent.

Text and icons stay white on mid-tone backgrounds, including orange, blue, green and teal. Cyan also keeps white content in its lighter active state. Pale backgrounds use dark content, with the same rules on the panel and web preview.

Custom card colours stay in place when changing a card’s controls, reopening its editor or switching themes. Media Cover Art uses the chosen card colour while artwork is loading or unavailable; visible artwork keeps white text. Media progress bars use a lighter or darker shade of the chosen card colour so progress remains visible.

Cards using their default background highlight the matching preset or custom picker for the current theme. Resetting a card’s colours restores this default selection.

Disabled Home Assistant cards keep their background colour and show muted labels and icons. Their normal text and icon colours return when the card becomes available again.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. The reset icon beside the **Appearance** arrow applies the default orange in the same way.
