# EspControl Companion for macOS

EspControl Companion is a native macOS 13+ menu-bar app for securely connecting a Mac to an EspControl display. It is intended to be distributed as a standalone Developer ID app outside the Mac App Store. The standalone packaging instructions are below; the legacy App Store packaging notes are in [APP_STORE.md](APP_STORE.md).

For the quickest local test, double-click **Run EspControl Companion.command**. macOS may ask you to confirm that you want to open a downloaded script. Keep its Terminal window open while testing; press Control-C there to stop the app.

You can run the same launcher from Terminal:

```bash
cd macos/Companion
./Run\ EspControl\ Companion.command
```

To build a standalone app bundle for local testing:

```bash
ALLOW_ADHOC=1 ./Packaging/build_standalone.sh
```

The output is `./.build/standalone/EspControl Companion.app`. For distribution, set `CODE_SIGN_IDENTITY` to a Developer ID Application certificate and notarize the resulting app. This bundle is not App Sandbox-restricted, so shortcut and window-control cards can use macOS Accessibility after the user grants permission.

For Xcode debugging, open `Package.swift`, choose **EspControl Companion**, and click Run. Installed applications are available to launch or to open validated `http://` and `https://` links. Finder folders are separate: add folders with the native picker in the app's **Folders** page, then select one for each Open folder card in the panel web editor. The app can replay keyboard shortcuts created in the panel's web editor; macOS Accessibility permission is required the first time a shortcut is used.

On first launch, the setup guide walks through Accessibility for shortcut and window-control cards, optional Mac statistics sharing, and opening Companion at login. Use **General → Run Setup Guide…** to review these choices later.

Click the EspControl icon in the macOS menu bar to see the display address and connection status, connect or disconnect, open display settings in your browser, or open Companion settings. About EspControl Companion is in the application menu.
When Companion is installed as a packaged `.app`, its General page also includes an **Open EspControl Companion at Login** switch. The local Swift launcher does not create an app bundle, so it intentionally omits this setting. macOS may require approval under **System Settings → General → Login Items**.

The app automatically shares the active session shown by macOS Control Centre with a paired 4848S040. A Companion Play / Pause card displays the state confirmed by the Mac as **Playing**, **Paused**, **Stopped**, or **Unavailable**. It waits for a system notification or the two-second refresh rather than changing the label immediately after a tap. No additional macOS permission is required.

This generic feed uses the private macOS `MediaRemote` framework, loaded dynamically rather than linked into the app. It is intended for any music, podcast, browser, or video application that publishes usable Now Playing data to macOS. If Apple changes or removes the private symbols, the app reports the feed or command as unavailable instead of switching to app-specific or web integrations. Other Companion cards remain operational.

To pair, press and hold the Wi-Fi icon on the physical panel until it displays a code. In the Mac app's **Display** page, enter the panel address and that code, then click **Pair Display** (or press Return in the code field). Progress and recovery instructions appear on the same page. Once paired, the page shows the display address, connection status, and **Open Display Settings…**. **Forget Display…** asks for confirmation before clearing the pairing. Pair on a trusted local network. The app stores the paired credential in the macOS Keychain and pins the panel certificate. Forgetting the panel clears both values.

After pairing, use the **Applications** page to approve only the installed apps
that the display may discover, launch, or control. The approved list is stored
locally on the Mac and can be changed at any time. Search by name; the toolbar actions enable or disable only the applications currently shown.

In **Folders**, use **Restore Access…** if macOS needs permission again. Removing a folder asks for confirmation and does not delete any files. In non-App Store builds, **General → Keyboard & Window Controls** shows Accessibility access and opens the relevant System Settings page. Simply viewing Companion settings does not request permission.

Enable **General → Share Mac system statistics** to share overall processor and memory usage, startup-disk storage, combined network throughput on the primary interface, and battery level when the Mac has a battery. Memory and storage cards can show either used or free capacity. No additional macOS permission is required, and it does not collect application, file, browsing, or network-content details. Choose the corresponding type on a Companion card to show a reading.

The versioned protocol is intentionally narrow: a Mac publishes installed bundle identifiers, opaque identifiers for user-approved folders, supported media controls, a Now Playing snapshot, and system statistics when supported by the connected panel using typed JSON messages. Artwork bytes use bounded binary chunks. The panel can request one of those applications or folders, a validated keyboard shortcut, media control, or a web URL opened by one of those applications. Folder paths stay in the Mac app and are never sent to the display. It does not execute shell commands, accept app or folder paths from the panel, allow non-web URL schemes, or accept inbound network connections.
