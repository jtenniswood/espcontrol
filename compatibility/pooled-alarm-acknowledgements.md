# Alarm command acknowledgements without widget timers

`PageAlarmAckRuntime` tracks successfully submitted alarm actions independently of
widgets and modals. Each configured alarm/control-action card has one 16-byte
record, allocated in a fallible arena during configuration preparation. Records
contain a card ID, mode, timestamps, connection and boolean flags, never PIN text,
a service request, widget pointer or timer. No record is allocated on a click.
The default record budget is a host integration limit pending device qualification.

After successful dispatch, the caller invokes `accepted` with the full originating
configuration handle and captured connection. Repeated successful actions on one
card replace that card's previous deadline and code-presence flag. Unsuccessful
service dispatch must not call it, preserving prior pending work. This API records
acknowledgements; it does not authorize or send a command. The caller must first
validate the full press/modal intent, configured action, live connection, fresh
state/current cache status and service availability.

State and arm-mode reports clear a pending action when the existing alarm policy
considers it progressed. Otherwise, after 3.5 seconds the owner exposes either a
code-rejection or unchanged-alarm failure. It rechecks current state at expiry,
matching the legacy timer callback, including already-achieved state and arming
progress. A report arriving before a due timer is processed can still cancel the
pending action. Report data is merged before deadline checks, so reversed observer
registration cannot manufacture a PIN failure at that boundary.

Failures are scalar events consumed by the application loop, ordered by deadline
and acceptance order. Consuming an event cannot write to an old widget. A failure
already produced before a later report remains available, like an already-fired
legacy timeout. A fresh command on that same card supersedes its prior pending
feedback. The eventual UI translates the two existing failure messages.

Reconnect cancels pending work and unconsumed failures from the old connection.
Configuration activation cancels old work, including generation zero, while failed
or discarded preparation leaves the active configuration's work intact. Candidates
have no pending commands, and copied candidate state cannot acknowledge active
commands. The source owner still retains critical alarm history independently.
Empty documents release record storage; all operations after prepare allocate no
C++/arena memory.

Register source state before acknowledgements in the observer set. Put the
acknowledgement owner before source state and graph in resource retirement order,
and use the same application millisecond clock. Poll `loop` even without visible
alarm cards, then drain `take_failure` to the current scene. Only successfully
validated, submitted commands may create pending records; PIN entry/deferred
submission and modal/view ownership remain separate work.

Generated tests compare 200,000 command/report/timeout sequences with actual legacy
acknowledgement functions, including unsigned tick wrap. Other checks cover multiple
hidden-card failures, deadline/acceptance ordering, reversed observer timing,
invalid/stale intents, reconnect cancellation, real graph/private-to-zero publication,
staged isolation, budgets, empty documents and 1,000 allocation-free replacements
on every device profile.

Production still selects the legacy renderer. Alarm views/PIN/control modals,
critical scene routing, remaining families, confirmations, compiled registry
retirement, physical qualification and the current factory build matrix remain
required. No device is flashed; host allocation checks do not establish physical
memory savings. Stored/backup formats and nesting depth are unchanged.
