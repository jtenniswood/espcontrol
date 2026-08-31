---
title: Companion Cards
description: Launch approved macOS applications from a 4848S040 EspControl panel.
---

# Companion Cards

Companion cards are a proof-of-concept card type for the **4-inch 4848S040** panel. They let you launch a small, approved list of applications or replay a saved keyboard shortcut on one paired Mac. They do not run shell commands or expose your Mac to incoming network connections.

## Before adding cards

1. Flash the Companion Cards test firmware to a 4848S040.
2. On the Mac, open the `EspControl Companion` project in Xcode, choose your Personal Team for signing, and run the menu-bar app.
3. Open the panel's browser editor, then go to **Settings → Companion** and choose **Start pairing**.
4. Choose **Copy pairing details**. In the Mac app settings, choose **Paste pairing details**, then **Pair**. The app checks the Verify code against the panel certificate before it sends the pairing code. Pair on a trusted local network, then choose which installed apps it may launch.

The Mac stores the pairing credential in Keychain and pins the panel's locally generated certificate. If you forget the panel from the Mac app, pair it again before Companion cards will work.

## Add a Companion card

Use the normal browser layout editor and select an empty home-screen or subpage slot, then choose **Companion**. Under **Action**, choose one of:

- **Launch app** — select one of the applications approved in the Mac app.
- **Keyboard shortcut** — click the shortcut field and press a combination such as Command-A. The browser records and displays the combination on the card.

The first time a shortcut is used, macOS asks for Accessibility permission so the Companion app can replay keyboard input. Allow **EspControl Companion** in **System Settings → Privacy & Security → Accessibility**, then press the card again. Shortcuts are sent to whichever Mac application is active at that time.

The card is disabled when the Mac is offline, or when an app-launch card references an application that has been removed from the approved list. Layouts, subpages, backup, and restore work through the same built-in editor as all other cards; no separate Mac layout editor is needed.

## Limits in this proof of concept

- One Mac can be paired to one panel at a time.
- Keyboard shortcuts require Command, Control, or Option plus a supported key. Modifier-only and unsupported system keys are rejected.
- Companion does not control playback, windows, files, or arbitrary commands directly.
- Companion is only offered on the 4848S040 profile. Other panels continue to behave normally.

If a pairing needs to be replaced, forget it in the Mac app and start a new pairing session from the panel's web settings.
