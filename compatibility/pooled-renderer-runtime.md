# Prepared page layouts and reusable bindings

`PageRendererRuntime<slotCount>` is the main-loop-only navigation/binding owner
for the pooled renderer migration. It is not selected by production firmware.
The existing LVGL grid remains active while real widget/driver ports are built
and physically qualified. No nesting or capacity change is enabled here.

The owner stores exactly the profile's number of binding slots. Preparation
compiles immutable root/subpage layouts using the same order/span/Back parsers
as the existing grid. Layout parsing and custom Back-label decoding happen
before commit, not on navigation. A portrait view can use 18 visible cells while
the 20-slot configuration retains its inactive records.

The device port must reserve its bounded widget, driver and text resources in
`prepare`. `bind`, `unbind` and `update` reuse those resources. Text inputs are
length-delimited views; ports must use their reserved buffers and preserve the
existing UTF-8/formatting rules. This interface does not yet implement those
ports or prove that real LVGL/card code satisfies its allocation contract.

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
The application must serialize preparation, commit and activation with the
main render loop; this host model is not production task/thread wiring.

Host coverage checks all nine profiles, 1,000 page cycles per profile,
configuration replacement, failed reservation, folder removal/ID changes,
rotation, Back labels/colours, stale tokens, reentrancy and takeovers. A second
integration uses the real dependency source, coordinator adapter and state owner
through one PageStore transaction. It checks 1,000 allocation-free navigation/
update cycles with stable subscriptions, hidden-page retained values,
reconnect invalidation and release ordering. Allocation counters and sanitizers
cover these model paths, not network dispatch, real widget drivers or device
memory/visual/touch behaviour.

Before production cutover, implement every card's real bind/update/unbind port,
profile-bounded child/context pools, persistent display buffers, native action
routes, modal/forecast/local-value ownership and main-loop transaction wiring.
Physically qualify S3, then a 20-slot P4 and the remaining grid sizes. Record
whole-device and owner-level steady/peak memory, navigation latency and image
peaks. Ship one selected renderer implementation per production build.
