# Reusable alarm cards

The unselected renderer supports alarm control and alarm action cards through
`PageAlarmStateRuntime`. Preparation requires that owner and an alarm-intent
callback. The callback receives the full validated binding; it does not retain
borrowed status data, open legacy UI or send a service itself.

Controls preserve persistent status labels (initially `--`), transient name-mode
history, configured/friendly/default names, initial configured icons and optional
dynamic status icons. An empty friendly name remains empty. Unknown state captions
show the same C-string prefix when a report contains an embedded NUL. Action cards
retain configured/default action captions and icons; their checked state follows
the requested mode rather than the overall alarm's active state. Custom on-icons,
units, numeric layout and push animation do not apply to alarms.

Controls use the primary theme surface unless their effective options specify an
off colour. Actions use the ordinary card surface. Both preserve contrast text and
pressed on colours; triggered controls use the existing raw red constant, including
when display RGB correction is enabled. An alarm-only LVGL state selects pressed
colours so that reuse does not change other families' press styles or animations.
The scene must supply corrected primary surface through `alarm_surface`.

Press captures the complete binding and connection. Click rechecks both and reads
the current owner snapshot. Control tiles can request their panel while unavailable, just as the legacy click
handler does; the modal adapter retains its separate availability admission check.
Action cards reject unavailable, invalidated or exhausted critical state. A mode
report alone cannot restore command availability after reconnect. The callback may
navigate reentrantly. The eventual scene must also enforce its global connection
policy, revalidate the stored intent before service dispatch and route PIN/control
UI through the bounded modal owner.

Control entities/options use the existing immediate-parent inheritance rules.
Automatic folder summaries include alarm controls using the ordinary entity
predicate; action children are excluded. Summary state is independent of the
control's active/triggered appearance.

Seven bytes of alarm options fit the existing prepared-definition union: the host
size remains 264 bytes. Dynamic glyph pointers are resolved once into one small
table per prepared document, only when a dynamic control needs it. There are no new
per-shell fields, callbacks, hidden widgets, subscriptions or timers. Fixed owner,
revision and action-hook fields are shared by the port. Static text capacity is
reserved during preparation. Runtime reads and navigation do not allocate.

Generated view tests compare actual legacy captions and glyph selection. All nine
profile model/native checks cover labels/icons, hidden status and takeover state,
press/release, unavailable controls, action freshness, reconnect/navigation guards,
reentrant navigation, folder eligibility, private-to-zero publication and 1,000
cycles with stable retained resources. Use `ESPCONTROL_TEST_FAMILY=alarm` for the
focused native suite.

Production still selects the existing renderer. Alarm control/PIN modals, deferred
submission/acknowledgement feedback, critical hidden-origin scene routing, remaining
families, confirmations, compiled legacy registry retirement, physical S3 and
working twenty-slot P4 qualification and the current factory build matrix remain
required. No device is flashed by this stage. Host sizes and SDK checks do not
establish physical memory savings. Stored/backup formats and nesting depth remain
unchanged; P4-86 stays reserved for USB diagnosis.
