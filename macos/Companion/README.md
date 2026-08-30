# EspControl Companion for macOS

This is a deliberately small macOS 13+ menu-bar app for the Companion card proof of concept. It is not distributed as an installer yet.

For the quickest local test, double-click **Run EspControl Companion.command**. macOS may ask you to confirm that you want to open a downloaded script. Keep its Terminal window open while testing; press Control-C there to stop the app.

You can run the same launcher from Terminal:

```bash
cd macos/Companion
./Run\ EspControl\ Companion.command
```

For Xcode debugging, open `Package.swift`, choose **EspControl Companion**, and click Run. The app can only launch applications you explicitly tick in its settings.

To pair, tap the Companion status icon on a 4848S040 panel, start pairing, then enter the eight-letter code in the app. The app stores the paired credential in the macOS Keychain and pins the panel certificate. Forgetting the panel clears both values.

The protocol is intentionally narrow: a Mac publishes a list of approved bundle identifiers, and the panel can send an `INVOKE` for one of those identifiers. It does not execute shell commands, accept app paths from the panel, or accept inbound network connections.
