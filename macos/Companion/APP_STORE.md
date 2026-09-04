# Mac App Store preparation

The Companion is built from Swift Package Manager sources, but the Mac App
Store requires a self-contained signed `.app` bundle. The packaging script
creates that bundle with App Sandbox, security-scoped folder access, the
privacy manifest, export-compliance metadata, and an App Store icon.

## Local verification

Run this on macOS from the repository root:

```bash
ALLOW_ADHOC=1 macos/Companion/Packaging/build_app_store.sh
```

This produces a locally signed app at
`macos/Companion/.build/app-store/EspControl Companion.app`. Ad-hoc signing is
only for local inspection and cannot be uploaded to App Store Connect. macOS
privacy approvals are tied to the signed build, so Accessibility may need to be
removed and re-enabled after replacing an ad-hoc app. Use the same stable
`CODE_SIGN_IDENTITY` for installed test builds when the approval must survive
updates.

## App Store build

Create a Mac App Store distribution certificate and provisioning profile in
the Apple Developer account, then run:

```bash
CODE_SIGN_IDENTITY="3rd Party Mac Developer Application: Your Name (TEAMID)" \
INSTALLER_IDENTITY="3rd Party Mac Developer Installer: Your Name (TEAMID)" \
PROVISIONING_PROFILE="/path/to/EspControl_Companion.provisionprofile" \
MARKETING_VERSION="1.0.0" \
CURRENT_PROJECT_VERSION="1" \
CREATE_PKG=1 \
macos/Companion/Packaging/build_app_store.sh
```

Upload the resulting `.pkg` with Transporter or Xcode, then complete the
App Store Connect product page, pricing, screenshots, age rating, privacy
answers, support URL, and review notes. The review notes should explain that
the app pairs with an EspControl panel on the same local network and that the
Accessibility permission is optional and only enables keyboard-shortcut cards.
Attach a short video showing physical pairing and the display receiving an
action, because App Review cannot exercise the core workflow without EspControl
hardware. Also explain that system statistics stay off until the user enables
them in General settings, and that optional Finder automation only identifies
an approved folder shown in the frontmost Finder window.

The App Store build intentionally omits the private macOS `MediaRemote`
bridge. Apple requires Mac App Store apps to use public APIs, so Now Playing
and Play/Pause/Previous/Next Companion actions are unavailable in the store
build. CoreAudio volume controls, pairing, folder cards, app cards, URLs,
keyboard shortcuts, and optional system statistics remain in the build.

## Non-code prerequisites

The repository is currently licensed under the PolyForm Noncommercial License.
Selling this app is commercial use and therefore needs a separate commercial
license or an explicit license change from the copyright holder before release.
