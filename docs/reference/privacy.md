---
title: Privacy Policy
description: How EspControl Companion handles pairing information, selected folders, and optional Mac system statistics.
---

# Privacy Policy

EspControl Companion is a local-network menu-bar app for connecting a Mac to
an EspControl touchscreen.

## Information stored on your Mac

The app stores the panel address and connection preferences in the app's local
preferences. The paired credential is stored in the macOS Keychain. Folders
you choose for Companion cards are represented by security-scoped macOS
bookmarks so the app can reopen them after a restart. Folder paths remain on
your Mac and are not sent to the display.

## Information sent to your display

Only after you enable **Share Mac system statistics**, the app sends the paired display processor, memory,
storage, network-throughput, and battery percentages. It can also send the
names and opaque identifiers of installed applications and user-approved
folders so the display can present Companion cards. These messages stay on
your local network and are sent only to the panel you pair.

The app does not collect analytics, advertising identifiers, browsing history,
file contents, application contents, or network-content data. It does not
send this information to EspControl or an analytics provider.

Starting a new pairing requires physical access to the touchscreen. The pairing
code is shown only on the display and is not returned by its browser API.

## Permissions

Folder access is granted only when you choose a folder in the app. Finder
automation is requested only when an approved folder needs to be matched to the
frontmost Finder window. Accessibility access is optional and is used only to
replay a keyboard shortcut requested by a Companion card. You can remove these
permissions in macOS System Settings.

The Mac App Store build does not request or use Finder automation or
Accessibility. It omits Finder front-window auto-detection, keyboard shortcuts,
and window controls; these capabilities are available only in the direct build.

## Deletion and support

Use **Forget this panel** to remove the panel credential and certificate data.
Remove folders from the Folders tab to remove their bookmarks. Uninstalling
the app removes its local preferences; Keychain items can be removed by
forgetting the panel before uninstalling.

For questions or deletion requests, contact the project owner through the
[EspControl support page](https://github.com/jtenniswood/espcontrol/issues).
