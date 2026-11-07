# Demanded Wi-Fi sharing modal

This #1825 prerequisite ports the QR/details/guest panel to one demanded owner.
It builds on [pooled QR cards](pooled-qr-cards.md), the
[bounded QR owner](pooled-qr-slots.md) and the
[demanded guest controller](pooled-guest-wifi-state.md). Production still selects
the existing renderer. Application-scene integration, compiled legacy retirement
and physical qualification remain. No firmware was flashed and no physical
memory improvement is claimed.

`PageWifiQrModalPort` owns one context with copied configuration, credentials,
ordered tabs and a fixed gesture reservation. QR storage is allocated only if
the opening has a QR tab: one matrix/descriptor bank and one decoder, using the
scene's shared encoder. Details/guest-only openings create no QR bank. Hidden
configured cards retain no modal widgets, password labels, matrices, callbacks or
timers. The guest controller borrows only the demanded context's owned entity
and is unbound before that context is destroyed.

The existing tab order, duplicate filtering and default QR/details selection,
Connect/custom caption, credential labels and guest toggle appearance are kept.
The adapter uses the common modal/tab/content/power geometry, including the
single-tab Back safe area, standard QR encoding and the 350ms Back guard. Tab
and guest input only update scalar state or queue an intent; maintenance handles
layout, encoding and sending. An unchanged pump performs no redraw/allocation;
guest state changes avoid unrelated panel/credential/QR layout work. Translation
literals are cached independently of normalized display glyph bytes. Theme/font
changes refresh the existing objects. The style resolver receives the full source
opening token during preflight, so a hidden subpage can resolve its own font roles
without consulting the currently visible grid.

`PageModalPort` now exposes family-specific connection checks, with the existing
checks as the default. Requests resolve their family once before capturing its
epoch; maintenance verifies that same family and epoch again. A composite scene
must route these checks to the selected family. Static QR/details use a stable
local epoch and remain usable offline. HA-backed controls retain their HA epoch
and stale-request rejection. Guest presses independently capture the graph's HA
connection, wait for fresh state after reconnect and cannot adopt a later epoch.
Missing/invalid guest dependencies disable guest controls without closing valid
static content. No HA command is sent merely by opening, selecting a tab or
closing the panel.

Retirement revokes gestures, guest ownership and QR drawing immediately. It
preserves objects, descriptors and pixels during dispatch or an active decode.
Safe collection drains hardware draw work before releasing QR storage and
widgets; a held decoder session defers collection and the next context. The
application scene must dismiss and collect modals and drain display work before
tearing down its owners. No widget is hidden/deleted from inside rendering.
Opening identity, configuration/card identity and QR operation identities remain
independent, including configuration generation zero and the final opening nonce.

Native tests use actual LVGL and QR encoding, the modal runtime, graph and guest
controller. Common geometry and widget helpers are generated from production;
shell/transport boundaries are doubled. Each of nine profiles runs 1,000
open/tab/send/acknowledge/close cycles with stable retained C++/LVGL/arena bytes
and registrations. Further cases cover 1,000 idle pumps, offline static content,
fresh guest reports and old presses after reconnect, tab order/defaults, protected
credentials, hidden-card source fonts, request/gesture allocation guards, the
last opening, metadata/QR arena and shell/style failures, reentrant close, held
sessions, retirement inside drawing and generation-zero replacement after old
configuration bytes are overwritten. Seven portrait/landscape/square sizes
exercise single-tab Back clearance. The core model separately switches local
and HA-backed panels 1,000 times while rejecting stale HA requests. These tests
are not physical device/font/network qualification or current factory builds.

After scene integration, qualify Guition 4848S040 S3 first and a working 20-slot
JC8012P4A1 P4 next. Check scanned QR output, root/subpage fonts, tab/Back geometry,
credential text, guest acknowledgement/timeout feedback, offline access,
rotation/theme changes, screensaver takeover and repeated configuration changes.
Compare matched heap/PSRAM, largest block, minimum heap, draw peaks and latency.
Keep P4-86 reserved for USB diagnosis. Document/backup formats and navigation
depth remain unchanged by this stage.
