# EspControl Companion for macOS

This is a deliberately small macOS 13+ menu-bar app for the Companion card proof of concept. The App Store packaging and submission checklist is in [APP_STORE.md](APP_STORE.md).

For the quickest local test, double-click **Run EspControl Companion.command**. macOS may ask you to confirm that you want to open a downloaded script. Keep its Terminal window open while testing; press Control-C there to stop the app.

You can run the same launcher from Terminal:

```bash
cd macos/Companion
./Run\ EspControl\ Companion.command
```

For Xcode debugging, open `Package.swift`, choose **EspControl Companion**, and click Run. Installed applications are available to launch or to open validated `http://` and `https://` links. Finder folders are separate: add folders with the native picker in the app's **Folders** tab, then select one for each Open folder card in the panel web editor. The app can replay keyboard shortcuts created in the panel's web editor; macOS Accessibility permission is required the first time a shortcut is used.

Click the EspControl icon in the macOS menu bar to open the app settings. Right-click it for quick access to Connect or Reconnect, Settings, and Quit.
When Companion is installed as a packaged `.app`, its General tab also includes an **Open EspControl Companion at Login** switch. The local Swift launcher does not create an app bundle, so it intentionally omits this setting. macOS may require approval under **System Settings → General → Login Items**.

The **Now Playing** tab automatically shares the active session shown by macOS Control Centre with a paired 4848S040. The tab shows the detected application, track, artwork, and a clear diagnostic when the system feed is unavailable. A Companion Play / Pause card displays the state confirmed by the Mac as **Playing**, **Paused**, **Stopped**, or **Unavailable**. It waits for a system notification or the two-second refresh rather than changing the label immediately after a tap. No additional macOS permission is required.

This generic feed uses the private macOS `MediaRemote` framework, loaded dynamically rather than linked into the app. It is intended for any music, podcast, browser, or video application that publishes usable Now Playing data to macOS. If Apple changes or removes the private symbols, the app reports the feed or command as unavailable instead of switching to app-specific or web integrations. Other Companion cards remain operational.

To pair, press and hold the Wi-Fi icon on the physical panel until it displays a code. In the Mac app's **Device** tab, enter the panel address and that code, then click **Pair**. Pair on a trusted local network. The app stores the paired credential in the macOS Keychain and pins the panel certificate. Forgetting the panel clears both values.

After pairing, use the **Applications** tab to approve only the installed apps
that the display may discover, launch, or control. The approved list is stored
locally on the Mac and can be changed at any time.

The **System Stats** tab automatically shares overall processor and memory usage, startup-disk storage, combined network throughput on the primary interface, and battery level when the Mac has a battery. Memory and storage cards can show either used or free capacity. No additional macOS permission is required, and it does not collect application, file, browsing, or network-content details. Choose the corresponding type on a Companion card to show a reading.

The versioned protocol is intentionally narrow: a Mac publishes installed bundle identifiers, opaque identifiers for user-approved folders, supported media controls, a Now Playing snapshot, and system statistics when supported by the connected panel using typed JSON messages. Artwork bytes use bounded binary chunks. The panel can request one of those applications or folders, a validated keyboard shortcut, media control, or a web URL opened by one of those applications. Folder paths stay in the Mac app and are never sent to the display. It does not execute shell commands, accept app or folder paths from the panel, allow non-web URL schemes, or accept inbound network connections.
