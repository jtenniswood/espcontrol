# ESPHome captive portal compatibility patch

Imported from ESPHome **2026.9.1**:
https://github.com/esphome/esphome/tree/2026.9.1/esphome/components/captive_portal

Adapted from [Espframe PR #257](https://github.com/jtenniswood/espframe/pull/257).
The setup access point uses IPv4, including when a custom build enables IPv6
for station connections. The upstream DNS
server chooses an IPv6 socket but receives client addresses into `sockaddr_in`.
This patch explicitly creates and binds an IPv4 socket for setup AP DNS. It also
clears counts for omitted authority/additional records when replying to EDNS queries.

`portal.html` was decoded from the gzip array in that version's `captive_index.h`;
its original SHA-256 is
`98ff9b7ed031268f7dd6483d90b7f0eed6e4cf689cdb14fe4eb5a59b22d8ad37`.
The layout now uses EspControl's webserver cards, labels, inputs and buttons.
`python3 scripts/build.py portal` extracts the relevant settings styles from
`src/webserver/application/styles.ts` and embeds them at the template's style
marker. It produces gzip and Brotli versions of `captive_index.h` and `wifi_saved.h`.
The normal all-assets build also generates these headers. Everything
is served locally, with no stylesheet or font download during hotspot setup.
The setup page shows a small Available Networks heading above the network list,
SSID/password fields and Save button.
Device/MAC headings and the OTA upload panel are omitted. The upstream script
retains provisioning behavior, prevents default `href="#"` navigation when
choosing a network, omits updates to the removed headings, and keeps the page title as EspControl WiFi setup. Browser coverage
checks these focused script changes, shared styles, narrow layouts and form behavior.

Page and scan responses use EspControl's no-cache policy. Operating-system
probes receive the standard HTTP 200 portal page; no custom redirects are added.
Credential persistence, scan filtering and the `/config.json`, `/wifisave` and
`/update` endpoints retain ESPHome's behavior. `/wifisave` returns a locally
embedded HTML confirmation with instructions to reconnect to home WiFi and
continue setup on the display. It needs no scripts, redirects or network requests.

Run `python3 scripts/check_tasks.py run-task wifi-setup` for asset parity, real
UDP DNS (with station IPv6 off/on), browser checks at 320/360/640 pixels and the
modeled factory-reset/reprovision/reboot tests. Physical device testing is still
required. Custom AP names/passwords are retained. USB Improv provisioning is
enabled in profiles with setup support; deployed S3 OTA profiles retain their
existing smaller configuration.

C/C++ files use ESPHome's GPLv3 license; Python uses its MIT license (`LICENSE`).
The page originated in esphome/esphome-webserver under MIT (`PORTAL_LICENSE`).
Recheck these patches when upgrading ESPHome and remove the local component
once the corresponding upstream fixes are available.
