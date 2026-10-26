# Deferred basic-card actions

`PageCardCommands` is the basic-action owner for the pooled scene migration.
Production still uses the existing renderer. It does not enable nested pages,
select the new scene, retire compiled legacy handlers or establish device memory
savings. [Slider dispatch](pooled-slider-commands.md) and
[media grid actions](pooled-media-commands.md) share this queue. Access, cleaning
and other control routes still need their final main-loop dispatch integration. [Timer actions](pooled-timer-commands.md) also share the
FIFO, with cancellation history updated after an accepted send.
The [shared policy router](pooled-command-routing.md) composes these families
for mixed configurations behind one native adapter.

An eight-entry fixed FIFO accepts validated grid bindings and the connection
captured with input. Admission allocates nothing and retains no configuration
strings, callbacks, timers or per-card objects. A full queue rejects the new
intent; it never overwrites an earlier accepted click. Maintenance processes one
intent at a time outside LVGL/native/coordinator dispatch. HA actions recheck the
captured connection and service availability; local actions, internal relays,
webhooks, screensaver and screen lock remain usable offline.

The scene must supply a read-only full binding validator that also rejects
covered, locked and wake-guarded input. Suspend/invalidate bindings and cancel
the FIFO on takeover so a hidden tap cannot revive when the display resumes.
Place this owner before the graph/store in the configuration resource group.
Activation cancels queued input even when the numeric generation is unchanged.

The owner copies all nine normalized configuration fields only when dispatching.
Native helper calls receive that owned snapshot, so a synchronous local action
or transport callback can replace and free the old configuration safely.
`DISPATCHED` counts calls to the existing helpers; it is not a transport delivery
or Home Assistant acknowledgement count.

The native adapter reuses existing action/script-field, relay, local-action,
webhook and screensaver/lock helpers. Root Push events retain the default
`Push <slot>` label and `slot` field; child Push events retain their translated
default label and omit that field. Required confirmations and selectors stay
with the demanded modal owners. For a direction that requires no confirmation,
the selected explicit on/off or cover open/close direction is retained across
state changes in the queue. A shared bounded state predicate keeps this decision
identical to the displayed toggle state.

The host fixture runs 1,000 five-action FIFO cycles per profile across all nine
profiles, checks allocation-free admission and stable graph registrations,
offline local actions, overflow, stale bindings, reconnect, same-generation
replacement and owned strings during reentrant arena destruction. The optional
native fixture uses real LVGL callback dispatch and the native adapter with
transport/helper spies, checking readiness, lock guards, helper routing and
1,000 root/child/custom Push event cycles with French child labels. This is not
a physical display, real HTTP/HA delivery or current SDK firmware build test.
