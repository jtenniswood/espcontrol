# Alarm presentation maintenance

`PageAlarmPresentation` reconciles critical alarm selection and scalar failure
feedback on the application loop, independently of visible grid bindings. It uses
one fixed record and adds no configured-card widgets, registrations or timers.
Production remains on the existing renderer; the complete application scene must
supply the display/feedback port and pump this owner before the shared modal owner
and frame presentation.

Fresh eligible reports can request an alarm panel, including a hidden action
card through the admitted control-panel route. Countdown ticks and caption expiry
are not new takeover events. Control-only arm-mode reports do not reopen dismissed
UI; eligible action-source arm-mode reports retain the existing refresh behavior.
A small monotonic source event counter distinguishes those cases without retaining
report text or allocating an event queue.

Closing an automatic control panel releases its critical display lease and suppresses
reopening until another eligible report, selected origin or connection changes.
The controller also detects close-before-next-frame. A scalar source-event marker
captured at retirement orders a close against reports between frames: a report
before Back cannot reopen the panel, while a later report can. Closing/completing a
PIN screen keeps the critical display lease until state changes, without reopening
the panel until another eligible report, matching the existing keypad behavior. It adopts an already visible
manual panel without making it automatic. When critical state ends, it closes its
own automatic control panel but leaves a manual panel or an active PIN session
open. Only the exact action already selected for the live PIN session can complete
after its takeover ends. The command port delegates this proof to the demanded
alarm port, and both queue and dispatch recheck it. A stale action-card panel cannot
select a new alternative mode, including before presentation cleanup. A selected
PIN transition is preserved through countdown expiry while the source remains
command-available; command configuration, availability, opening,
PIN and connection checks remain mandatory.

A failed modal setup is attempted once per relevant source/connection event, not
on every frame. While disconnected, retained critical state keeps its display
lease; no new modal request is admitted until connected. This owner does not
suspend the renderer: the scene must reconcile actual modal visibility, avoiding
an empty screen when setup or connectivity prevents opening. It must also integrate
critical display priority and normal modal/takeover policy.

Modal opening nonces are reserved when a valid request is queued. They identify
that pending request through creation or cancellation and are never reused. An
alarm can cancel its own pending request without erasing newer user input. The
shared owner exposes the last scalar retirement token/reason, dismissal policy and
family source-event marker, plus the current opening's policy, so presentation does not retain a modal context or widget.
Nonce exhaustion rejects new requests while allowing the last accepted one to
finish; an invalid request does not consume a nonce or replace valid queued work.

A configuration activation only changes scalar ownership; UI/display effects wait
for safe maintenance. Put presentation before modal, command, acknowledgement,
source and graph owners in the resource group. Pump source/acknowledgement state
even while the grid is suspended. The presentation port must report safe dispatch,
current connection and one scene-owned critical display lease. It consumes at most
one current-configuration/current-connection failure event per loop, without PIN
text or borrowed status pointers. The feedback callback may reenter configuration
replacement; recursive presentation loops are suppressed.

Host sanitizer tests cover all nine profiles, manual versus automatic ownership,
user close before observation, countdown suppression, fresh-report reopening,
hidden action origins, reconnect, failed setup retry limits, exact pending-request
cancellation, deferred activation effects, feedback and reentrant replacement.
Each profile runs 1,000 allocation-free critical open/close cycles with stable
registrations/arenas. Native LVGL tests additionally run the demanded control/PIN
adapter and real command dispatcher, complete a chosen PIN action after countdown
expiry, preserve manual close until a new report and consume scalar failure feedback.
Their shell/layout and outbound display/transport ports remain doubles.

The selected scene, actual display/feedback wiring, remaining families,
confirmations, compiled legacy registry retirement, full factory builds and physical
S3/working twenty-slot P4 qualification remain required. No device is flashed or
measured by this stage. Stored/backup formats and nesting depth are unchanged;
P4-86 stays reserved for USB diagnosis.
