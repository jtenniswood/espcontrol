# Covered grid input

This dependent #1825 runtime stage provides the overlay input boundary for the
pooled renderer. Production still selects the legacy renderer. The application
scene, production cutover, device qualification and nested pages remain pending.

`PageGridInput<Slots>` borrows the prepared view, renderer and shared command
queue. An overlay's callback calls `cover(true)` to immediately block input,
clear captured button presses and cancel queued commands, including released
slider previews. It performs scalar/history changes only: no widget destruction,
layout, rendering or producer callbacks. `cover(false)` cannot revive old input
if an open/close pair occurs between maintenance ticks.

Safe main-loop maintenance calls `settle()` before dispatching commands or
re-enabling grid input. It cancels held pointer gestures, resets the input device
until release and clears pressed visuals and unfinished timer confirmation.
Slider PREPROCESS guards and button callbacks reject touch, key, rotary and
value-change events throughout the covered/pending/settling phases. Cancellation
cannot collect while LVGL dispatch is active. A reentrant cover during collection
remains pending for a later safe pass.

This boundary keeps the original full binding and image/artwork ownership alive;
it does not suspend/unbind the renderer for a normal overlay. The scene uses
`valid()` for grid input/command/modal origins, plus its display/wake/PIN guards,
and `bound()` for read-only ownership. Full display/screensaver takeovers still
use the renderer's suspension lifecycle. All these objects outlive callbacks.

Accepted seek, volume, speaker-group and access/cover feedback validates the
read-only full binding rather than the grid-input gate. This lets an admitted
panel update its covered originating card. Caller authorization remains the
accepted command/matching modal-opening boundary; these functions do not send a
service. Navigation, replacement, reconnect and source identity checks continue
to reject stale feedback.

Cancellation preserves accepted previews on cards without an unfinished drag.
Unfinished number drags return
to reported state. An unfinished Kelvin drag also restores reported temperature
through the existing guarded rollback, instead of leaving an unsent local value
on screen. A released-but-unsent edit is rolled back by cancelling the command
queue. A newer report or binding cannot be overwritten by an old cancellation.

Memory is fixed per application: the gate has three borrowed references (24 host
bytes), and the view adds three flags in existing host padding. Host view sizes
remain 3,664/5,032/7,768/10,048 bytes for 6/9/15/20 shells; the per-shell stride
stays 456 bytes. There are no per-card fields, new queues, subscriptions or
widgets. Target ABI sizes and physical heap/PSRAM savings remain to be measured.

Native sanitizer tests exercise actual pointer input, fast cover/uncover pairs,
queued edit cancellation, held percent/number/Kelvin drags, preserved accepted
previews, covered-origin media feedback, timer confirmation reset and cover
notification during drawing. Existing 1,000-cycle navigation/command fixtures
also check retained resources and stale input. These are host LVGL tests, not
physical S3/P4 qualification. Keep P4-86 reserved for separate USB diagnosis.
