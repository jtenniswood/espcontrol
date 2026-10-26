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

ESPHome 2026.10.0 has no public unsubscribe operation. A small version-pinned
bridge mutates its public subscription-record view from the main loop. It
identifies EspControl entries by their exact owned name-buffer addresses,
preserving foreign registrations even when their entity/attribute text matches.
It restarts authenticated clients' positional subscription enumeration after
changes. Audit this bridge when changing the pinned ESPHome version.

The 2026.10.0 audit compared the actual SDK sources with 2026.9.1: subscription
records and state-response dispatch are unchanged. Enumeration still uses a
positional cursor. The new wizard-input path remembers state subscriptions and
skips unconfigured empty IDs; invoking the public enumeration request sets
that flag as the SDK expects and restarts its cursor. The native-style fixture now retains dynamic
foreign name storage across compaction and checks this enumeration flag. The
version guard remains exact so a later SDK change requires another audit.

The native protocol has no remote-unsubscribe message. Home Assistant may keep
announcing retired entities until its next reconnect; those replies are ignored.
This stage reclaims device-owned registrations, not Home Assistant's own memory.

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
