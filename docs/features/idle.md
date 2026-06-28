---
title: EspControl Idle Timeout
description:
  How the EspControl panel automatically returns to the home screen after a period of inactivity.
---

# Idle

If you navigate to a [subpage](/features/subpages) or leave a popup open and walk away, the panel can automatically return to the home screen after a set amount of time. This keeps the display on the main card grid so it's ready to use the next time you glance at it.

## Settings

Configured in the **Idle** section of the **Settings** tab in [Setup](/features/setup).

**Return Home After** — how long the panel waits without a touch before switching back to the home screen. Choose from:

- **Disabled** (the default) — stay on whatever screen was last shown
- 10, 20, or 30 seconds
- 1, 2, or 5 minutes

## How It Works

The idle timer restarts every time you touch the screen. If you're already on the home screen with no popup open, nothing changes. If a subpage or popup is active, the timer closes it and returns to the home screen.

The idle timer runs independently from the [screensaver](/features/screensaver). If both are configured, the panel returns to the home screen first (idle), then the screensaver activates after its own timeout.
