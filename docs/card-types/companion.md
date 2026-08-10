---
title: Companion Cards
description: Launch approved macOS applications from a 4848S040 EspControl panel.
---

# Companion Cards

Companion cards are a proof-of-concept card type for the **4-inch 4848S040** panel. They let you launch a small, approved list of applications on one paired Mac. They do not run shell commands or expose your Mac to incoming network connections.

## Before adding cards

1. Flash the Companion Cards test firmware to a 4848S040.
2. On the Mac, open the `EspControl Companion` project in Xcode, choose your Personal Team for signing, and run the menu-bar app.
3. On the display, tap the laptop icon in the top bar. It shows a short-lived code in the form `ABCD-EFGH`.
4. Enter the panel address and code in the Mac app, then choose which installed apps it may launch.

The Mac stores the pairing credential in Keychain and pins the panel's locally generated certificate. If you forget the panel from the Mac app, pair it again before Companion cards will work.

## Add a Companion card

Use the normal browser layout editor: select an empty home-screen or subpage slot, choose **Companion**, then choose one of the approved Mac apps from **Mac App**. Give it a label and icon like any other card.

The card is disabled when the Mac is offline or when that app has been removed from the approved list. Layouts, subpages, backup, and restore work through the same built-in editor as all other cards; no separate Mac layout editor is needed.

## Limits in this proof of concept

- One Mac can be paired to one panel at a time.
- The first version only launches apps. It does not control playback, windows, files, or arbitrary commands.
- Companion is only offered on the 4848S040 profile. Other panels continue to behave normally.

If a pairing needs to be replaced, forget it in the Mac app and use the physical panel to start a new pairing window.
