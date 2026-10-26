# Configuration-owned modal lifetime

The pooled runtime now has a modal owner and an adapter to the existing climate
panel. Neither is selected by production. Other modal families, the complete
scene/configuration transaction, physical styling and device qualification remain
cutover work. This does not enable nested pages or change document versions.

`PageModalRuntime` holds one pending request and at most one allocated context.
A touch request captures a grid binding and is checked again before creation on
the main loop. Authenticated modal APIs can instead pass a current configuration
card handle, including a navigable hidden page. Stored inactive pages are not
modal targets. After opening, configuration/card/kind and a never-reused opening
nonce identify the modal independently of a suspended grid binding.

Closing invalidates the opening immediately and releases callback/timer ownership.
It keeps context memory until the main loop is outside native, coordinator,
LVGL event and timer dispatch. Reentrant close, update and destruction cannot
start another cleanup pass. A retired context owns its configuration strings;
releasing the old PageStore arena cannot invalidate an unfinished callback.
The application, port and dependencies must outlive callback dispatch, and final
owner destruction must run after those callbacks finish.

The existing `ControlModalStateService` retains close guards, nested menus and
preserve-on-takeover policy. The generic owner can preserve an alarm-style
modal during takeover; the climate adapter uses the existing dismiss policy.
Real scene wiring must call modal maintenance before presenting the next frame.
No LVGL work may originate from an HTTP worker.

## Climate adapter

`PageClimateModalPort` allocates one context when opening, copies normalized
configuration from PageStore, and borrows profile fonts for its lifetime. Its
required style resolver supplies current profile fonts and per-card colours.
Installation rejects an existing eager climate owner. Every grid-widget pointer
in the modal context is null, so an old callback cannot repaint a reused card.
The panel still uses the existing climate visual/control helpers.

The shared graph primes new and lazy subscriptions through the guarded modal
replay source. Initial replay does not reset already-initialized edits. Managed
optional fields are attached and primed on the main loop, bypassing the legacy
reannouncement timer. Closing returns subscriptions to the graph's current
requirements. The existing coordinator's allocator/failure contract still
applies to modal callback allocations.

Normal close or switching panels completes a valid pending temperature command
in safe main-loop collection, after callback/draw dispatch finishes. The closing
callback sends nothing. A later takeover, replacement, explicit cancellation or
connection change revokes the queued edit, even if the panel has already retired.
Collection rechecks the full opening identity and fresh graph availability/HVAC
mode before sending. Disconnect, replacement, failed setup and shutdown cancel it. A valid command
rejects non-finite/out-of-range integer temperatures and clears its pending flag
before dispatch, avoiding a second send if closing runs
reentrantly. A callback can close the panel and continue reading its context
until dispatch returns. Option-list callbacks copy their chosen action before
refreshing a list whose click records can be reused.

Replacement/shutdown retirement cancels callbacks/timers and removes the context
from global refresh lists. Outside LVGL dispatch it also hides overlays and
detaches shared modal-service state. During LVGL dispatch all overlay changes
wait for safe collection, including shared control/select/media, alarm and weather
panels. The opening and gestures are already invalid, so the retained widgets
cannot accept another action. Label/widget deletion and clock-label refresh wait for the following
safe maintenance pass, outside durable activation. Old UI strings remain owned
by the context/LVGL during that interval. Complete activation allocation counts
and presentation ordering still need the selected scene and physical tests.

A fallible, caller-budgeted gesture arena is reserved before panel creation. It
adds preprocessing guards to interactive objects, captures presses, and rejects
click/arc events from another opening or after retirement. Deleted option-list
objects release arena entries for reuse. A full arena fails the panel setup and
closes it; controls are never silently left unguarded. The default 128-entry
reservation is a modal setup budget, not a qualified device capability increase.
The pool outlives widget deletion and event dispatch.

## SDK boundary and verification

LVGL exposes no public event/timer-dispatch query. `page_lvgl_dispatch.h` reads
the event-stack and timer-handler flags from its private global state without
mutating them. An exact LVGL 9.5.0 compile guard makes dependency upgrades stop
for an audit. Review this bridge alongside the repository's pinned ESPHome
version. It does not patch SDK files or alter their event scheduling.

The normal host suite runs 1,000 modal cycles against the real modal-state
service with fake widget/context policies. It covers stale/rebound requests,
address reuse, hidden/inactive targets, close guards, nested menus, takeover,
configuration replacement, reconnect epochs, reentrant retirement and nonce
exhaustion. The native LVGL tests add 1,000 option-list rebuilds and stale input
checks, using bounded guard storage.

Real LVGL refresh probes retire each control/select/media, climate, alarm control,
alarm PIN and weather panel from its draw callback. They assert that the overlay
and child tree remain visible and intact until drawing returns, then are collected
by maintenance. These probes run across all nine profiles alongside the existing
1,000-cycle lifetime checks. The runtime model also covers final-edit revocation
with configuration generation zero and nonzero, including same-generation
replacement and reconnect before collection.

The optional `page_climate_modal_test` runs the actual adapter, coordinator,
configuration graph, production context/subscription/temperature/teardown
helpers and real LVGL event/timer dispatch. Its visual panel and outbound action
transport are test doubles. Across 1,000 cycles it checks stable retained bytes,
registrations and arenas, pending-edit preservation, single final sends, failed
reservations/style resolution, unavailable updates, disconnect cancellation,
live temperature overflow, model retirement during dispatch and a
generation-mismatched graph. Sanitizers
cover those lifetime paths. A copied generated S3 translation unit also compiles
the full adapter and real panel against the pinned SDK without modifying caches
or flashing a device.

These checks do not establish actual climate-panel layout, touch/arc behaviour,
physical memory peaks or scene integration. Those remain required before
production cutover. The latest S3 integration must preserve the speaker-group
firmware currently installed from PR #2286 and the preceding reliability fixes.

Queued requests capture the connection epoch at admission, for both visible
bindings and authenticated hidden-card requests. A reconnect before the main-loop
opening discards that request, even when the connection is back by the next loop.
Creation retains the captured epoch: a connection change inside a fallible create
prevents initialization, and a change during initialization retires the opening
without adopting it into the new connection. A fresh request can then open normally.
