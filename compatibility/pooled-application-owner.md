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

Live unit, timezone and time-sync notifications update the native temperature
suffix snapshot and repaint on the safe scene loop. They retain bindings,
subscriptions and configuration generations, including across display sleep.
The prepared text buffers fit both supported suffixes. Card-curve changes use
scene-aware readiness so a running native bank refreshes without a reboot.

The page-definition arena capacity remains the admission ceiling, while each
generation retains only its used bytes. Preparation first validates privately
at the ceiling, rebuilds into an exact-size arena, then prepares resources that
borrow its strings. The ceiling scratch is released before those resources are
reserved. Compaction allocation or resource failure leaves the active generation
intact. Activation allocates nothing and retires arenas with their actual sizes.
Memory accounting includes the transient ceiling, compact and active arenas;
this does not yet remove the full-size definition staging peak.

The production HA graph starts its packed value pool at 4 KiB and grows on
actual network demand, preserving its total 128 KiB (constrained) / 512 KiB
limits and 32 KiB per-value limit. Channel metadata and registrations do not
move when this separate PSRAM pool grows. Updates within its current capacity,
navigation and activation reserve nothing. The default fixed-reserve policy
remains available, including for local producer state.

Retained shared values must fit the prepared graph before commit: a failed
copy reservation rejects the replacement and leaves the active scene intact.
A later network growth failure marks that channel exhausted without corrupting
other channels; a later report can recover. Allocation failures are counted
separately from quota overflows. Owner counters include value capacity and the
overlap of active/staged metadata and old/new value buffers. Replacing the
configuration releases its retired pool rather than retaining past demand.

The concrete device wraps its scene allocator with `PageCountedArena`, tracking
live requests, allocation failures and the actual overlapping arena high-water
mark without a per-allocation registry. Device-object bytes, LVGL widgets,
image download/decode storage, platform framebuffers and allocator overhead are
separate from these requested-byte counters. Read snapshots on the main loop;
an accounting error invalidates the counter snapshot without changing the
backing allocator's behavior. Complete-scene shutdown tests require the counted
reservations to return to zero.

HA coordinator callbacks capture a four-byte token, rather than the owner,
revision, channel and delivery function. The S3 toolchain's eight-byte inline
callback storage otherwise puts that sixteen-byte capture on the internal
heap for every channel. Each graph instead embeds one dispatch lease per
active/staged slot. Preparing a lease reserves a never-reused token range;
release unlinks it before resetting or freeing graph storage. Late callbacks
therefore remain harmless after graph/adapter destruction and address reuse.
Dispatch copies its scalar destination before entering consumer code, so that
delivery may retire its own lease. The lease bookkeeping needs no allocation,
including during activation/release, and token exhaustion rejects preparation.

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

Default ESPHome TemplateText mirrors use the pinned SDK's existing preference
keys and length-prefixed payloads directly. Each field drains the preference
queue before and after its write, with a stack window of at most 256 bytes.
This avoids the SDK control path's extra cached string and a complete batch of
pending blobs. Failed flushes report a compatibility failure; retries compare
durable data instead of trusting the SDK saver's potentially stale cache.
Unchanged live text is not published again. Other Text platforms retain their
normal control behavior. This does not change native or legacy storage layouts.

Compatibility text limits apply to fields owned by that projection. Native-only
settings, such as badge rows, retain their native size limits and remain in the
durable document; they do not prevent compatible button fields from being
mirrored. Compatibility edits continue to preserve those native-only records.

The optional `page_native_scene_test` sanitizer harness compiles the complete
production scene unchanged with real LVGL widgets, state owners, command
policies/queue, artwork request/view pipelines and QR encoder/frames. Hardware,
HA endpoints and modal UI families are doubled. The latter retain independent
real-family UI tests. Coverage includes 1,000 navigation cycles without widget
growth, hidden-page styles, modal retirement, deferred commands, slider tap/drag
separation, three QR tiles with two image slots, image completion/drawing,
configuration replacement/rollback, appearance-only rollback and shutdown.
The complete scene also exercises all 81 supported S3 cards and all 400 P4 V3
cards using their respective production arena budgets. Each profile performs
20 replacements between disjoint sensor sets (72 or 380 sources) and 2,000
navigation cycles with further arena reservations disabled. Every slot must
remain visible, registrations/widgets/reserved bytes must remain stable, and
retired-generation reports must be rejected. These are host capacity fixtures,
not physical maximum-configuration qualification or maximum-dependency/text
fixtures.
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

The native device exposes the current immutable palette to scene maintenance.
Light, Dark and automatic theme changes repaint the retained bank and grid
background in place, preserving display calibration, checked accents, page
bindings, configuration generations and subscriptions. Changes during callback
dispatch wait for safe maintenance, including while the display is suspended;
no document or view-bank rebuild is needed.

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

The controlled S3 comparison on `2b0e6c6c5` against legacy `ac5214e3c`
uses SDK 2026.10.0, the same read-only memory action and nine root sensor cards.
Both cohorts verify visible cards, no loaded artwork or image requests, ten
settled samples, zero unexpected API disconnects and exact original-document
restoration. Median internal free memory rises from 70,871 to 80,419 bytes;
PSRAM free falls from 2,523,900 to 2,264,472 bytes. Combined free therefore
falls by 249,880 bytes. This is a regression, not a completed memory gate.

The S3 copies instructions and read-only data into PSRAM. ELF instruction-page
reservation grows by 196,608 bytes and read-only data by 16,256 bytes, matching
the measured 212,864-byte drop in allocator-managed PSRAM capacity. Those
allocator totals are not physical RAM capacity. The remaining difference also
includes runtime allocations; retaining both compiled renderers is still a
separate cleanup gate. This small fixture does not qualify maximum real HA
values, artwork peaks, every card family or the P4.

Configuration storage now reads and writes raw partition byte ranges without
retaining either full slot image. NVS keeps its deployed whole-blob format and
borrows one PSRAM slot image only during each serialized store load or commit;
all exits release it. The two permanent slot buffers are removed (32 KiB on
NVS profiles; 80 KiB with 40 KiB partition slots). Three permanent service,
HTTP and deferred-application buffers remain for the next storage stage.
Host tests compare complete old/new durable images, read in both firmware
directions, interrupt every mutating storage call before/during/after its
write, and cover allocation/read/sync failure and retry. These checks do not
replace physical power-loss or full backup/downgrade qualification.

Partition validation uses a 1 KiB stack window. The preceding 64-byte window
made the direct-flash backend perform 2,566 reads to inspect two full 40 KiB
slots and load the selected payload; the bounded window reduces this to 166
reads without allocating slot RAM or changing CRC bytes. The physical S3
21,694-byte save/restart/restore test exposed the transaction overhead; latency
must be measured again on the updated firmware, separately from host checks.

Remaining gates are current factory builds, matched S3 and working 20-slot P4
heap/PSRAM tests, the remaining transient configuration buffers and
backup/downgrade qualification. Depth-two nesting follows the runtime overhaul. Do not access the
P4-86 reserved for USB diagnosis as part of these hardware tests.
