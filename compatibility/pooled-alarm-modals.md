# Demanded alarm control and PIN UI

`PageAlarmModalPort` adapts the existing alarm control and keypad helpers to one
`PageModalRuntime` record. It copies only that opening's configuration, reserves a
bounded gesture arena and borrows profile fonts. No configured alarm keeps a hidden
panel, status-label context, HA registration or LVGL timer. Countdown/progress reads
come from `PageAlarmStateRuntime`; commands and acknowledgements use their separate
owners. Production remains on the existing renderer.

Installation requires the legacy renderer retired, no active legacy alarm/PIN UI,
no legacy takeover/deferred timer and no competing lifetime observer. Supply the
current per-card palette/profile fonts through the required style resolver. Source
entity/options inherit from the immediate parent; palette resolution still uses the
original child configuration, matching the existing driver. Control primary-surface
ownership does not apply to a critical panel opened from an action card.

An explicitly unavailable/never-reported control does not open, matching the existing
modal admission check. A panel already open can become readonly while unavailable.
After reconnect, retained available status can be viewed while command freshness
waits for a primary report; an arm-mode report alone cannot restore commands. Direct
action cards with required PIN use their configured mode; those without PIN use the
grid command path and do not allocate a modal context.

Control-to-PIN transition keeps the same opening and demanded context. The selected
mode is fixed to that PIN session. The normal control-close callback during shell
replacement does not retire the opening. Preprocessing gesture guards cover the
control rail, keypad and back buttons before returning to the event loop. Missing
guard capacity rejects setup; controls are not left partly guarded. Naked/stale
clicks and input from a retired opening cannot act.

Managed mode/PIN handlers queue scalar intent rather than creating the legacy
one-millisecond deferred timer. PIN submission copies the complete bounded code
into the command owner, wipes the UI input and keeps the opening valid until that
command is consumed. It then closes the keypad. Cancellation explicitly wipes the
UI input and cancels only a command with that exact modal token; a newer grid
command is preserved. Retiring during configuration activation hides/resets shell
state without deleting widgets inside dispatch. Actual deletion and context/gesture
release wait until a safe main-loop pass. Old context strings survive callbacks
that continue after replacement.

A requested modal kind must be admitted by its family. Ordinary families accept
only their normal card kind. The alarm adapter additionally admits a full control
panel for the current critical source, including a hidden action card with optional
PIN. It verifies the source/configuration identity instead of inventing a grid
binding. Switching the same card between PIN and control creates a new opening.
Commands from that critical action panel use panel action rules while it remains
the selected critical origin. Other action intents remain restricted to their
configured mode. The application scene must reconcile critical selection before
presentation, route takeover/dismissal, and consume scalar acknowledgement feedback;
this adapter does not select the scene or fabricate takeover lifetimes itself.

Control UI preserves the existing display-takeover policy; PIN UI uses the existing
dismiss policy. A context never stores grid-button pointers. The port only updates
shared state projection when its revision changes, avoiding repeated UI writes on
unchanged frames. Legacy helpers retain their ordinary behavior without an installed
observer; PIN clear/back/close explicitly wipe the current input bytes in both paths.
Transport copies and previously released legacy buffer capacity are outside that
retention guarantee.

The native LVGL test uses production contexts, PIN/defer/input handlers, gesture and
modal owners, shared graph/status/acknowledgement owners and deferred dispatcher.
Shell/layout construction and outbound transport are doubles. All nine profiles run
1,000 control/PIN/send/close cycles with stable C++/arena/LVGL retained bytes,
registrations and timer counts. Tests cover idle no-allocation updates, sixteen-digit
limits, mode/session mismatch, hidden inherited actions, critical action-to-control
routing, readonly/reconnect behavior, queued-command cancellation, replacement inside
dispatch, unavailable/failed setup, guard capacity and opening transitions. Generated
legacy policy/report tests also run with the managed observer absent. Real S3/P4 SDK
checks compile the complete existing panel and adapter, including six/nine/twenty-slot
banks; they do not establish visual layout or the factory firmware matrix.

Alarm scene/feedback routing, remaining families, confirmations, compiled legacy
registry retirement and physical S3/working twenty-slot P4 qualification remain
required. No device is flashed or physically measured here. Stored/backup formats
and nesting depth are unchanged; P4-86 stays reserved for USB diagnosis.
