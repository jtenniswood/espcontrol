# Reusable vacuum and mower cards

The unselected renderer reads Vacuum and Lawn Mower cards directly from the
shared HA cache, without configured-card contexts, callbacks or another state
owner. Mode aliases/defaults, labels, generated-icon recognition and service
selection follow the current production helpers. Production scene selection
remains unfinished; the existing renderer is still selected.

Status cards are read-only and translate recognized state captions, falling back
to configured/default text for other states. Stateful controls retain configured
labels while icons follow the current state. Saved generated icons from another
mode remain automatic; custom icons stay fixed. A direct vacuum command without
state dependence retains its initial icon even if a folder indicator or another
card shares the entity's state channel. Only vacuum Start/Stop has a checked
background, and only for an exact cleaning report. Mower cards keep their
existing unchecked background.

A never-reported cache entry preserves initial labels/icons and the existing
empty-state service policy. Reconnect-invalidated entries show unavailable state;
stateful actions fail closed on exhausted data. Normalized availability retains
the existing unknown-state exceptions for button/select domains, while captions,
icons and services otherwise use exact raw state. An unavailable mower rejects
commands; vacuum modes keep their established fallback service rules. Direct
commands do not acquire a state dependency merely because a folder summary reads
that entity. Clean Area requires a nonempty original area target; the scene
resolves and sends the stored target unchanged.

Press captures the complete binding and connection. Click selects the service
from the current snapshot and passes only its firmware-lifetime service literal
to the scene. Navigation, suspension, replacement and reconnect invalidate old
input. The eventual scene must also enforce its global connection policy and
use existing request construction, including Clean Area's target data. Reentrant
navigation is safe after dispatch.

All cleaning children retain their legacy folder contribution. Vacuum children
use the ordinary primary-state predicate; mower children use the existing
mowing/returning predicate independently of their icon and checked state. No
additional HA registrations are created for views.

The port prepares at most one six-glyph lookup table per active/staged
configuration when automatic stateful icons require it. Reads and navigation
reuse it. Typed mode options fit existing prepared metadata; no extra shell
fields, widgets, timers or callback contexts are retained.

Generated reference tests compare 373,464 combinations against actual legacy
contexts and helpers: modes/aliases, raw states, configured/default labels,
generated/custom icons, entity-domain availability and nine languages. Model
and native checks cover all nine profiles, private-to-zero publication, readonly
status, independent folder predicates, direct command isolation, missing area,
exhaustion, reconnect, stale input, suspension, reentrant navigation and 1,000
navigation cycles with stable retained resources. Use
`ESPCONTROL_TEST_FAMILY=cleaning` for the focused native suite.

Remaining drivers, confirmations, scene integration, physical S3/working
20-slot P4 qualification and the current factory build matrix remain required.
No device is flashed by this stage. Stored/backup formats and nesting depth are
unchanged. Host counters and SDK syntax checks do not establish physical layout
or heap/PSRAM savings. P4-86 remains reserved for USB diagnosis.
