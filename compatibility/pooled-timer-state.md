# Configuration-owned timer state

`PageTimerRuntime` keeps timer countdown anchors and per-card completion history
outside widgets. [Reusable timer views](pooled-timer-cards.md) supply bounded
confirmation input; production scene selection remains unfinished and production
uses the existing renderer.

An exact configuration arena shares each `timer.*` entity's state, duration,
remaining anchor and absolute finish epoch. Each card retains its confirmation
settings, natural-finish grace timestamp and whether its cancel command was
accepted. This preserves duplicate-card behaviour without storing widgets,
labels, callbacks or HA registrations. Defaults are 32 sources, 512 cards and
2,048 key bytes including terminators. Diagnostics separate fixed,
active/staged/peak bytes, source/card counts and preparation failures.

The application injects monotonic and epoch clocks and calls one owner loop.
A single 250 ms revision cadence replaces the need for per-card tick timers;
reads derive remaining seconds from current time. An absolute HA finish time
wins when wall time is synced; otherwise the monotonic remaining anchor is
used. Paused timers keep their reported remaining value. A natural active-to-
idle transition holds zero for five seconds before returning to the configured
duration. Accepted cancellation suppresses that grace for its own card.

Prepare before the HA graph. A matching staged source copies actual report
anchors into its own arena, then ignores older copied fields. This prevents a
configuration replacement from restarting the countdown at a stale cached
remaining value. A live repeated remaining report still resets its anchor,
matching the existing callbacks. New card records do not inherit another
configuration's local cancellation/completion history. Private preparation
adopts the committed generation, including zero. Discard/failure preserves
active history; empty replacements release storage. Reconnect clears stale
state and commands remain unavailable until new state arrives.

The existing renderer and new owner share bounded duration/ISO parsing and
countdown formatting. Valid day/hour forms, offsets and fractional seconds keep
the existing rules. Parsing uses a fixed 96-byte buffer; invalid/oversized values
produce zero. Remaining intervals are clamped to the representable display
range. Confirmation timeouts retain the default three seconds and 30-second
ceiling. The owner returns START/CANCEL/NONE; it dispatches no action. The
application scene must validate the full binding, connection and graph status,
and record a cancel result only after its existing timer service policy accepts
that command. The future view must bound/cancel confirmation by its live binding.

Host tests compare 20,000 clock/report/action sequences against the production
callbacks, with deterministic clocks. They cover countdowns, pause/resume,
completion/cancellation, offset parsing, wall-clock changes and monotonic wrap.
Additional tests cover shared-graph replacement without anchor reset, live
repeated reports, generation-zero publication, discard/allocation failure,
budgets, all nine profiles, allocation-free updates and 1,000 replacements.
The existing timer/confirmation regression suite also remains applicable.

No device is flashed by this stage. Remaining view ports, scene integration,
physical S3/working 20-slot P4 qualification and the complete factory build
matrix remain required. Host state checks and SDK syntax compilation do not
establish layout or heap/PSRAM savings. Saved documents, backups and nesting
depth are unchanged. P4-86 remains reserved for its separate USB diagnosis.
