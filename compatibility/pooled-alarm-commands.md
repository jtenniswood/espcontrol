# Deferred alarm commands

`PageAlarmCommands` owns one fixed pending arm/disarm command, independent of
configured card count. It creates no widgets, timers, subscriptions or per-card
records. It is an unselected prerequisite for the alarm modal and application
scene; production still uses the existing renderer.

An intent contains exactly one full grid binding or modal-opening token plus the
connection captured with that input. Grid intents apply only to action cards and
must match their configured mode. Control cards require a live alarm control/PIN
opening and a currently exposed mode, in the existing contract order and visible
limit. The dedicated arming/pending view can always offer Disarm, including when its countdown is unknown or finished. Options and
PIN requirements use immediate-parent inheritance. Mode/options parsing happens
when explicitly queuing a user command, rather than on frame or navigation updates.

The queue checks the current configuration, opening/binding, global connection,
service availability, source command freshness and critical cache capacity. It
checks those guards again before dispatch. Reconnect, navigation/modal retirement,
unavailable state and committed replacement cancel stale work. Failed or discarded
preparation preserves a valid active command. An invalid new input leaves the
existing command intact; a valid later input replaces it.

PIN entry belongs to the one demanded UI context. Queue only the completed code:
one to sixteen decimal digits when required, with no truncation or embedded NUL.
Optional PIN modes can send an empty code. The queue clears its previous code on
replacement, cancellation and destruction. Before sending, it moves the command to
local storage and explicitly wipes the fixed member buffer; the local code is also
wiped after return. The UI must independently clear its input buffer and keep its
opening valid until dispatch completes. These rules limit retention; they do not
promise erasure of copies made by the HA transport.

The application calls `loop` outside native, coordinator and LVGL dispatch. A port
supplies those readiness checks, global connection/service checks, full token
validation and the actual HA sender. Validation methods are read-only. A successful
send starts the shared acknowledgement policy; a failed send does not replace an
existing acknowledgement. The entity string is owned for that explicit dispatch so
that a reentrant configuration replacement cannot invalidate the sender's borrowed
view. A send can queue a newer command, but recursively pumping the queue cannot
send it twice. Configuration/connection changes during send prevent the old
acknowledgement from attaching to a replacement document.

Place commands before acknowledgements, state and graph in the runtime resource
group. The source/ack observers keep their existing ordering. The UI must bind the
selected mode to its PIN session and use gesture guards; this queue does not
manufacture authorization from an arbitrary card ID. Hidden-alarm takeovers use
live configuration/modal identities rather than fabricated grid bindings.

Generated tests compare actual legacy visible-mode/PIN policy, including filtered
and over-limit actions. All nine profiles cover every service, complete/invalid
PINs, busy dispatch, stale bindings/openings/connections, unknown/exhausted state,
inherited hidden children, generation-zero publication, failed/discarded staging,
failed sends and 1,000 pending-command replacements. Reentrant send tests replace
the document while entity/code views remain in use, queue another command and
check that only the new configuration can acquire its acknowledgement.

Demanded alarm control/PIN UI, scalar feedback, critical routing, remaining families,
confirmations, scene integration, compiled legacy registry retirement and physical
S3/working twenty-slot P4 qualification remain required. No device is flashed or
measured by these checks. Stored/backup formats and nesting depth are unchanged;
P4-86 stays reserved for USB diagnosis.

An action-card control opening must still match the current takeover to widen its
configured action. The sole post-takeover exception is the exact already-selected
PIN action proven by `PageAlarmModalPort::selected_pin`; implement the command
port's `selected_pin` callback by delegating there. Both queue and dispatch validate
this proof. A modal kind by itself cannot authorize a new alternative mode.
