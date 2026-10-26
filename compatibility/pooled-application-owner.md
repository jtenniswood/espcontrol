# Concrete pooled application owner

This #1825 stage connects the existing pooled card, state, command, artwork, QR
and modal owners in `PageNativeScene<Slots, ImageSlots>`. Device startup now
selects this scene through a PSRAM-backed `PageNativeDevice` and one fixed widget
bank. This stage neither enables nested pages nor establishes physical memory
savings; selected firmware builds and physical qualification remain gates.

One application-lifetime scene owns the immutable page store, shared HA graph,
semantic models, a single card command queue, the fifteen-kind modal bank and
one supplied widget bank. QR leases use the visible slot count independently of
the smaller image downloader pool. Configuration preparation remains fallible;
activation retires modals and image leases before rebinding the renderer, then
publishes commands and semantic state before the graph. Navigation creates no
widgets or per-card contexts.

The page-definition arena capacity remains the admission ceiling, while each
generation retains only its used bytes. Preparation first validates privately
at the ceiling, rebuilds into an exact-size arena, then prepares resources that
borrow its strings. The ceiling scratch is released before those resources are
reserved. Compaction allocation or resource failure leaves the active generation
intact. Activation allocates nothing and retires arenas with their actual sizes.
Memory accounting includes the transient ceiling, compact and active arenas;
this does not yet remove the full-size staging peak or HA state-cache reserve.

Ordinary panels cover input while retaining the underlying full bindings and
artwork. Retired panels continue covering input until safe collection. Full
display/PIN takeovers suspend the bank; critical alarm presentation uses the
platform's display lease. Display policy is sampled after callbacks that can
change it. A temporary input guard cancels unsent confirmation, alarm and guest
Wi-Fi commands without discarding the panel's ownership or an already-sent
guest acknowledgement. Shutdown revokes tokens immediately and collects only
after native/HA/LVGL dispatch ends.

An accepted media action reply validates its exact opening and connection rather
than asking to admit new input. It may update its current panel inside native
callback dispatch or while a wake guard covers gestures. Recursive scene/configuration
maintenance and new panel/grid commands remain blocked during that callback.

`PageNativeDeviceStyles` resolves colours and fonts from the original owning
page, including authenticated hidden-card openings. Device startup captures
root/child source font roles before rebinding the bank. Resolvers reject wrong
generations and owning pages. Confirmation captions borrow immutable store text,
not temporary parsed strings. Fonts and theme caption/unit views must outlive
the application scene. The grid pointer supplies layout geometry, never source
card identity.

`reconfigure` prepares only view/layout resources for appearance or orientation
changes. It preserves subscriptions and cached semantic state, invalidates old
bindings, closes old panels and rolls back on allocation/preflight failure. It
refuses to overwrite the prepared resources owned by an outstanding save.
The device owner commits its grid descriptors and source style profile at the
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

Production boot and refresh no longer enter the eager legacy renderer. A failed
native reservation blocks configuration writes rather than selecting a mixed
renderer. The existing YAML wake actions remain, while native bindings own card
commands. Initial configuration can activate while loading/setup pages own the
display; the bank stays suspended until the main page is shown. Appearance and
configuration activation synchronize with the renderer's actual suspension state,
including a wake that occurs before the next scene-maintenance iteration.

Remote modal/page/navigation APIs and network settings delegate to one native
navigation owner. Selection uses immutable definitions and compiled visible
layouts, with root/display-order priority and rejection of unreachable folders.
Home requests can wait while the display sleeps; root-card activation shares the
physical typed press/click path. Requests revalidate configuration and connection
before dispatch. Full display takeover retires native image leases through the
same draw-safe lifecycle, including the S3 camera screensaver path.

After the prepared scene activates, the configuration service publishes the
complete native document to live compatibility text entities before downgrade
mirroring. Publication is separate from the allocation-free scene generation
swap. A publication failure is reported as a compatibility failure; the newly
committed scene/document remain authoritative, including during boot restore.
A partially published compatibility view cannot be re-imported by a later text
refresh. A successful publication of the durable document reopens that edit path. Debounced direct text
edits merge only fields owned by the compatibility projection, retaining
native-only settings. They use normal runtime preparation and durable commit;
failed preparation/storage leaves the previous scene and document intact.
Equivalent records in a different order and appearance-only refreshes do not
rebuild the scene or write storage. Boot continues to prefer the durable native
document over possibly stale persisted legacy text.

The concrete startup argument types and the generated selected S3 main source
pass SDK syntax checks. These are separate from a complete firmware link and
physical testing. Profile/generator checks, boot migration, native navigation,
settings/image takeover, legacy publication/merge and transaction failure
regressions pass locally. Full CI and selected factory build results belong to
the immutable commit recorded in the PR validation notes.

Remaining gates are selected factory builds, matched S3 and working 20-slot P4
heap/PSRAM tests, streaming configuration storage and backup/downgrade
qualification. Depth-two nesting follows the runtime overhaul. Do not access the
P4-86 reserved for USB diagnosis as part of these hardware tests.
