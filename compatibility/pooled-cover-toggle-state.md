# Cover-toggle status history

`PageCoverToggleRuntime` stores cover-toggle state per distinct entity, independent
of visible widgets. Prepare it before the shared HA graph and include it in the
fixed observer set. Duplicate cards share ordered reports and deadlines. A new
configuration starts with fresh history, seeded by copied HA values; failed or
discarded preparation preserves the active configuration.

The first terminal Open/Closed report seeds history without replacing the normal
label. Changed terminal captions appear for three seconds; movement, unknown and
unavailable captions remain visible. Equal display captions do not restart the
deadline or change its release policy. Comparison preserves the original localized
caption across language changes. Exact lowercase raw states determine the checked
state (closed/closing), open icon (open/opening), and terminal release policy.
The `state_open` adjective translation remains distinct from the generic Open
action translation. Hidden pages retain deadlines; one application-loop poll
expires them without LVGL timers. Reconnect invalidates status to `--`.

Source names occupy one fallible arena per active/staged configuration. Defaults
are 32 distinct sources and 2,048 copied name bytes; the eventual scene must
choose and advertise device-qualified budgets. Each source owns a bounded status
buffer or borrows a firmware-lifetime translation literal. The bounded formatter
matches current whitespace, word case and UTF-8 behavior without allocating.
Memory counters report fixed, active, staged and peak bytes, sources and failures.
No subscription, widget, action or timer is owned by this component.

The generated test extracts current production caption and transient-label
functions, comparing 90,000 translated report/time sequences. Lifecycle checks
cover duplicates, reconnect, unsigned clock wrap, allocation-free updates, source
budgets, failed/discarded preparation, 1,000 replacements, all nine profiles and
real private-to-committed publication including generation zero.

This stage supplies state only. Visible cover-toggle rendering, scene action and
confirmation integration, physical S3/P4 qualification and the current factory
build matrix remain required. Production selection, saved/backup formats and
nesting depth are unchanged; no device is flashed by this stage.
