---
title: "Appearance Settings"
description: Configure EspControl active colour, card style, theme, idle timeout, and clock bar in one guide.
---

# Appearance

Open **Screen > Appearance** on your [device setup page](/features/setup). Use the sections below to change your cards, choose a theme, return home after inactivity, or configure the top bar.

Changes are saved on the panel and included in [backups](/features/backup). Check the physical screen after making changes.

For one card's label or icon, use [Layout settings](/settings/layout). For brightness or orientation, open [Display](/settings/display).

## Active Colour

Open **Screen > Appearance > Active Colour** on your [device setup page](/features/setup).

Choose a preset under **Active Colour**, or use the eyedropper swatch for a custom colour. This is the highlight used when a card is active. The reset icon restores the default orange. Text and icons adjust their contrast automatically.

To change a single card, right-click it in **Layout** and open **Change Colour**, or edit its **Custom Colours**. You can also select several cards together or recolour a subpage's **Back** card. **Reset Colour** restores its default background.

In a card's **Custom Colours**, choose its **Active state colour**:

| Choice | Result |
|---|---|
| **Matching** | Uses a lighter or darker shade of the card colour; default cards use the global active colour. |
| **Global** | Uses the colour from **Screen > Appearance > Active Colour**. |
| **Custom** | Lets you pick a separate active colour. |

Sensor, Door/Window, and Presence cards also need **Active Colour** enabled in their card settings. Artwork, full QR images, and status colours keep their own appearance.

## Card Style

Open **Screen > Appearance > Card Style** on your [device setup page](/features/setup).

Choose **Card curve > Small**, **Medium**, or **Large**. Medium is the default. This changes the corners of home-screen and subpage cards, including **Back**, and the preview follows the panel.

## Theme

Open **Screen > Appearance > Theme** on your [device setup page](/features/setup).

Choose a mode:

- **Dark** or **Light** for a fixed appearance.
- **Automatic > Sunrise & Sunset** to switch using the panel's calculated solar times. Set the correct [timezone](/features/clock) first; no Home Assistant sun entity is needed.
- **Automatic > Manual** to set **Light from** and **Dark from** in 24-hour `HH:MM` format. The light period can cross midnight; equal times keep Dark active.

Automatic keeps the last appearance while time or solar data is unavailable, and starts in Dark after a fresh boot until that data is known. The clock screensaver keeps its black background and saved text colour.

## Idle

If you navigate to a [subpage](/features/subpages) or leave a popup open and walk away, the panel can automatically return to the home screen after a set amount of time. This keeps the display on the main card grid so it's ready to use the next time you glance at it.

### Idle Settings

Configured in **Screen > Appearance > Idle** in [Setup](/features/setup).

**Return Home After** — how long the panel waits without a touch before switching back to the home screen. Choose from:

- **Disabled** — stay on whatever screen was last shown
- 10, 20, or 30 seconds
- **1 minute** (the default), 2 minutes, or 5 minutes

### How Idle Works

The idle timer restarts every time you touch the screen. If you're already on the home screen with no popup open, nothing changes. If a subpage or popup is active, the timer closes it and returns to the home screen.

The idle timer runs independently from the [screensaver](/features/screensaver). Each uses its own timeout; the panel only returns home first if the idle timeout is shorter.

## Clock Bar

The top bar has fixed positions: temperature on the left, time in the middle, and connectivity on the right.

### Clock Bar Settings

Open **Screen > Appearance > Clock Bar**:

- **Show Clock Bar** turns the whole bar on or off.
- **Show Night Mode Icon** adds a moon while [Night Schedule](/features/screen-schedule) is active. Off by default.

To change individual items, open **Layout** and select them in the preview:

- **Temperature > Edit** selects the Home Assistant sensor and degree-symbol display.
- **Clock** or **Connectivity > Hide / Show** changes visibility.

Hidden items stay greyed in the preview so you can show them again. Only one temperature reading is displayed. Set [time and timezone](/features/clock) and [temperature units](/features/temperature) under **System > Preferences**.

### Open Device Information

Tap the connectivity icon on the physical panel to open **Settings**. It shows the IP address, Wi-Fi quality on Wi-Fi builds, firmware build, and device name. Tap **Back** to return to your previous page.

If a PIN is saved, Settings also offers **Screen Lock**, even with automatic wake protection off. Tap it to lock the current view, then touch the screen and enter the PIN to unlock. See [PIN protection](/features/screensaver#pin-protection-after-wake).

When a subpage or card popup is open, its title temporarily replaces the temperature reading.
