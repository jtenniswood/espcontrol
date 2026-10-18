# Configuration-owned light-temperature state

`PageLightTemperatureRuntime` keeps light-temperature report and thumb history
outside widgets. Visible temperature views and production scene selection are
still unfinished; production uses the existing renderer.

An exact configuration arena deduplicates entity keys, with default limits of
32 sources, 512 eligible cards and 2,048 key bytes including terminators. Each
card retains its configured Kelvin range, colour mode, last Kelvin, painted
fill, drag preview and interaction flags. This small per-card history is needed:
duplicates can have different ranges or an unacknowledged local preview. There
are no widgets, formatted labels, callbacks, timers, HA registrations or actions
in the owner. Diagnostics separate fixed, active/staged/peak bytes, source/card
counts and preparation failures.

Prepare before the shared HA graph and fan out observations through its
observer set. Private preparation adopts the committed generation, including
zero. Discard/failure preserves live records; empty configurations release the
arena. A replacement seeds actual HA history rather than copying a released
preview. Copied snapshots cannot overwrite a newer live report of the same
field. Reconnect clears old state and interaction and requires a fresh report.

Kelvin reports are remembered while off and applied when an on report arrives.
An off report empties the fill without losing the Kelvin value. Input can
optimistically turn on its own card while waiting for HA's echo. Shared reports
update hidden cards too; duplicates retain independent previews. Drag labels
round to 50 K, clamped to the configured range, matching the existing helpers;
commands retain the unrounded target. A same-position press does not invent a
numeric overlay. HA updates may move the thumb during a drag without replacing
its existing drag label. Release and cancellation hide that label; navigation
preserves the last painted preview. Invalid/unavailable state cancels input.

Range parsing keeps the existing valid/default range rules with bounded,
overflow-checked integer conversion. Finite numeric Kelvin prefixes are accepted
within 64 bytes; nonfinite or unrepresentable reports are ignored. Empty entities
cannot begin a command. The future view must validate full binding/connection,
fail closed on exhausted graph data and dispatch through the original card's
existing service policy only after a valid release.

Host reference tests extract the production report and input callbacks and
compare 40,000 sequences across default, narrow and custom ranges. Lifecycle
tests cover independent duplicates, cached/live report ordering, shared graph
replacement, generation-zero publication, discard/allocation failure, budgets,
all nine profiles, allocation-free updates and 1,000 replacements. Sanitizer and
SDK syntax checks do not establish physical layout or heap/PSRAM savings.

No device is flashed by this stage. Remaining view ports, scene integration,
physical S3/working 20-slot P4 qualification and the complete factory build
matrix remain required. Saved documents, backups and nesting depth are unchanged.
P4-86 remains reserved for its separate USB diagnosis.
