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

- **Primary** — the colour cards show when an entity is active. Choose from the same compact grid used in individual card settings. It contains 19 preset colours and a final swatch with an eyedropper icon that opens your browser's native colour picker for any colour, including white. The custom swatch is transparent and dashed until a custom colour is chosen, then shows that colour without the dashed border. Choosing a preset keeps the last custom colour available while editing; clicking the custom swatch restores it. A checkmark shows the selected preset; an accent outline highlights a custom colour. The orange swatch is `FF8C00`, the panel's default active colour.
- Secondary inactive cards and tertiary information cards use fixed panel colours so setup stays simpler and modal styling remains consistent.

Text and icons keep white on dark and mid-tone backgrounds, including the orange, green, teal and blue presets. Pale backgrounds use dark grey content. The panel and web preview use the same rule. Custom cards use a brighter tint when active for darker colours, or a darker shade for pale colours and white, making on and off easier to distinguish.

Slider cards use a contrasting fill that keeps labels readable as values change.

Striped active cards use lighter or darker stripes to keep text readable across both tones.

Custom card colours stay in place when changing a card’s controls, reopening its editor or switching themes. Media Cover Art uses the chosen card colour while artwork is loading or unavailable; visible artwork keeps white text. Media progress bars use a lighter or darker shade of the chosen card colour so progress remains visible.

Cards using their default background highlight the matching preset or custom picker for the current theme. Resetting a card’s colours restores this default selection.

In a card’s **Custom Colours** controls, **Active state colour** offers three choices:

- **Matching** uses a brighter shade of a darker custom card colour, a darker shade of a pale colour, or the Appearance active colour when the card uses its default background.
- **Theme** uses the active colour selected in **Settings → Appearance**, including when the card has a custom background.
- **Custom** shows a second colour picker for a separate active colour. It starts with Matching’s shade selected and follows the card colour until you choose an alternative. Its presets use the same shades as Matching; the eyedropper still lets you choose any colour.

These choices apply on the main screen and subpages wherever a card normally uses an active colour. Sensor, Door/Window and Presence cards still need their **Active Colour** option enabled to highlight their active state. Resetting colours also restores **Matching**.

Slider cards also use the selected Theme or Custom colour for their fill. Text and icons change contrast where the fill passes behind them. Matching keeps its existing shade adjustment for readable slider content.

Disabled Home Assistant cards keep their background colour and show muted labels and icons. Their normal text and icon colours return when the card becomes available again.

Uploaded card backgrounds stay fully opaque, including when a card is pressed, active or unavailable. The web preview also shows these images at full opacity.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. The reset icon beside the **Appearance** arrow applies the default orange in the same way.
