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
Retained-state replay takes a local snapshot so an owner can retire its cache
without invalidating the state view it is consuming.

ESPHome 2026.9.1 has no public unsubscribe operation. A small version-pinned
bridge mutates its public subscription-record view from the main loop. It
identifies EspControl entries by their exact owned name-buffer addresses,
preserving foreign registrations even when their entity/attribute text matches.
After a batch changes the native list, it marks authenticated Home Assistant
clients for disconnection through the public API hook. The server closes them on its next
loop before advancing obsolete positional cursors. Their fresh connections
enumerate the current list. Unchanged lists do not trigger a reconnect. Audit
this bridge when changing the pinned ESPHome version. Diagnostic and unnamed
clients stay connected, matching the application's HA-specific connection policy.

The native protocol has no remote-unsubscribe message. Replaying the list on a
live connection would install duplicate Home Assistant listeners. Reconnecting
lets Home Assistant release all connection-owned listeners before subscribing
again, including those for retired entities. Expect a brief API interruption
when card replacement changes the registration set; normal state updates and
same-entity rebuilds do not reconnect.

Host coverage exercises 1,000 disjoint configurations, deferred add/remove
during dispatch, late callbacks, owner destruction, shared channels, retained
reads after compaction, reconnect invalidation and foreign subscriptions. The
production bridge is also tested with native-style records and cursors. Address
and undefined-behaviour sanitizers cover callback/name lifetimes.

Physical qualification must compare settled counts after the loop maintenance
message, rather than the earlier `grid-complete` message. Track `owned`,
`pending`, `registry_bytes`, `upstream` and native `capacity`, alongside whole
device heap/PSRAM. Repeated disjoint card sets should return to the original
configuration's registration requirements without restarting. Image allocation
peaks remain a separate measurement. The remaining PageStore dependency
mapping and reusable renderer gates still apply before enabling nesting.
