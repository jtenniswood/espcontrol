---
title: "Appearance Settings"
description:
  Choose the panel theme and customise its active colour.
---

# Appearance

The **Appearance** section is at the top of **Screen**. Its panels are
**Active Colour**, **Card Style** (curve), **Theme**, **Background** (wallpaper, colour and transparency controls), [Idle](/features/idle) and [Clock Bar](/features/clock-bar).

Cards use continuous corners, with the same proportions on the panel and
setup-page preview. Choose **Card curve → Small, Medium or Large** in
**Screen → Appearance**. The choice applies to home and subpage cards, including Back,
is saved on the display and survives restarts.

All supported displays use continuous corners that ease gradually into the
card edges. **Medium** is the default. Corner sizes in pixels are:

| Display | Small | Medium | Large |
| --- | ---: | ---: | ---: |
| 7-inch and 4.3-inch panels | 12 | 16 | 20 |
| 10-inch panels (all versions) | 13 | 18 | 22 |
| P4-86 | 21 | 28 | 35 |
| S3 | 16 | 21 | 26 |

The 7-inch reference uses 12 / 16 / 20 pixels; other displays scale proportionally.
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

Find these controls in **Screen → Appearance** on the [Setup](/features/setup) page. Your selection is saved on the panel and applies without a reboot. Existing panels default to Dark.

## Theme

- **Dark** and **Light** select a fixed appearance.
- Under **Automatic theme**, **Sunrise & Sunset** switches to Light at sunrise and Dark at sunset. The panel shows the calculated sunrise and sunset times when this method is selected. It uses its existing on-device calculation based on the configured timezone and approximate location. Set the correct timezone under **System → Preferences → Time**. A Home Assistant `sun.sun` entity is not required.
- Under **Automatic theme**, **Manual** uses the panel's local clock. Set **Light from** and **Dark from** in 24-hour `HH:MM` format. The Light period can cross midnight. If both times are the same, Dark remains active.

When clock or solar data is temporarily unavailable, Automatic keeps the last active appearance. A fresh boot falls back to Dark until the required data is known. The **Screen: Active Theme** Home Assistant sensor reports the effective Dark or Light palette even while Automatic is selected.

Home Assistant can also change the saved settings through **Screen: Theme Mode**, **Screen: Theme Auto Method**, **Screen: Theme Light Start**, **Screen: Theme Dark Start**, **Screen: Theme Sunrise Offset**, and **Screen: Theme Sunset Offset**. Changes apply without a reboot. The setup-page preview follows the chosen manual theme or, in Automatic, the reported Active Theme.

The theme changes the panel's neutral backgrounds, controls and text, including setup and loading screens. Primary/accent, artwork, camera images, cover art, QR codes and status colours keep their own colours. The clock screensaver keeps its saved text colour in both themes and uses the selected wallpaper or colour, or a black background when no background is set; theme changes do not wake the panel or reset navigation.

Backups include the theme mode, automatic method, times and offsets. Older backups without these fields leave the panel's current theme settings unchanged. Older Schedule and Sun selections migrate to Automatic with the corresponding method.

## Background

Choose **Wallpaper** or **Colour** under **Background**. Wallpaper is the default. Colour uses the same preset swatches and custom colour picker as the other appearance controls. Switching to Colour keeps your uploaded wallpaper, so you can switch back without uploading it again. Your mode and colour are saved on the display and survive restarts.

Use **Upload Image** in the **Wallpaper** tab to upload a JPEG, PNG or WebP background for the physical panel and browser preview. The preview matches your display’s screen ratio and card grid, including rotation. It shows standard and active card fills over the selected background and updates as you adjust transparency. Use **Upload Image** again to replace it, or the trash icon to remove it.

Updated P4 firmware preserves image resolution while starting JPEG compression at 75% quality for smaller files. It supports compressed wallpapers up to 1280 pixels on the longest side and 1,024,000 pixels in total (including 1280 × 800). The panel keeps full-colour image data before drawing to the display. S3 displays use a 480-pixel limit and smoother 16-bit colour conversion to suit their memory. Images are centred to fill the screen; very detailed images may be compressed further or resized to fit storage. Older wallpaper firmware continues to use 480-pixel uploads. Re-upload the original image after updating firmware to benefit from the higher quality. Choose an image with a similar aspect ratio to your display for less cropping.

Use **Standard Card Transparency** and **Active Card Transparency** to choose independently how much of the selected background shows through inactive and active card fills: **0%** gives solid fills and **100%** makes the fills completely clear. Labels and icons keep their normal colours and remain visible. Both controls appear when Colour is selected or a wallpaper is loaded. New setups default to **20%** standard transparency and **2%** active transparency. Existing saved values are retained. Pressed cards use the active setting. Updating from a single transparency setting carries that value over to both controls. Camera images, cover art and QR codes keep their own content. Home and subpage cards use the same saved transparency settings, including after restarting or rebuilding the layout. The selected background appears on home screens, subpages, setup pages and screensavers. Startup and loading screens keep a solid theme background, including firmware-update status screens. The Clock screensaver has its own **Show Wallpaper** toggle; switching it off keeps that screen black. Modal panels keep completely solid theme backgrounds, independent of card transparency; camera and cover-art images appear over the selected background.

Uploads and removal apply immediately. The image and both transparency settings are stored on the display and survive restarts. Both transparency values are remembered when replacing or removing the wallpaper; they apply to both background modes. In Wallpaper mode without an image, both controls are hidden and show **0%**; cards have solid backgrounds. Uploading a wallpaper restores the remembered settings. Background choices, wallpapers and their transparency settings are separate from configuration backups and are cleared by resetting customisation or factory settings. Older firmware with one adjustable transparency value keeps its single **Card transparency** control. Older firmware or partition layouts without image storage show the wallpaper control as unavailable.

Wallpaper reserves one **512 KB slot** in the existing 2 MB image-storage partition. Updating from the earlier two-slot version moves the current wallpaper into this slot and releases the former backup slot. Replacing an image overwrites it: if power is lost or a flash write fails during replacement, the saved wallpaper may be lost and need uploading again. Colour mode stores only a small setting and does not use another image slot; it also works on updated firmware without the image-storage partition. Configuration and panel identity remain in their separate reserved areas.

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
- **Global** uses the active colour selected in **Screen → Appearance**, including when the card has a custom background.
- **Custom** shows a second colour picker for a separate active colour. It starts with Matching’s shade selected and follows the card colour until you choose an alternative. Its presets use the same shades as Matching; the eyedropper still lets you choose any colour.

These choices apply on the main screen and subpages wherever a card normally uses an active colour. Sensor, Door/Window and Presence cards still need their **Active Colour** option enabled to highlight their active state. Resetting colours also restores **Matching**.

Slider cards also use the selected Global or Custom colour for their fill. Text and icons change contrast where the fill passes behind them. Matching keeps its existing shade adjustment for readable slider content.

Disabled Home Assistant cards keep their background colour and show muted labels and icons. Their normal text and icon colours return when the card becomes available again.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. The reset icon in **Active Colour** applies the default orange in the same way.
