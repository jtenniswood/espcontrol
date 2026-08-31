# EspControl Companion for macOS

This is a deliberately small macOS 13+ menu-bar app for the Companion card proof of concept. It is not distributed as an installer yet.

For the quickest local test, double-click **Run EspControl Companion.command**. macOS may ask you to confirm that you want to open a downloaded script. Keep its Terminal window open while testing; press Control-C there to stop the app.

You can run the same launcher from Terminal:

```bash
cd macos/Companion
./Run\ EspControl\ Companion.command
```

For Xcode debugging, open `Package.swift`, choose **EspControl Companion**, and click Run. The app can launch only applications you explicitly tick in its settings. Those approved applications can also be selected to open `http://` or `https://` links. The app can replay keyboard shortcuts created in the panel's web editor; macOS Accessibility permission is required the first time a shortcut is used.

To pair, open the panel's browser editor and go to **Settings → Companion**. Start pairing and copy the details, then use **Paste pairing details** in the Mac app and click **Pair**. Pair on a trusted local network. The app stores the paired credential in the macOS Keychain and pins the panel certificate. Forgetting the panel clears both values.

The protocol is intentionally narrow: a Mac publishes a list of approved bundle identifiers, and the panel can request one of those applications, a validated keyboard shortcut, or a web URL opened by one of those applications. It does not execute shell commands, accept app paths from the panel, allow non-web URL schemes, or accept inbound network connections.
