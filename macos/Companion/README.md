# EspControl Companion for macOS

This is a deliberately small macOS 13+ menu-bar app for the Companion card proof of concept. It is not distributed as an installer yet.

For the quickest local test, double-click **Run EspControl Companion.command**. macOS may ask you to confirm that you want to open a downloaded script. Keep its Terminal window open while testing; press Control-C there to stop the app.

You can run the same launcher from Terminal:

```bash
cd macos/Companion
./Run\ EspControl\ Companion.command
```

For Xcode debugging, open `Package.swift`, choose **EspControl Companion**, and click Run. Installed applications are available to launch or to open validated `http://` and `https://` links. Finder folders are separate: add folders with the native picker in the app's **Folders** tab, then select one for each Open folder card in the panel web editor. The app can replay keyboard shortcuts created in the panel's web editor; macOS Accessibility permission is required the first time a shortcut is used.

Click the EspControl icon in the macOS menu bar to open the app settings. Right-click it for quick access to Connect or Reconnect, Settings, and Quit.
When Companion is installed as a packaged `.app`, its General tab also includes an **Open EspControl Companion at Login** switch. The local Swift launcher does not create an app bundle, so it intentionally omits this setting. macOS may require approval under **System Settings → General → Login Items**.

The **System Stats** tab shares overall processor and memory usage, startup-disk storage, combined network throughput on the primary interface, and battery level when the Mac has a battery. Memory and storage cards can show either used or free capacity. Sharing is enabled by default and can be disabled independently. It does not collect application, file, browsing, or network-content details. Choose the corresponding type on a Companion card to show a reading.

To pair, open the panel's browser editor and go to **Settings → Companion**. Start pairing and copy the details, then use **Paste pairing details** in the Mac app and click **Pair**. Pair on a trusted local network. The app stores the paired credential in the macOS Keychain and pins the panel certificate. Forgetting the panel clears both values.

The protocol is intentionally narrow: a Mac publishes installed bundle identifiers, opaque identifiers for user-approved folders, and optional system statistics. The panel can request one of those applications or folders, a validated keyboard shortcut, media control, or a web URL opened by one of those applications. Folder paths stay in the Mac app and are never sent to the display. When Finder is frontmost, the app reports the approved folder shown in its front window so the matching card can light up; an unrelated or different Finder folder clears that light. macOS may ask for permission for EspControl Companion to automate Finder. It does not execute shell commands, accept app or folder paths from the panel, allow non-web URL schemes, or accept inbound network connections.
