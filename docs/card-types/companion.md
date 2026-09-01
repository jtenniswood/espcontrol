---
title: Companion Cards
description: Launch macOS applications, open Finder folders, control media, open web links, or replay keyboard shortcuts from a 4848S040 EspControl panel.
---

# Companion Cards

Companion cards are a proof-of-concept card type for the **4-inch 4848S040** panel. They let you launch an application, open an approved Finder folder, control Mac media playback, open a web address with an installed application, or replay a saved keyboard shortcut on one paired Mac. They do not run shell commands or expose your Mac to incoming network connections.

## Before adding cards

1. Flash the Companion Cards test firmware to a 4848S040.
2. On the Mac, open the `EspControl Companion` project in Xcode, choose your Personal Team for signing, and run the menu-bar app.
3. Open the panel's browser editor, then go to **Settings → Companion** and choose **Start pairing**.
4. Choose **Copy pairing details**. In the Mac app settings, choose **Paste pairing details**, then **Pair**. Pair on a trusted local network.

The Mac stores the pairing credential in Keychain and pins the panel's locally generated certificate. If you forget the panel from the Mac app, pair it again before Companion cards will work.
When the authenticated Mac is connected, a monitor icon appears beside Wi-Fi in the panel's clock bar. It disappears within a moment if the connection ends.

## Add a Companion card

Use the normal browser layout editor and select an empty home-screen or subpage slot, then choose **Companion**. Under **Action**, choose one of:

- **Launch app** — select an installed Mac application. Finder is not shown as an application because folders use their own action.
- **Keyboard shortcut** — click the shortcut field and press a combination such as Command-A. The browser records and displays the combination on the card.
- **Open URL** — enter an `http://` or `https://` address and choose the approved installed application that should open it, such as Safari or Chrome.
- **Open folder** — first add one or more folders from the Mac app's **Folders** tab, then choose the folder for this card. The display receives an anonymous identifier and friendly name; the filesystem path remains on the Mac.
- **Media control** — choose Play / Pause, Previous Track, or Next Track for the Mac's current Now Playing application.

Use a [Slider card](/card-types/sliders) when you want to control the Mac's output or input volume.

The first time a shortcut is used, macOS asks for Accessibility permission so the Companion app can replay keyboard input. Allow **EspControl Companion** in **System Settings → Privacy & Security → Accessibility**, then press the card again. Shortcuts are sent to whichever Mac application is active at that time.

The card is disabled when the Mac is offline, when an app or URL card references an unavailable application, when a folder has been removed from the Mac app, or when a URL is incomplete. Layouts, subpages, backup, and restore work through the same built-in editor as all other cards.

## Limits in this proof of concept

- One Mac can be paired to one panel at a time.
- Keyboard shortcuts require Command, Control, or Option plus a supported key. Modifier-only and unsupported system keys are rejected.
- URL cards accept only `http://` and `https://` addresses without embedded usernames or passwords.
- Media buttons control the application currently registered with macOS Now Playing. Support depends on that application's media integration.
- Companion is only offered on the 4848S040 profile. Other panels continue to behave normally.

If a pairing needs to be replaced, forget it in the Mac app and start a new pairing session from the panel's web settings.
