# Reusable cover-toggle cards

The unselected renderer supports cover Toggle cards using
`PageCoverToggleRuntime`. Prepare this owner before the HA graph and include it
in the observer set, then pass it to the reusable grid port. Preparation rejects
missing owners or toggle hooks. The existing eager renderer remains selected.

Cards retain configured/friendly/translated Cover labels, including explicitly
empty friendly names. Status captions borrow the owner's localized history;
Open/Closed transitions expire after three seconds while movement/unknown
captions remain visible. Navigation never restarts the deadline. Labels use
bounded reusable buffers, with no per-card callback, context, timer or widget.
The port adds one fixed owner pointer and revision, with no extra shell fields.

The checked background follows exact closed/closing reports. The open icon
follows exact open/opening reports independently. Configured icons override the
default Blinds/Blinds Open pair. Automatic folder indicators follow the existing
primary-state predicate, rather than the toggle's inverted checked state.
No slider, fill, number, unit or push transition is shown.

Press captures the shared graph connection and complete binding. Click validates
both and rejects empty targets or exhausted state before using the existing
toggle scene hook. The current card's optimistic checked feedback is preserved.
The existing cover-toggle path does not require a primary availability report;
the eventual scene must additionally enforce its global connection policy and
resolve the original configuration. Navigation, suspension, configuration
replacement and reconnect invalidate old input. Hooks may navigate reentrantly.
Other card confirmation and dispatch policies remain separate integration work.

All-profile model tests cover labels, source history, independent checked/icon
state, exact expiry, empty friendly names, allocation-free reads, exhausted data
and reconnect. Native tests use real private-to-zero publication, configured and
duplicate cards on hidden pages, folder predicates, stale presses, exhaustion,
suspension, reentrant navigation and 1,000 navigation cycles with stable retained
resources. Use `ESPCONTROL_TEST_FAMILY=cover_toggles` for the focused suite.

Scene selection, remaining drivers, physical S3/working 20-slot P4 qualification
and the current factory build matrix remain required. No device is flashed by
this stage. Host counters and SDK syntax checks do not establish physical layout
or heap/PSRAM savings. Stored/backup formats and nesting depth are unchanged.
P4-86 remains reserved for its separate USB diagnosis.
