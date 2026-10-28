# Prepared page layouts and reusable bindings

`PageRendererRuntime<slotCount>` is the main-loop-only navigation/binding owner
used by the concrete [pooled application](pooled-application-owner.md).
Generated device profiles select that application at compile time, including
startup or allocation failure. Standalone consumers without a generated
profile retain their explicit boot selection. No nesting or capacity change
is enabled here. Physical qualification remains a cutover requirement.

The real widget port is described in [pooled-basic-cards.md](pooled-basic-cards.md).

The owner stores exactly the profile's number of binding slots. Preparation
compiles immutable root/subpage layouts using the same order/span/Back parsers
as the existing grid. Layout parsing and custom Back-label decoding happen
before commit, not on navigation. A portrait view can use 18 visible cells while
the 20-slot configuration retains its inactive records.

The device port must reserve its bounded widget, driver and text resources in
`prepare`. `bind`, `unbind` and `update` reuse those resources. Text inputs are
length-delimited views; ports must use their reserved buffers and preserve the
existing UTF-8/formatting rules. Concrete card and modal ports implement these
operations; host counters and real-LVGL tests check their reservation and
retirement paths. Whole-device allocation peaks require physical tests.

Each binding token includes the committed configuration, binding revision,
page, card and shell. Tokens become invalid before any unbind work. Delayed
input/timer callbacks must carry their origin token, not read a replacement
context's token when they execute. Suspend/resume invalidates gestures without
creating new subscriptions. Exhausted binding revisions fail closed instead of
reusing a token. Back is limited to the existing root/first-level hierarchy.

Replacement restores the currently open folder by its owning root slot when
that folder survives, even if its PageId changes. Otherwise it returns Home.
Failed preparation keeps the old view and its bindings active. The state owner
exposes a revision poll that also observes reconnect invalidation, so visible
values can clear while waiting for a fresh announcement.

`PageRuntimeResourcesGroup` coordinates prepared owners. Place the renderer and
modal-unbind owners before shared state owners: activation invalidates/unbinds
old views before the old graph/model storage is released. Participants receive
the published PageStore view, including its committed generation. Binding must
defer state reads until the first update after every participant activates.
The application scene serializes preparation, commit and activation with the
main render loop, and defers maintenance during native or LVGL callbacks.

Host coverage checks all nine profiles, 1,000 page cycles per profile,
configuration replacement, failed reservation, folder removal/ID changes,
rotation, Back labels/colours, stale tokens, reentrancy and takeovers. A second
integration uses the real dependency source, coordinator adapter and state owner
through one PageStore transaction. It checks 1,000 allocation-free navigation/
update cycles with stable subscriptions, hidden-page retained values,
reconnect invalidation and release ordering. Allocation counters and sanitizers
cover model paths and the concrete LVGL/card drivers. Device network timing,
whole-device memory, visuals and touch behaviour still require physical tests.

Device builds route shared clock, date, temperature and forecast events into
coalesced scene maintenance. Camera/image refreshes use the admitted native
image owners and preserve metadata refresh, downloader priority and display
suspension policy. The obsolete eager weather/timezone/calendar/image context
registries are excluded from generated-profile binaries; shared modal UI
storage remains in use.

Before shared production cutover, physically qualify S3, then a 20-slot P4 and
the remaining grid sizes. Record whole-device and owner-level steady/peak
memory, navigation latency and image peaks. Host tests and all-profile compile
checks do not establish these physical results. Ship one selected renderer
implementation per production build.

Normal overlay input cancellation while retaining feedback origins is described
in [covered grid input](pooled-covered-input.md).

The native configuration/loop lifecycle bridge is described in
[application scene integration](application-scene-integration.md).
