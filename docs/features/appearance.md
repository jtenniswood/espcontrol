---
title: "Appearance Settings"
description:
  Choose the panel theme and customise its active colour.
---

# Appearance

The **Appearance** section is at the top of **Settings**. Its panels are
**Active Colour**, **Card Style** (curve), **Theme**, [Idle](/features/idle) and [Clock Bar](/features/clock-bar).

Cards use continuous corners, with the same proportions on the panel and
setup-page preview. Choose **Card curve → Small, Medium or Large** in
**Settings → Appearance**. The choice applies to home and subpage cards, including Back,
is saved on the display and survives restarts.

All supported displays use continuous corners that ease gradually into the
card edges. **Medium** is the default. Corner sizes in pixels are:

| Display | Small | Medium | Large |
| --- | ---: | ---: | ---: |
| 7-inch and 4.3-inch panels | 15 | 20 | 25 |
| 10-inch panels (all versions) | 17 | 22 | 28 |
| P4-86 | 26 | 35 | 44 |
| S3 | 20 | 26 | 33 |

Small uses 60% of Large and Medium uses 80%, rounded to whole pixels.
The same curve applies to the network-status Settings cards, including Back and Screen Lock. Ordinary modal controls and colour swatches keep their existing button radius.
Images, artwork and slider fills follow the same outline. The web preview
uses the same curve, including in browsers without native continuous-corner
support, and scales the card gaps to match the physical display. Selection
outlines stay visible above every card type, including Cover Art. Selected-card
and dashed empty-card outlines follow the chosen curve with even thickness.

On the first boot after installing this update, new and existing displays switch
to Medium once. Choices made afterward are saved across restarts and future updates.

The curve setting is included in configuration backups. Restoring an older
backup without it keeps the current choice. Home Assistant can also change
the **Screen: Card Curve** select. Changes update the preview immediately
and redraw the physical cards after the display's brief refresh delay.

Find these controls in **Settings → Appearance** on the [Setup](/features/setup) page. Your selection is saved on the panel and applies without a reboot. Existing panels default to Dark.

## Theme

- **Dark** and **Light** select a fixed appearance.
- Under **Automatic theme**, **Sunrise & Sunset** switches to Light at sunrise and Dark at sunset. The panel shows the calculated sunrise and sunset times when this method is selected. It uses its existing on-device calculation based on the configured timezone and approximate location. Set the correct timezone under **Device → Preferences → Time**. A Home Assistant `sun.sun` entity is not required.
- Under **Automatic theme**, **Manual** uses the panel's local clock. Set **Light from** and **Dark from** in 24-hour `HH:MM` format. The Light period can cross midnight. If both times are the same, Dark remains active.

When clock or solar data is temporarily unavailable, Automatic keeps the last active appearance. A fresh boot falls back to Dark until the required data is known. The **Screen: Active Theme** Home Assistant sensor reports the effective Dark or Light palette even while Automatic is selected.

Home Assistant can also change the saved settings through **Screen: Theme Mode**, **Screen: Theme Auto Method**, **Screen: Theme Light Start**, **Screen: Theme Dark Start**, **Screen: Theme Sunrise Offset**, and **Screen: Theme Sunset Offset**. Changes apply without a reboot. The setup-page preview follows the chosen manual theme or, in Automatic, the reported Active Theme.

The theme changes the panel's neutral backgrounds, controls and text, including setup and loading screens. Primary/accent, artwork, camera images, cover art, QR codes and status colours keep their own colours. The clock screensaver keeps a black background and its saved text colour in both themes; theme changes do not wake the panel or reset navigation.

Backups include the theme mode, automatic method, times and offsets. Older backups without these fields leave the panel's current theme settings unchanged. Older Schedule and Sun selections migrate to Automatic with the corresponding method.

## Active colour

- **Primary** — the colour cards show when an entity is active. Choose from the same compact grid used in individual card settings. It contains 19 preset colours and a final swatch with an eyedropper icon that opens your browser's native colour picker for any colour, including white. A checkmark shows the selected preset; an accent outline highlights a custom colour. The orange swatch is `FF8C00`, the panel's default active colour.
- Secondary inactive cards and tertiary information cards use fixed panel colours so setup stays simpler and modal styling remains consistent.

The eyedropper swatch has a dashed border while empty and a solid border once a custom colour is chosen. Choosing a preset keeps the custom colour visible in the swatch while selecting the preset. Click the eyedropper again to restore that custom colour and open the picker. This retained colour is kept while editing the current panel; the selected colour is what is saved.

Text and icons keep white on dark and mid-tone backgrounds, including the orange, green, teal and blue presets. Pale backgrounds use dark grey content. The panel and web preview use the same rule. Custom cards use a brighter tint when active for darker colours, or a darker shade for pale colours and white, making on and off easier to distinguish.

Slider cards use a contrasting fill that keeps labels readable as values change.

Striped active cards use lighter or darker stripes to keep text readable across both tones.

Custom card colours stay in place when changing a card’s controls, reopening its editor or switching themes. Media Cover Art uses the chosen card colour while artwork is loading or unavailable; visible artwork keeps white text. Media progress bars use a lighter or darker shade of the chosen card colour so progress remains visible.

Cards using their default background highlight the matching preset or custom picker for the current theme. Resetting a card’s colours restores this default selection.

In a card’s **Custom Colours** controls, **Active state colour** offers three choices:

- **Matching** uses a brighter shade of a darker custom card colour, a darker shade of a pale colour, or the Appearance active colour when the card uses its default background.
- **Global** uses the active colour selected in **Settings → Appearance**, including when the card has a custom background.
- **Custom** shows a second colour picker for a separate active colour. It starts with Matching’s shade selected and follows the card colour until you choose an alternative. Its presets use the same shades as Matching; the eyedropper still lets you choose any colour.

These choices apply on the main screen and subpages wherever a card normally uses an active colour. Sensor, Door/Window and Presence cards still need their **Active Colour** option enabled to highlight their active state. Resetting colours also restores **Matching**.

Slider cards also use the selected Global or Custom colour for their fill. Text and icons change contrast where the fill passes behind them. Matching keeps its existing shade adjustment for readable slider content.

Disabled Home Assistant cards keep their background colour and show muted labels and icons. Their normal text and icon colours return when the card becomes available again.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. The reset icon in **Active Colour** applies the default orange in the same way.
