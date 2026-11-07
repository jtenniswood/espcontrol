# Configuration-owned alarm history

`PageAlarmStateRuntime` owns alarm state and countdown history independently of
visible widgets. Sources are deduplicated by entity and control/action policy;
those policies remain separate because the current control and action drivers
restart delay anchors differently. A prepared card-to-source map supports bounded
reads, and alarm children inherit their immediate parent's entity/options when
empty. The dependency graph also declares the current control driver's friendly
name source when an empty label uses name mode.

The owner distinguishes the initial configured icon from the first state or arm-mode
report, including an empty mode report. Known captions use existing translated ROM strings; unknown states use bounded
sentence casing. One control history supports persistent and transient label modes,
including first-terminal seeding, translated-caption equality and the three-second
return to the steady label. Raw state/arm-mode comparisons retain all 32 report
bytes, including embedded NULs. Caption history likewise retains those bytes for
comparison; views must display only the prefix before the first NUL, matching the
current C-string label API. Other status owners retain their existing NUL behavior.

Delay parsing preserves the current short-field digit scanning and HH:MM:SS rules.
Malformed colon arithmetic that cannot fit an integer is rejected rather than
invoking overflow. Remaining time uses unsigned tick differences; progress uses
wide intermediate arithmetic. The application polls this owner even when no alarm
is visible. Countdown seconds, label expiry and takeover release advance its
revision without an LVGL timer.

A triggered alarm or positive arming/pending countdown can request the critical
display. The owner records a configuration-only origin for hidden alarms; it does
not manufacture a visible widget binding. Updated eligible sources replace the
selected alarm in current registration order. Expiry or an unavailable report
releases that selected source without falling back to an older eligible alarm.
Control arm-mode-only reports do not steal a takeover; action arm-mode reports
retain their existing refresh behavior.

Reconnect keeps the last reported critical status and running countdown, as the
current widgets do while the transport clears retained values. Command availability
is tracked separately: a connection change blocks stale commands until a fresh
primary state report. Mode-only reports cannot restore it. The existing initial,
never-reported action availability is retained before invalidation. Scene/service
integration must also check the live connection, current cache budget status and
full originating intent. A fresh unavailable report can release a retained trigger.

The owner prepares before the shared graph, keeps active and candidate histories
separate and promotes private generations including zero. Candidates never publish
takeovers before activation. Same-source candidate copies preserve running delay
anchors, preventing a configuration replacement from extending a countdown. Failed
or discarded preparation preserves the active configuration; empty documents free
source storage. Default source/key budgets are host integration limits pending
physical qualification, not an advertised device capacity.

All sources, interned keys and card indices occupy one fallible arena per
configuration. The owner creates no widgets, subscriptions, UI callbacks, LVGL
timers, service requests or PIN storage. Counters report fixed/active/staged/peak
bytes, source counts and preparation failures. Pending action acknowledgements,
PIN entry, demand-created alarm controls, critical scene routing and view bindings
remain separate work; callers must not dispatch UI or services from observations.

Generated tests compare 102,240 pure policy/delay cases and 180,000 report sequences
with actual production handlers across nine languages, including caption equality,
embedded NULs and the different control/action countdown rules. Additional checks
cover hidden takeover ordering/no fallback, reconnect retention and fresh-command
gating, real graph/private-to-zero publication, copied anchors, staged isolation,
1,000 allocation-free replacements on every profile, budgets and cleanup.

Production still selects the legacy renderer. Alarm views/modals, remaining
families, confirmations, scene integration, compiled legacy registry retirement,
physical qualification and the current factory matrix remain required. No device
is flashed and host allocation counters do not establish physical memory savings.
Stored/backup formats and nesting depth are unchanged.
