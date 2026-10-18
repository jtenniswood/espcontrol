# Reusable cover command cards

The unselected renderer supports Open, Close, Stop and Set Position cover
command cards through the shared HA cache. Production scene selection,
remaining drivers and physical qualification are still unfinished; production
uses the existing renderer.

Cards retain configured/friendly/translated Cover labels, the normal configured
or Blinds icon, push feedback and no checked/number/fill display. Command cards
do not contribute to an automatic folder indicator. Targets remain in the
original stored card, so the eventual scene can use its existing position
parsing, service dispatch and Stop-response policy.

The existing renderer and reusable view share capability selection helpers.
Missing or invalid capabilities keep the existing normal-command fallback.
Known masks reject unsupported commands. Tilt is selected only when its
capability exists and the normal equivalent does not; normal wins when both
exist. Reads use a fixed bounded parser and fail closed if the shared graph
exhausted its state budget. No per-card context, callback, timer or registration
is created. Prepared metadata reuses existing fields; the port adds one fixed
scene hook and no per-shell fields.

Press captures the shared graph's connection generation as well as the full
binding. Click validates the binding, connection and current capability
snapshot before calling the scene hook. The scene must resolve the original
card and use the existing command policy. Navigation, suspension, config
replacement and reconnect cannot turn an old press into a new command. The
hook may navigate reentrantly after validation.

All-profile model tests cover all 256 standard masks for all four commands,
normal/tilt preference, unknown/invalid data, labels, exhaustion, reconnect and
allocation-free reads. Native tests use real private-to-zero publication,
missing-hook rejection, capability changes before click, unsupported commands,
reconnect between press/click, exhausted data, stale bindings, reentrant
navigation and 1,000 navigation cycles with stable retained resources. Use
`ESPCONTROL_TEST_FAMILY=cover_commands` for the focused suite. Existing cover
confirmation and Stop-failure tests remain applicable to dispatch integration.

No device is flashed by this stage. Scene/action integration, remaining view
ports, physical S3/working 20-slot P4 qualification and the factory build matrix
remain required. Host retained bytes and SDK syntax checks do not establish
physical layout or heap/PSRAM savings. Saved documents, backups and nesting
depth are unchanged. P4-86 remains reserved for its separate USB diagnosis.
