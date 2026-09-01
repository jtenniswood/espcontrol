# EspControl Companion for macOS

This is a deliberately small macOS 13+ menu-bar app for the Companion card proof of concept. It is not distributed as an installer yet.

For the quickest local test, double-click **Run EspControl Companion.command**. macOS may ask you to confirm that you want to open a downloaded script. Keep its Terminal window open while testing; press Control-C there to stop the app.

You can run the same launcher from Terminal:

```bash
cd macos/Companion
./Run\ EspControl\ Companion.command
```

For Xcode debugging, open `Package.swift`, choose **EspControl Companion**, and click Run. The app can launch only applications you explicitly tick in its settings. Those approved applications can also be selected to open `http://` or `https://` links. The app can replay keyboard shortcuts and fixed window-control presets created in the panel's web editor; macOS Accessibility permission is required the first time either is used. Built-in window tiling requires macOS 15 or later.

Click the EspControl icon in the macOS menu bar to open the app settings. Right-click it for quick access to Connect or Reconnect, Settings, and Quit.
When Companion is installed as a packaged `.app`, its General tab also includes an **Open EspControl Companion at Login** switch. The local Swift launcher does not create an app bundle, so it intentionally omits this setting. macOS may require approval under **System Settings → General → Login Items**.

The **Now Playing** tab shares the active session shown by macOS Control Centre with a paired 4848S040. Sharing is enabled by default and can be disabled at any time. The tab shows the detected application, track, artwork, and a clear diagnostic when the system feed is unavailable. On the panel, choose **Mac Companion** under **Settings → Cover Art Screen Saver → Cover Art Source** to use it.

This generic feed uses the private macOS `MediaRemote` framework, loaded dynamically rather than linked into the app. It is intended for any music, podcast, browser, or video application that publishes usable Now Playing data to macOS. If Apple changes or removes the private symbols, the app reports the feed as unavailable instead of switching to app-specific or web integrations. Companion cards remain operational.

To pair, open the panel's browser editor and go to **Settings → Companion**. Start pairing and copy the details, then use **Paste pairing details** in the Mac app and click **Pair**. Pair on a trusted local network. The app stores the paired credential in the macOS Keychain and pins the panel certificate. Forgetting the panel clears both values.

The protocol is intentionally narrow: a Mac publishes a list of approved bundle identifiers and an optional system Now Playing snapshot, and the panel can request one of those applications, a validated keyboard shortcut, a fixed allow-listed window action, or a web URL opened by one of those applications. Artwork is resized into a black 480×480 JPEG and transferred in acknowledged authenticated chunks. It does not execute shell commands, accept app paths from the panel, allow non-web URL schemes, or accept inbound network connections.
