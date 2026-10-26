# Demanded network Settings

This #1825 prerequisite moves the clock-bar Settings panel to the shared modal
runtime. It builds on [local modal connection policies](pooled-wifi-qr-modal.md).
Production still uses the existing renderer. Application-scene integration,
compiled legacy retirement and physical qualification remain; no firmware was
flashed and no physical memory saving is claimed.

`PageLocalModalOrigin` carries the configuration generation, source page and a
nonzero scene-issued chrome epoch captured by the clock-bar press. It never
manufactures a card binding or uses card zero. `request_local` admits only a
family explicitly supported by the port and a current navigable source page.
The press queues scalar work without allocation, sampling or widget creation.
Settings uses a stable local connection epoch, so HA loss does not close it.

An admitted Settings request retains its original source while its own previous
overlay retires, rather than substituting a new chrome epoch. The scene must
cancel queued/admitted local requests through `dismiss`/`display_takeover` on
external navigation, mode changes and shutdown, and issue a new chrome epoch.
Configuration replacement cancels them automatically. Reentrant takeover or
shutdown during metadata creation skips initialization and retires that exact
context, including when the active configuration generation is zero.

`PageNetworkStatusModalPort` owns one demanded context, at most six cards with
only an icon and label each, an eight-entry gesture reservation and two grid
tracks sized to the visible profile (at least six entries). Ethernet omits the
Wi-Fi card; Lock is last and hidden without a PIN. Rows retain the configured
minimum and expand for visible settings cards. Full device/build identity is
copied, and geometry/fonts resolve using the captured source page and full
opening token. The shared card factory preserves corner policy and label wrap.
The panel uses neutral theme colors and the underlying page's clock-bar inset,
padding and gaps. Back/Lock are the only clickable cards.

The application-owned `PageLocalNetworkStatusSource` reads ESPHome's cached IP
and optional Wi-Fi quality in main-loop maintenance at one-second intervals.
There is no panel timer, HA registration, hidden configured-card context or
network transfer. IP absence, disconnected Wi-Fi, rounded/clamped percentage,
version/dev/unknown labels and translation match the existing panel. The pure
version/text helpers are shared with the live legacy implementation. Unchanged
maintenance does not allocate or redraw. Existing demanded objects follow font,
geometry, PIN and theme changes. Lock queues an opening-scoped intent; it
rechecks the current PIN in maintenance before activating the local lock.
Closing, takeover and replacement cancel that intent without activating it.

Retirement revokes gestures and value ownership immediately. Widget hiding and
deletion wait until native/LVGL/coordinator dispatch ends. The scene's Settings
caption callback changes scalar mode only; it recomputes the current page title
without saving an old page's label or retaining configuration/widget pointers.
The scene must dismiss/collect the panel before destroying its owners.

Actual-LVGL tests run 1,000 open/lock/close cycles for each of nine profiles and
both transport layouts, with stable retained C++/LVGL/arena memory. Generated
manifest dimensions and production card/wrap helpers drive the fixtures. Tests
also cover idle allocation/redraw guards, tick wrap, dynamic PIN/theme/fonts,
long identity text, bounded malformed IP input, metadata/headroom/style failure,
stale chrome input, bare-click rejection, reentrant create/sample/lock/settings
callbacks, close during rendering and generation-zero replacement after old
configuration bytes are overwritten. Core model tests separately cover delayed
retirement, local/HA connection changes and opening exhaustion. These are host
checks, not physical font/network/lock qualification or factory firmware builds.

After scene integration, test Guition 4848S040 S3 and a working 20-slot JC8012P4A1
P4: root/subpage Settings access and fonts, Back/Lock/PIN behavior, offline IP and
Wi-Fi labels, rotation, theme changes, screensaver takeover and repeated config
replacement. Record matched heap/PSRAM, minimum heap, largest block, latency and
draw peaks. Keep P4-86 reserved for USB diagnosis. Navigation depth and backup
formats are unchanged at this stage.
