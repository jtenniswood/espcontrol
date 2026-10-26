# Concrete pooled application owner

This #1825 stage connects the existing pooled card, state, command, artwork, QR
and modal owners in `PageNativeScene<Slots, ImageSlots>`. Device YAML still
selects the legacy renderer. This stage neither enables nested pages nor
establishes physical memory savings.

One application-lifetime scene owns the immutable page store, shared HA graph,
semantic models, a single card command queue, the fifteen-kind modal bank and
one supplied widget bank. QR leases use the visible slot count independently of
the smaller image downloader pool. Configuration preparation remains fallible;
activation retires modals and image leases before rebinding the renderer, then
publishes commands and semantic state before the graph. Navigation creates no
widgets or per-card contexts.

Ordinary panels cover input while retaining the underlying full bindings and
artwork. Retired panels continue covering input until safe collection. Full
display/PIN takeovers suspend the bank; critical alarm presentation uses the
platform's display lease. Display policy is sampled after callbacks that can
change it. A temporary input guard cancels unsent confirmation, alarm and guest
Wi-Fi commands without discarding the panel's ownership or an already-sent
guest acknowledgement. Shutdown revokes tokens immediately and collects only
after native/HA/LVGL dispatch ends.

An accepted media action reply may still update its current panel inside native
callback dispatch. The scene's read-only modal input policy does not confuse
that with maintenance readiness: recursive scene/configuration maintenance and
new grid commands remain blocked during the callback.

`PageNativeDeviceStyles` resolves colours and fonts from the original owning
page, including authenticated hidden-card openings. Device wiring must capture
root/child source font roles before rebinding the bank. Resolvers reject wrong
generations and owning pages. Confirmation captions borrow immutable store text,
not temporary parsed strings. Fonts and theme caption/unit views must outlive
the application scene. The grid pointer supplies layout geometry, never source
card identity.

`reconfigure` prepares only view/layout resources for appearance or orientation
changes. It preserves subscriptions and cached semantic state, invalidates old
bindings, closes old panels and rolls back on allocation/preflight failure. It
refuses to overwrite the prepared resources owned by an outstanding save.
Device wiring must commit its grid descriptors and source style profile at the
same safe boundary. The scene performs a layout pass after bank binding before
starting size-dependent image/QR work.

The optional `page_native_scene_test` sanitizer harness compiles the complete
production scene unchanged with real LVGL widgets, state owners, command
policies/queue, artwork request/view pipelines and QR encoder/frames. Hardware,
HA endpoints and modal UI families are doubled. The latter retain independent
real-family UI tests. Coverage includes 1,000 navigation cycles without widget
growth, hidden-page styles, modal retirement, deferred commands, slider tap/drag
separation, three QR tiles with two image slots, image completion/drawing,
configuration replacement/rollback, appearance-only rollback and shutdown.
The state-owner test independently repeats 1,000 configuration cycles. An S3
SDK syntax probe instantiates the actual scene and native style adapter; that
is separate from a complete selected firmware build or physical testing.

Remaining gates are the concrete device platform/startup and widget reservation,
retirement of raw YAML/legacy input and eager contexts, remote-opening wiring,
selected factory builds, matched S3 and working 20-slot P4 heap/PSRAM tests,
streaming configuration storage and backup/downgrade qualification. Depth-two
nesting follows the runtime overhaul. Do not access the P4-86 reserved for USB
diagnosis as part of these hardware tests.
