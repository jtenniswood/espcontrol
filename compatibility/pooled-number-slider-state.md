# Configuration-owned number slider state

`PageNumberSliderRuntime` prepares shared ranges and state for numeric Slider
cards and existing numeric-domain aliases. It supports `number` and
`input_number` entities without reducing them to 100 percentage steps. Visible
number-slider views and production scene wiring remain unfinished; production
continues to use the existing renderer.

Each entity has one copied key, min/max/step metadata, finite state value and
known-field bits. Each eligible card has a small independent thumb/drag record.
There are no widgets, callbacks, timers, formatted strings or HA registrations
in this owner. Exact active/staged arenas default to 32 sources, 512 cards and
2,048 name bytes including terminators. Preparation validates budgets before
allocating; discard/failure preserves active records, and an empty configuration
frees the arena. Memory counters separate fixed, active/staged/peak storage,
source/card counts and preparation failures.

Prepare before the HA graph and include the owner in `PageEntityObserverSet`.
Call its loop on the application loop. New configurations seed their own history
from graph reports instead of inheriting a released thumb. Duplicate cards share
HA values while retaining independent positions during interaction. Matching
reports still reconcile a released thumb even if cache bytes are unchanged.

The existing number-slider policy supplies legal positions, snapping, bounds,
step counts and targets, preserving double precision and its 10,000-position
ceiling. Metadata/state parsing accepts finite numeric prefixes within 64 bytes.
Invalid fields clear their known bit without overwriting the last finite number.
Interaction requires valid primary state plus complete, valid range metadata.
Units and friendly names remain in the shared graph for the future view.

Valid primary reports preserve an active numeric drag and update other cards.
Metadata reports reset the numeric overlay and apply current reported state,
matching the existing callbacks. A separate gesture flag allows release after
an equivalent metadata report. An actual range change or invalid state cancels
the gesture, so an old release cannot send a value using changed limits. The
future view must also validate binding and connection and wait for that pointer
to release before admitting another gesture. Cancel restores current state when
available; invalid state preserves the last visual position while disabling
input. Reconnect clears old range, state and interaction records.

The owner formats no values and sends no actions. `finish` returns a snapped
double only for a current, valid gesture. The eventual scene must consume that
result with the original card and current validated metadata; exhausted graph
data must fail closed in the view/action path. Target formatting must retain the
existing numeric policy rather than formatting the raw double generically.

Host reference tests compare 30,000 state/metadata/drag sequences against the
actual production slider subscription and numeric helpers. They also cover
independent duplicates, repeated confirmations, repeated versus changed ranges,
invalid fields, shared-graph staging, reconnect, budget/allocation failures,
all nine profiles, allocation-free reads/updates and 1,000 replacements.

These are state-owner checks, not physical layout or heap/PSRAM qualification.
Visible ports, production selection, remaining drivers, the complete factory
build matrix and physical S3/working 20-slot P4 qualification are still required.
No device was flashed; saved/backup formats and nested-page support are unchanged.
