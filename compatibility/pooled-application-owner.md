# Concrete pooled application owner

This #1825 stage connects the existing pooled card, state, command, artwork, QR
and modal owners in `PageNativeScene<Slots, ImageSlots>`. Device startup now
selects this scene through a PSRAM-backed `PageNativeDevice` and one fixed widget
bank. Generated profiles select it at compile time, including allocation or
startup failure; standalone consumers retain explicit boot selection. This
stage neither enables nested pages nor establishes physical memory savings.
Physical maximum-load, memory and parity qualification remains required.

Configuration text wiring and bound compatibility values use the actual slot
and chunk counts emitted by ESPHome code generation. The compatibility lookup uses the same bounds; merging preserves unowned
native records, and unsupported compatibility fields reject mirroring rather
than truncating data. The native document retains its existing 32-slot format
and inactive-record semantics. The deferred configuration owner explicitly allocates in PSRAM even
when its smaller size falls below the SDK's ordinary internal-allocation
threshold. Allocation failure preserves the existing delayed retry path.

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

An active climate modal compares its displayed C/F suffix with the current
policy during safe maintenance. This also handles a change without new HA data
or an intervening shared-panel repaint, while preserving a pending edit. The
native context registry has one reference, matching the single demanded modal
record; standalone legacy capacity is preserved. Retired and reset references
are cleared, and the shared climate UI retains its existing ownership.

Clock/date/forecast producers queue scene work instead of walking eager page
registries. Explicit camera refresh queues fresh shared-channel metadata for
admitted visible images. Reconnect and endpoint maintenance also refresh cover
art through the native owners. Configuration activation, appearance rebuilds
and stop discard pending image refreshes, so work from an old configuration
cannot refresh its replacement. Hidden/takeover policy and downloader limits
remain unchanged.

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
Large graph builds use a temporary hash lookup to deduplicate entity/attribute
pairs. Ordered declarations check duplicate edges within the current card;
interleaved sources keep their stable general path. Lookup allocation failure
rejects preparation without changing the active configuration, and lookup
storage is released before edge grouping, value copying or transport setup.
Its live bytes are included in the graph's overlapping reservation peak.
Appending copied values to the packed pool skips neighbour-offset scans when
no existing values move; in-place growth/shrink still preserves borrowed views.
Large replacements use a temporary sorted channel-index array to match shared
values, rather than scanning every old channel for every new one. The array
uses the fallible scene allocator and is released before preparation returns;
its bytes and overlapping value-pool growth count toward the graph peak.
Allocation failure preserves the active graph. Coordinator retirement marks
channel activity once and remaps subscriptions and deferred reads in single
passes, preserving callback order and stable native callback identities.
Callback snapshots use an allocator-backed reference count confined to the main
loop. The P4 C++ library implements `std::shared_ptr` control blocks with OS
mutexes, whose internal-RAM allocations bypass the external allocator and can
fail under large subscription loads. The callback lease avoids these hidden
allocations while keeping an executing callback alive through owner retirement
and skipping cancelled callbacks in the remaining snapshot. It must not cross
threads; the coordinator and its dispatch already require the main loop.
SDK-owned subscription records also need external memory: their ordinary
`std::vector` allocations bypass the application allocator. Display profiles
share an ESP-IDF policy that prefers PSRAM for allocations above 4 KiB and
internal RAM for smaller allocations, with 32 KiB reserved for internal-only
and DMA requirements. This changes neither document capacity nor storage
layout or watchdog timeouts. SDK storage remains outside the scene arena
counters, so qualification also measures free memory and low-water marks in
each heap region.
Native graph registration borrows a temporary sorted index of existing
coordinator channels. Each new graph pair is unique, so registration searches
only that index and appends missing channels without rescanning the growing
list. The fallible scene allocator owns the index, counts its peak bytes and
releases it before preparation returns, after ending the borrowed batch. The
shared-value copy index is released before this transport index is allocated.
Stable coordinator IDs and native registry tokens use binary lookup. Registry
retirement temporarily sorts borrowed-name addresses for exact ownership
removal, then restores token order before dispatch or new installation;
foreign and one-shot native subscriptions remain untouched.
Single-consumer channels reuse the existing maintenance word as a callback
index, avoiding a global subscription scan and temporary vector on each value.
Shared channels keep ordered snapshots. Compaction rebuilds the index, activity
refresh preserves shared status, and pending-read reentrancy revalidates the
callback before delivery. This adds no per-channel or per-subscription storage.

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
Unchanged live text is not published again. Before replacement writes, a fully
validated mirror clears obsolete empty preferences only for restore-enabled
TemplateText fields whose configured initial value is empty. Missing keys restore
the same empty value in the pinned SDK and older readers. This reclaims space in
small NVS partitions and avoids materializing every unused subpage chunk. Fields
with nonempty defaults retain explicit empty blobs; other Text platforms retain
their normal control behavior. Generated metadata selects eligible fields without
user YAML changes. Native and legacy storage layouts remain unchanged.

A native document may still exceed the legacy partition budget even when its
individual fields fit. Its durable native save remains valid, while the API
reports the compatibility warning; this does not reduce native configuration
limits or establish compatibility with firmware that only reads legacy fields.

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
NVS profiles; 80 KiB with 40 KiB partition slots).

The service scratch buffer is now scoped to a serialized load, merge or commit.
Firmware allocations use PSRAM explicitly and fail before durable publication
if unavailable; there is no throwing vector resize or internal-heap fallback.
Read/merge scratch is released before a subsequent encoding allocation, so the
service borrows at most one slot at a time. Tests inject allocation failure at
each stage, verify the old generation and active runtime remain intact, and
exercise repeated success, conflicts, store/mirror failure and retry without
retained allocations. The optional externally supplied scratch API remains
compatible. This removes another 16 KiB permanent allocation on NVS profiles,
or 40 KiB with partition slots. The S3 service-buffer build gains 39,962 bytes
combined free over its predecessor in the settled nine-root-sensor workload.
The largest internal block stays 31,744 bytes; combined free remains 127,404
bytes below the original legacy baseline. The 21,694-byte save/software-reboot/
exact-restore check also passes. Full maximum-workload qualification remains.

HTTP now owns separate buffers for each request. GET keeps its scoped output
alive through main-loop dispatch and response transmission. PUT authenticates
and checks media type, generation header and bounded length before allocating
its exact-length upload. The request owns upload state through reception,
dispatch and response; destruction also frees an incomplete upload after a
disconnect or timeout. No handler-wide upload state or retained document buffer
is shared between requests. Endpoint readiness is independent of retained bytes.

Initial loading, deferred boot restoration and compatibility refresh now also
own scoped output buffers. All five permanently retained full-slot buffers are
removed. Temporary document/encoding buffers still exist, so this does not
establish a constant-space streaming pipeline or complete its qualification.
An HTTP request and a main-loop operation may overlap; account for both callers,
service scratch and the NVS operation buffer separately from active/staged
runtime arenas, graphics and transport peaks.

Temporary allocation failure has a distinct service/store status and returns
HTTP 503. Initialization, boot and compatibility edits keep their pending work
and retry at most once a second without allocating retry timers. Initialization
opens storage and configures the service once across retries; it keeps native
discovery in its starting state until the document can be loaded. Boot retries
reload the latest committed generation, so a later browser save wins. Actual
production lifecycle methods have host allocation-fault, clock-wrap, pending
flag, storage-handle, publication, newer-save and no-retained-buffer coverage.
Genuine storage I/O failures keep their separate error policy.

The editor preserves the native save path through startup/resource HTTP 503 or
a missing endpoint on a previously native panel. It reports a failed save
instead of selecting legacy writes. Explicit legacy-only firmware and capped
missing endpoints that have never advertised native support retain their
compatibility path, including portable backup export. Newer/readonly native
contracts cannot be mistaken for legacy-only firmware.

Host tests compare complete old/new durable images, read in both firmware
directions, interrupt every mutating storage call before/during/after its
write, and cover allocation/read/sync failure and retry. These checks do not
replace physical power-loss or full backup/downgrade qualification.

Browser backup export reads the complete saved native-v1 document after queued
native edits finish, and before later edits run. It preserves the original bytes,
including native-only settings and inactive cards/subpages. The portable layout
is projected from that same snapshot rather than potentially stale SSE state.
The backup envelope and native payload versions remain unchanged. Confirmed
legacy-only firmware retains the portable export path; discovery, authentication,
read, profile or validation failures stop the export instead of downloading an
incomplete backup. A preceding failed save also stops export until a later save
succeeds. A durable native save whose compatibility mirror failed remains
available for backup. These checks do not establish full downgrade qualification.

Partition validation uses a 1 KiB stack window. The preceding 64-byte window
made the direct-flash backend perform 2,566 reads to inspect two full 40 KiB
slots and load the selected payload; the bounded window reduces this to 166
reads without allocating slot RAM or changing CRC bytes. The physical S3
21,694-byte save/restart/restore test exposed the transaction overhead; latency
was checked again on the S3. Five settled reads of the unchanged original
configuration had median latency 50.1 ms before and 37.7 ms after this change;
the ranges overlap and include network/scheduler time. Separate 21,694-byte
save/reboot/restore tests pass on both versions, but their individual save times
do not establish an end-to-end save speedup.

Remaining gates are current factory builds, matched S3 and working 20-slot P4
heap/PSRAM tests, bounded transient configuration peaks and
backup/downgrade qualification. Depth-two nesting follows the runtime overhaul. Do not access the
P4-86 reserved for USB diagnosis as part of these hardware tests.
