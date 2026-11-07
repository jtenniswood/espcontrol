# Reusable timer cards

The unselected renderer now renders Timer cards using `PageTimerRuntime` and the
shared HA graph. Production scene selection, remaining drivers and physical
qualification are still unfinished; production uses the existing renderer.

Cards show the existing countdown format with no icon or unit, normal sensor
font, an active checked background, and a configured/friendly/translated Timer
label. Unsupported entities and unavailable/exhausted state show `--:--` and
cannot dispatch commands. Eligible child timers light the parent folder only
while active; explicit folder entity indicators retain their existing policy.

Idle/paused clicks request START; active clicks request CANCEL. The scene hook
returns whether its existing service policy accepted the action. Only accepted
cancellation suppresses that card's natural-finish grace. The hook receives a
validated full binding and action; the view adds no service registrations or
callbacks with HA. A reentrant navigation cannot send a command for the new
binding, and a configuration/connection change drops old cancellation results.

Optional two-tap cancellation uses the shell's bounded confirmation state and
the owner's application-loop clock, without an LVGL timer or captured closure.
A pending prompt follows updated friendly names when dismissed. Expiry,
navigation, suspension, reconnect and nonactive state invalidate it. A shared
source revision also catches active→idle→active cycles between paints, so the
old second tap cannot confirm a new state. Each visible shell adds one revision
field; source state adds a confirmation invalidation counter. No hidden widget
or per-card confirmation context is retained. Translations of Confirm are
budgeted during preparation.

The fixed bank retains the existing label/value geometry and font restoration.
Preparation requires the timer owner and scene action hook. Immediate prompt
painting does not consume pending updates for other cards. The normal renderer
polls the single owner before forced navigation/reconnect frames, so countdowns
and the five-second finish grace remain current on hidden pages.

Model tests cover defaults/raw names, display formats, checked state, timer-child
versus explicit folder predicates, unavailable/exhausted data, confirmation
invalidation and allocation-free reads on all nine profiles. Native tests use
real private-to-zero publication, countdown/grace behaviour, accepted/rejected
cancellation, prompt expiry, friendly-name changes, unseen state cycles,
reconnect between press/click, stale bindings, reentrant navigation, 1,000
confirmation cycles with no timer additions and 1,000 navigation cycles with
stable retained resources. Use `ESPCONTROL_TEST_FAMILY=timers` for that suite.

The [deferred timer command owner](pooled-timer-commands.md) now admits these
view callbacks without acknowledging queue admission as a sent cancellation.
Remaining view ports, scene integration, physical S3/working 20-slot P4
qualification and the full factory build matrix remain required. No device is
flashed by this stage. Host retained bytes are not physical heap/PSRAM savings;
saved documents, backups and nesting depth are unchanged. P4-86 remains reserved
for its separate USB diagnosis.
