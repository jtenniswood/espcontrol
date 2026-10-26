# Shared local relay state and card views

The unselected renderer supports Internal Relay toggle and push cards through
`PageLocalRelayRuntime`. Toggle cards share one state record per key across
navigable pages. Push cards have no retained state and do not contribute to
folder indicators, matching the existing renderer.

The runtime copies keys into exact configuration-owned arenas, with default
limits of 32 distinct toggle sources and 2,048 key bytes including terminators.
Preparation snapshots the firmware-lifetime relay registry. Activation refreshes
those snapshots without allocating or dispatching actions. Preparation failure
or discard preserves the active configuration. Private preparation generations
are promoted to the real committed generation at activation. An integration test
uses `PageStoreRuntime` publication, including relay/media/forecast/modal promotion,
generation-zero initial loads, empty local-sensor retirement and cancelled
replacements. A push-only configuration frees all relay arena storage.

A fixed application-loop feed receives existing `notify_internal_relay_changed`
notifications for active and staged configurations. It can be installed only
after the caller declares the legacy renderer retired, belongs to one runtime,
and clears on destruction. It adds no producer callbacks, timers, HA
registrations or per-card contexts. State is not polled every frame: manual
notifications retain the same meaning as in the existing renderer. HA reconnects
do not invalidate device-local relay state.

Labels retain configured, registered and sentence-cased key precedence, with a
translated Relay fallback for an empty key. Prepared labels own their copied
text. Auto icons retain Power Plug for toggles and Gesture Tap for push cards;
custom on-icons apply to toggles. View actions pass the full validated binding
to the scene's toggle hook. The eventual production scene must resolve the
original card and dispatch its existing push/toggle helper, rather than infer
the mode from the relay key.

Memory diagnostics separate fixed runtime/feed storage, active and staged arena
bytes, peak arena bytes, source count and preparation failures. They do not
measure allocator overhead or establish physical heap/PSRAM savings.

Host tests cover shared sources, fresh activation, notifications, unknown keys,
failed preparation, owner lifetime, budgets and 1,000 disjoint replacements.
Native LVGL tests cover all nine profiles, labels, icons, push/toggle actions,
folder indicators, stale presses, reconnect independence and 1,000 navigation
cycles with stable retained resources. Use `ESPCONTROL_TEST_FAMILY=local_relays`
for the focused native suite.

Production selection, remaining drivers and physical qualification are still
unfinished. No device was flashed by this stage; saved and backup formats are
unchanged. P4-86 relay hardware testing must follow its separate USB diagnosis.
