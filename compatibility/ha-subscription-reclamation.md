# Reclaiming Home Assistant subscriptions

This is a prerequisite for the pooled page renderer. It does not enable nested
subpages, change saved configuration, or complete the entity-state migration.

Previously, removing cards released their callbacks but retained their channel
records and native API registrations until reboot. Replacing configurations
with different entities therefore accumulated registration storage.

The production transport now owns removable registrations. Each native callback
contains a firmware-lifetime token; retired tokens never operate a replacement
channel or a destroyed coordinator. Channel indices can be compacted while
retained reads, shared scopes and current callbacks keep their association.

Registration changes run from `EspControlApp::loop`, outside native API dispatch.
Retirement disables delivery immediately. Native entries are removed before
their borrowed names are freed. Name and registry storage uses the existing
external-first allocator, and empty/history-sized containers are reclaimed.
The coordinator also uses that allocator for channel names, retained values
and replay snapshots. Short strings can remain inline in their containing
record; longer strings prefer external RAM and retain the existing internal-RAM
fallback. This changes placement, not which consumers retain values or the
aggregate state budgets. Allocation exhaustion still follows the existing fatal
STL allocator contract; it is not a new fallible preparation guarantee.

Retained-state replay takes a local snapshot so an owner can retire its cache
without invalidating the state view it is consuming. Fresh metadata requests
encode length-delimited borrowed views synchronously, avoiding a conversion
back to default-allocator strings. Older host transport policies can continue
to accept `std::string` arguments.

ESPHome 2026.10.2 has no public unsubscribe operation. A small version-pinned
bridge mutates its public subscription-record view from the main loop. It
identifies EspControl entries by their exact owned name-buffer addresses,
preserving foreign registrations even when their entity/attribute text matches.
Host transport tests derive their version header from `.github/esphome.env`.
The guard accepts the audited 2026.10.1/2 API and rejects an unaudited patch;
an old hard-coded host version cannot conceal a repository pin mismatch.
When the persistent list changes, it marks only authenticated Home Assistant
sessions for disconnection. The API server closes them before another positional
cursor step; their peers clean up connection-owned listeners, then enumerate
the changed list on reconnect. Log, diagnostic and unnamed clients stay
connected. An unchanged maintenance tick does not request another disconnect.
This can briefly interrupt HA connectivity when adding or removing native
channels; navigation within an already prepared graph does not change that list.

The audited 2026.10.2 public disconnect hook marks removal without mutating the
active-client vector, and the server skips marked clients before dispatch. The
HA listener lifecycle was checked against primary upstream
[manager](https://github.com/home-assistant/core/blob/f4e3a420d981b8a523fad8614923ffbc18494741/homeassistant/components/esphome/manager.py)
and [entry data](https://github.com/home-assistant/core/blob/f4e3a420d981b8a523fad8614923ffbc18494741/homeassistant/components/esphome/entry_data.py)
sources: repeating a persistent request adds another listener, while disconnect
runs their cleanup callbacks. This is source/host evidence, not qualification of
the user's installed HA version. Audit this bridge when changing the pinned SDK.

Existing grid, climate and media value-refresh helpers now schedule one-shot
requests instead of restarting persistent enumeration. Maintenance coalesces
requests and visits one entity/attribute pair per loop tick, sending only to
eligible HA clients. Backpressure retries that pair; list compaction resets the
cursor before another request. Empty wizard IDs are skipped. A refresh alone
neither reconnects HA nor installs another remote listener. The cursor and flag
use fixed scalar storage, with no additional retained value cache.

The actual pinned HA subscription, one-shot and disconnect method bodies were
also executed with state/event doubles: 100 persistent replays install 100
listeners, disconnect removes them, and 100 one-shot reads retain only the
single original listener. Production-transport host checks cover deferred and
coalesced refreshes, pacing, two HA clients, backpressure, changed-list cursors,
null attributes, empty IDs and backend unavailability. Weakened persistent,
unpaced and obsolete-cursor implementations fail the regression checks.

Host coverage exercises 1,000 disjoint configurations, deferred add/remove
during dispatch, late callbacks, owner destruction, shared channels, retained
reads after compaction, reconnect invalidation and foreign subscriptions. The
production bridge is also tested with native-style records and cursors. The
actual external allocator is exercised with long names, retained values, replay
snapshots, invalidation during a borrowed-state callback, length-delimited
requests, reclamation and internal fallback. Address
and undefined-behaviour sanitizers cover callback/name lifetimes.

Physical qualification must compare settled counts after the loop maintenance
message, rather than the earlier `grid-complete` message. Track `owned`,
`pending`, `registry_bytes`, `upstream` and native `capacity`, alongside whole
device heap/PSRAM. Repeated disjoint card sets should return to the original
configuration's registration requirements without restarting. Image allocation
peaks remain a separate measurement. The remaining PageStore dependency
mapping and reusable renderer gates still apply before enabling nesting.
