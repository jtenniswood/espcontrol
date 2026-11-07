# Demanded Yes/No confirmation panel

This #1825 prerequisite adds `PageConfirmationModalPort` on top of the
[confirmation command owner](pooled-confirmation-runtime.md). It creates one
context only for a requested switch, garage or script prompt. Production still
selects the legacy renderer. Prepared-card routing, application-scene selection,
compiled legacy retirement and physical qualification remain; no firmware was
flashed and no device RAM saving is claimed.

The context owns full configuration, decoded message/Yes/No text, caption and
an eight-entry gesture reservation. It retains no source widget or old arena
pointer. Setup requires a matching captured command plan; a direct modal request
cannot create a prompt that lacks an original action/press epoch. Failed setup
relinquishes the plan, including metadata/headroom/style/shell failures.

The panel reuses the common shell and text-button construction. The text-button
sizing code is now a stateless helper shared by initial construction and live
font/geometry updates, preserving natural width, wrapping and minimum touch
height. Message/action spacing and positioning match the existing confirmation
panel. The style resolver receives the full opening and owned configuration,
so a root/subpage source can resolve its actual button/message/icon font roles
and coherent caption. Caption views are immediately copied. Theme, accent,
readable Yes text, font and rotation changes update existing demanded objects;
unchanged maintenance allocates and redraws nothing.

Only a current press/release gesture can activate Yes/No/Back. Yes captures its
HA epoch and queues a command, rather than sending inside LVGL dispatch. Safe
maintenance sends the original selected direction/script with owned bytes and
closes the matching opening. No and Back revoke the context and cancel any
queued action. Retirement inside event/draw dispatch revokes input and command
ownership without hiding, invalidating or deleting the widget being processed.
Collection deletes the panel and gesture metadata after dispatch completes.
The command owner, style owner, graph and allocator must outlive collection.

Native tests use actual LVGL, the modal runtime, command owner, graph and store;
common shell geometry and text-button helpers are generated from production.
Transport and shell ownership boundaries are doubled. Nine device profiles
run 1,000 Yes/No/Back cycles each (27,000 opens), checking stable retained
C++/LVGL/arena memory and registration counts. Additional cases cover bare or
lost presses, queued-input allocation guards, 1,000 idle pumps with no redraw,
custom/default messages, switch/garage directions and full script options,
root/child source fonts, theme/accent changes, metadata/style/headroom/shell
failures, stale reconnect input, takeover, reentrant close/send, close during
drawing, staging discard, overwritten retired arenas, generation-zero
publication and final-opening exhaustion. Nine screen shapes check Back and
action clearance and exact coordinates against the live legacy positioning
block. These do not constitute physical font/device testing or factory builds.

After scene integration, qualify Guition 4848S040 S3 and a working 20-slot
JC8012P4A1 P4: normal/long translated custom text, both directions, script fields,
No/Back, hidden-source fonts, rotation/theme, HA reconnect, takeover and repeated
configuration changes. Compare matched heap/PSRAM, minimum/largest block and
latency. Keep P4-86 reserved for USB diagnosis. Backup formats and navigation
depth are unchanged by this stage.
