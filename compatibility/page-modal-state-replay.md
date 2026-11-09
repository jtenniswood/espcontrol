# Initial state for bounded modal contexts

The prepared renderer keeps configured HA values in PageEntityStateRuntime,
with coordinator replay retention disabled for that configuration owner. A
newly opened legacy control must receive these values immediately; subscribing
to an already registered channel does not make Home Assistant send it again.

`HaReadCoordinator::replay_owner_missing(owner, source)` provides this handoff.
It runs synchronously on the main loop, outside callback dispatch, after an
owner has registered its control callbacks. Only that owner's subscriptions
which have not received a value are seeded. Repeat it after lazy control setup;
already initialized callbacks are skipped so an old initial value cannot reset
a user's pending edit. Ordinary live announcements and existing coordinator
replay also mark initialization. Native dispatch performs this bookkeeping in
its existing subscription scan, without adding a per-callback search.

`PageEntityModalReplay` resolves only dependencies declared for one card and
configuration generation. It returns an owned, NUL-terminated snapshot using
the supplied arena allocator. A callback may retire the source cache while
reading the snapshot. The snapshot's epoch validation rejects replacement,
reconnect invalidation or cache changes before dispatch. Missing, stale,
overflowed or allocation-failed snapshots are not marked initialized, allowing
a later setup step to retry. Empty available values are delivered normally.

The handoff never injects a network state, registers a channel, sends a request
or fills another persistent coordinator cache. It can allocate one temporary
value snapshot at a time; these are modal setup allocations, not ordinary
navigation allocations. Supply an allocator accounted to the modal memory
budget. Its peak snapshot size is the selected card's largest cached value plus
one terminator. Source/state/allocator/coordinator lifetimes must cover the
synchronous call. Providers and callbacks follow the existing firmware's
non-throwing, main-loop contract; this is not cross-thread synchronization.

This is not production modal selection. The existing control-modal lifecycle
service still needs its real per-driver ports and application wiring. A modal
owner must cancel callbacks before configuration activation, close or invalidate
on reconnect, honor nested-menu/takeover policies, and defer context destruction
until in-flight callbacks finish. The snapshot protects value bytes; it does
not make raw legacy context pointers safe to delete from inside a callback.
Lazy callbacks must be seeded after registration without repeatedly replaying
already initialized temperature/mode fields. Modal callbacks can disable their
own duplicate coordinator retention once their real ownership path is wired.

Host coverage uses the real coordinator, its PageEntitySubscriptions adapter,
the packed state cache and non-owning StringRef callbacks. It checks 1,000 modal
registration/replay/release cycles, stable native channel counts, foreign-owner
isolation, lazy fields, live-value precedence, stale generations, allocation
failure, reconnect/cache replacement, empty strings, reused owners, reentrancy
and cache retirement during a callback. These tests do not create modal widgets
or prove physical control behaviour. The existing coordinator suite remains a
regression gate for the shared callback path.

The [modal lifetime and climate adapter](page-modal-lifecycle.md) now use this
source. They remain unselected; the native adapter test doubles the visual
panel and action transport. Full scene and physical panel qualification are
still required.
