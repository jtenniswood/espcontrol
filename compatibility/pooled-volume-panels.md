# Demand-created volume panels

The unselected page renderer can open the existing volume panel using one
context for the active panel. It reads the configuration-owned entity graph;
opening a panel adds no `MediaPlaybackState`, per-card subscription, or HA
transport registration. Closing the panel retires its context and gesture
storage after callback dispatch completes.

The existing panel layout, labels, volume limit, 1.5-second optimistic grace,
unknown-volume display, absolute/step/read-only capability rules, and service
commands are shared with the current renderer. Inputs pull fresh graph state
before deciding a command. The graph-backed adapter skips unchanged frames,
including expired grace periods, without allocating or rewriting labels.
Disconnect, unavailable state, configuration replacement, and display takeover
invalidate the opening and its delayed gestures.

The optional numeric-preview sink receives the exact modal opening token only
for an absolute-volume command. During scene integration, retain the original
full grid binding when requesting the panel and validate both tokens before
calling the card bank's `preview_volume`. Looking up a replacement binding by
card ID would incorrectly update a reused shell. Step/read-only inputs never
publish an absolute preview. Speaker-group linked controls keep their existing
callback behavior; this adapter only accepts standalone volume cards.

Native LVGL tests extract the production context, panel construction, event
handlers, command policy, and shared snapshot application. Only platform
shell/layout and network operations use doubles. They cover live capability
changes before input, acknowledgement and grace expiry, stale gestures,
configuration replacement during a preview callback, admission failures,
reconnect invalidation, 1,000 unchanged frames and 1,000 open/close cycles.
These are host checks, not physical device qualification.

Production still selects the existing renderer. Full media-control/group
panels, scene wiring, complete card parity and physical S3/P4 workload and
memory qualification remain prerequisites for cutover. This stage changes no
saved configuration, backup format or subpage depth and makes no device-memory
saving claim.
