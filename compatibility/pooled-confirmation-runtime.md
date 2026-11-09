# Demanded confirmation command owner

This #1825 prerequisite prepares switch, garage and script confirmation for the
shared modal runtime. It follows [local Settings ownership](pooled-network-status.md).
Production remains on the existing renderer. The demanded confirmation UI,
prepared-card routing, application-scene integration, compiled legacy retirement
and physical qualification remain. No device was flashed or RAM saving claimed.

`PageConfirmationPolicy` reads the immutable card's type/entity/mode and exact
comma-separated confirmation flags without allocating. It keeps the existing
switch/garage direction rules and script.turn_on confirmation, including bypass
for button entities. Other card families and non-confirmed directions are not
admitted. A confirmation never changes an explicit Open/Close or On/Off into a
generic toggle after the state changes beneath the prompt.

`PageConfirmationRuntime` holds one pending opening plan and one demanded
borrowed configuration pointer, rather than per-card contexts, observers, timers
or subscriptions. The card owner passes a coherent displayed state and the HA
connection epoch captured on press. The request queues a shared modal opening
without allocation. The plan records the complete source binding, opening token,
connection and selected action. The UI copies configuration from that card,
then binds only the matching pending opening with the same entity, type, mode,
options and unit. A direct modal request without a captured plan cannot bind.

Yes only queues an opening-scoped command with its own captured press epoch.
Repeated Yes input cannot dispatch twice. Main-loop maintenance checks current
configuration/opening/connection and service availability, copies the demanded
configuration to owned local storage, sends the original typed action and closes
that exact opening. This preserves full script-field values through reentrant
close/replacement; sending cannot borrow freed widget/configuration bytes or
close a newly opened prompt. No action is sent by No, Back, closing, replacement,
shutdown or display takeover. The scene backend must keep admission queries
read-only and request/cancel/close scalar; only send may pump producers.

Toggle-derived direction additionally requires a fresh available primary report,
including after reconnect. Explicit garage direction and script actions require
no new state subscription. The UI/scene must unbind before freeing its owned
configuration, cancel the command owner before retiring the graph/store and
resolve source fonts using the captured opening. Configuration activation and
retirement allocate/send nothing. Private staging can promote to generation
zero; discarded staging leaves a live prompt untouched.

Host tests compare policy against 3,456 combinations of legacy predicates.
Each of nine profiles runs 1,000 cycles through five toggle/garage/script paths,
with fixed owner/registration counts and allocation-free request/Yes/graph input.
Further checks cover full long script options, source state changes, root/child
cards, wrong configuration fields, stale bindings/openings/epochs, reconnect
without fresh state, missing services, cancelled requests, deferred dispatch,
reentrant send/close/new prompt, staging discard, overwritten retired arenas,
generation-zero publication and final opening exhaustion. Tests use the actual
modal runtime, PageStore, dependency declarations and entity graph with doubled
transport/UI boundaries. These do not constitute physical confirmation testing.

After the demanded UI and scene are integrated, qualify the Guition 4848S040 S3
and a working 20-slot JC8012P4A1 P4. Test custom/default Yes/No/message labels,
source fonts and Back geometry, both switch directions, garage Open/Close,
script variables, No/Back cancellation, reconnect/takeover and repeated config
replacement. Compare matched heap/PSRAM/minimum/largest-block/latency workloads.
Keep P4-86 reserved for USB diagnosis. Backup/document formats and navigation
depth are unchanged by this stage.
