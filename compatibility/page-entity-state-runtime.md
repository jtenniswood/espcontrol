# Configuration-owned entity state

This is the next host-tested part of the page runtime migration. Production
continues using the existing grid. No new state budget, card capacity or nested
page capability is advertised by this change.

`PageEntityStateRuntime` prepares a card-to-entity/attribute graph from an
explicit driver dependency source. Equal pairs share one channel; multiple
cards and duplicate references share the same value. Names, graph edges and a
packed state cache live in one fallible arena per configuration generation.
Dependencies, channels and state bytes have caller-supplied bounds. These are
preparation budgets, not qualified device limits.

The existing binding coordinator can carry both active and prepared owners on
one native transport channel. Configuration consumers disable its additional
replay/cache copies; shared widget and modal consumers retain their existing
behaviour. Shared values are copied from the active graph into its replacement,
and subsequent announcements update both until activation. There is no widget
address in the configuration graph.

`PageStoreRuntime` optionally reserves these resources before storage commit.
A preparation failure cancels partially attached subscriptions and frees the
staged arenas while leaving the active generation intact. Activation changes
the generation and retires the previous owner without allocating. Resource
retirement runs before the corresponding PageStore arena is freed.

Slot revisions reject callbacks from discarded or reused staging slots. Card
queries require the committed configuration generation. A connection-generation
change invalidates active and staged state, including when a query is the first
operation after disconnect. Unavailable/empty values remain distinct from
missing state. Values that exceed the supplied aggregate cache budget are
marked `EXHAUSTED`; they are never silently truncated or presented as fresh old
state. A later fitting value recovers normally.

Card queries use per-card row offsets rather than scanning the complete graph.
The retained index costs four bytes per stored card plus one sentinel when the
graph is nonempty. Driver declarations arrive in card order and require no
extra grouping allocation. Other sources retain their per-card declaration
order through temporary stable grouping before subscriptions are attached; its
scratch is included in the preparation peak and is released immediately. A
scratch failure leaves the active generation intact.

State delivery compacts the reserved cache without allocating. Changed values
of the same byte length update in place; identical values remain a no-op. Returned views
are valid only until the next delivery, disconnect or replacement; renderers
must pull/copy the required values from the main event loop. This is not yet a
claim that the coordinator's complete dispatch path or the existing renderer is
allocation-free. Arena counters exclude coordinator/native storage, fixed owner
objects and temporary dependency-source work.

Host tests cover staged visibility, allocation-free activation and cache
updates, shared dependencies, UTF-8, unavailable values, packing/exhaustion and
recovery, partial subscription failure, arena failure, reconnect invalidation,
1,000 generation replacements and 10,000 reference-checked cache updates.
They also exercise the real binding coordinator adapter. These are model tests,
not physical-device memory or visual-parity results.

An optional `page_entity_state_benchmark` target reports lookup and equal-width
update timings, fixed owner size and retained arena size for maximum S3 and
20-slot P4 layouts with 24 synthetic attributes per card. It has no timing pass
threshold and does not qualify device budgets or represent measured device
heap use.

The [card dependency declarations](page-card-dependencies.md) now cover the
registered drivers and have host corpus/max-layout coverage. Before production
wiring, finish physical parity for every mode, hidden-page behaviour, parent
summaries and panel-level consumers. Qualify complete preparation peaks (including coordinator/native
allocation behaviour), choose and advertise measured device budgets, and finish
the reusable renderer. Keep the existing PageStore/renderer gate in place;
retaining this graph alongside all eager page widgets is not the intended
production architecture.
