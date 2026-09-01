---
title: Companion Cards
description: Launch macOS applications, control media, open web links, or replay keyboard shortcuts from a 4848S040 EspControl panel.
---

# Companion Cards

Companion cards are a proof-of-concept card type for the **4-inch 4848S040** panel. They let you launch an application, control Mac media playback, open a web address with an installed application, or replay a saved keyboard shortcut on one paired Mac. They do not run shell commands or expose your Mac to incoming network connections.

## Before adding cards

1. Flash the Companion Cards test firmware to a 4848S040.
2. On the Mac, open the `EspControl Companion` project in Xcode, choose your Personal Team for signing, and run the menu-bar app.
3. Open the panel's browser editor, then go to **Settings → Companion** and choose **Start pairing**.
4. Choose **Copy pairing details**. In the Mac app settings, choose **Paste pairing details**, then **Pair**. The app checks the Verify code against the panel certificate before it sends the pairing code. Pair on a trusted local network, then choose which installed apps it may launch.

The Mac stores the pairing credential in Keychain and pins the panel's locally generated certificate. If you forget the panel from the Mac app, pair it again before Companion cards will work.
When the authenticated Mac is connected, a monitor icon appears beside Wi-Fi in the panel's clock bar. It disappears within a moment if the connection ends.

## Share what is playing on the Mac

The Companion app can also share the active macOS **Now Playing** session, including its title, artist, album, progress, playback state, source application, and cover art. This works without Home Assistant and is enabled by default in the Mac app's **Now Playing** tab. Turn the switch off there if you do not want the Mac to share this information.

On the 4848S040 browser editor, open **Settings → Cover Art Screen Saver** and set **Cover Art Source** to **Mac Companion**. Home Assistant entity, external-source, and filtering controls are hidden in this mode because one complete source supplies the metadata and artwork together. The existing delay and track-overlay duration settings still apply. Change the source back to **Home Assistant** to use the existing Home Assistant behavior; the panel never mixes the two sources or falls back automatically.

Only actively playing media opens cover art automatically. Pausing or stopping follows the normal cover-art closing behavior. A short Companion network interruption gets a five-second grace period; after that the panel hides the presentation while keeping its decoded image cache until a fresh matching update arrives.

Companion reads the session macOS shows in Control Centre. This can include Apple Music, Spotify, podcast apps, browsers, and video apps when they publish usable system metadata. An application that does not appear correctly in macOS Control Centre is not supported.

## Add a Companion card

Use the normal browser layout editor and select an empty home-screen or subpage slot, then choose **Companion**. Under **Action**, choose one of:

- **Launch app** — select one of the applications approved in the Mac app.
- **Keyboard shortcut** — click the shortcut field and press a combination such as Command-A. The browser records and displays the combination on the card.
- **Open URL** — enter an `http://` or `https://` address and choose the approved installed application that should open it, such as Safari or Chrome.
- **Media control** — choose Play / Pause, Previous Track, or Next Track for the Mac's current Now Playing application.

Use a [Slider card](/card-types/sliders) when you want to control the Mac's output or input volume.

### Add Safari shortcuts

For a **Launch app** card that uses Safari, turn on **Add shortcut folder**. The card will bring Safari to the front and then open a folder on the panel containing Back, Forward, Reload, New Tab, and Close Tab. These controls use Safari's standard keyboard shortcuts.

The folder is created with those five controls once. You can then edit their labels, icons, shortcuts, and order, or delete and add keyboard-shortcut cards. Turning the folder option off does not discard those edits; turning it back on restores the same folder. Safari must remain approved in the Companion app.

The first time you use one of these controls, macOS may ask for Accessibility permission. Allow **EspControl Companion** in **System Settings → Privacy & Security → Accessibility**. If Safari is no longer approved or the Companion is offline, the Safari card is disabled and the folder is not opened.

The first time a shortcut is used, macOS asks for Accessibility permission so the Companion app can replay keyboard input. Allow **EspControl Companion** in **System Settings → Privacy & Security → Accessibility**, then press the card again. Shortcuts are sent to whichever Mac application is active at that time.

The card is disabled when the Mac is offline, when an app or URL card references an application that has been removed from the approved list, or when a URL is incomplete. Layouts, subpages, backup, and restore work through the same built-in editor as all other cards; no separate Mac layout editor is needed.

## Limits in this proof of concept

- One Mac can be paired to one panel at a time.
- Keyboard shortcuts require Command, Control, or Option plus a supported key. Modifier-only and unsupported system keys are rejected.
- URL cards accept only `http://` and `https://` addresses without embedded usernames or passwords.
- Reading other applications' Now Playing data uses macOS's private `MediaRemote` framework because Apple's public API only lets an application publish its own session. The framework is loaded dynamically. If a macOS update removes the required symbols, Companion reports the system feed as unavailable and its existing cards continue to work.
- Media buttons control the application currently registered with macOS Now Playing. Support depends on that application's media integration.
- Companion is only offered on the 4848S040 profile. Other panels continue to behave normally.

If a pairing needs to be replaced, forget it in the Mac app and start a new pairing session from the panel's web settings.
