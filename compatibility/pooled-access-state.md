# Garage, gate and lock status history

`PageAccessStateRuntime` moves status history from eager hidden widgets into one
fallible arena per active/staged configuration. Prepare it before the shared HA
graph and include it in the fixed observer set. It owns no subscription, widget,
action context or LVGL timer. Production remains on the existing renderer.

History is shared by entity and policy: transient garage/gate, persistent
garage/gate, or lock. Duplicate cards share a record; persistent and transient
labels keep independent deadlines. Commands without a status label need no
history. Lock commands also need no history; their direct service is determined
by the stored command rather than a retained toggle state.

The first terminal report seeds transient history without replacing the steady
label. Changed terminal captions revert after three seconds; movement/unknown
captions remain visible. Persistent labels show the first report and never
expire. Equal display captions do not alter release policy or deadlines. Garage
and gate captions retain the existing localized `state_open` adjective, distinct
from the Open action. Lock captions keep the current sentence-case text without
adding translation. Caption callbacks return their input or firmware-lifetime
literals; untranslated text uses a fixed UTF-8-aware buffer. Live language
changes compare with the previously displayed caption.

Exact lowercase raw states determine checked/icon state and whether a lock is
locked. The separate availability flag uses the existing trimmed, case-insensitive
unknown/unavailable predicate. Reconnect resets all records to `--`, unavailable
and unchecked. One application-loop poll expires deadlines, including on hidden
pages; unsigned elapsed time handles clock wrap. Copied snapshots seed fresh
configuration history, while failed/discarded preparation preserves active state.
Private prepared generations promote at commit, including initial generation zero.

A prepared card-to-history index makes frame reads allocation-free without
reparsing options. Defaults allow 64 entity/policy records and 4,096 copied name
bytes, including terminators; the same entity in different policies consumes
separate records/name bytes. Memory counters report fixed, active, staged and
peak arena bytes, history count and preparation failures. The eventual scene
must choose device-qualified budgets rather than treating these defaults as
advertised configuration capacity.

Generated reference tests extract current production state/caption and transient
label functions. They compare 270,000 report/time sequences across three policies
and nine languages, including live language changes. Other checks cover mixed
policies for one entity, command exclusions, copied snapshots and publication,
generation zero, duplicate consumers, exact deadlines, reconnect, clock wrap,
allocation-free updates, budgets, failed/discarded preparation, empty replacements,
all nine device profiles and 1,000 disjoint replacements.

Visible access rendering, confirmations, service dispatch, remaining drivers,
scene selection, physical S3/working 20-slot P4 qualification and the current
factory build matrix remain required. No device is flashed by this stage. Stored
and backup formats and nesting depth are unchanged; host counters do not prove
physical heap/PSRAM savings.
