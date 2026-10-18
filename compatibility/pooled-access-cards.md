# Reusable garage, gate and lock cards

The unselected renderer supports access toggles and direct commands through the
shared cache and `PageAccessStateRuntime`. Pass the owner and access action hook
to the grid port. Missing owners/hooks reject preparation. Garage confirmation
options still reject preparation until the bounded confirmation flow is ported.
Production remains on the existing renderer.

Configured, friendly and translated fallback labels retain their existing
priorities, including empty friendly names. Garage/gate persistent status ignores
the configured steady label and starts at `--`; normal labels use transient
history. Lock captions keep current sentence-case behavior. Direct commands
without status retain their command labels/icons without subscribing to friendly
names or creating status history. Status commands initially show the command's
normal icon, then use configured closed/open icons when state arrives. A custom
on-icon does not change the initial command icon.

Gate controls retain unavailable styling: labels use the theme's disabled text,
the background stays intact and input is disabled. A first unknown cache entry
with revision zero keeps the existing initially enabled behavior; invalidated
unknown entries stay disabled until a fresh primary report arrives; empty, unknown/unavailable reports
and reconnect invalidation disable the control. Garage/lock availability behavior
is unchanged. Exhausted critical state rejects actions. Widget reuse clears
old disabled states before displaying another card.

Press captures the shared graph connection and full binding. Click validates
both and the current command snapshot, then calls a scalar-only access hook;
no borrowed status text escapes through actions. Garage/gate toggles keep their
optimistic checked feedback. Lock toggle decisions carry the exact current
locked flag. The eventual scene must resolve the stored card and use existing
service dispatch/confirmation rules plus its global connection policy.
Navigation, suspension, replacement and reconnect invalidate old input, and
hooks may navigate reentrantly.

Automatic folder indicators include access toggles and garage/gate commands
with status. Direct commands without status and lock commands keep their
existing exclusion. Summaries use primary entity state, not the view's icon or
caption history. Option decoding occurs during preparation; frame reads use
prepared metadata and the owner's card index.

The access options fit existing prepared-definition padding (264 bytes before
and after on the host). A command's initial glyph reuses the pointer storage
used by folder summaries; the two card kinds are mutually exclusive. The port
adds fixed owner/revision/hook fields and disabled-text theme data, with no extra
per-shell fields, callbacks, contexts, widgets or timers. These host sizes do
not establish ESP32 heap/PSRAM savings.

All-profile model and native checks cover toggles/commands, labels/icons,
independent persistent/transient history, availability, exhaustion, stale input,
private-to-zero publication, suspension, reentrant navigation and 1,000
navigation cycles with stable retained resources. Use
`ESPCONTROL_TEST_FAMILY=access` for the focused native suite.

Remaining drivers, confirmations, scene integration, physical S3/working
20-slot P4 qualification and the current factory build matrix remain required.
No device is flashed by this stage. Stored/backup formats and nesting depth are
unchanged. P4-86 remains reserved for its separate USB diagnosis.
