# Demanded guest Wi-Fi state and dispatch

This #1825 prerequisite supplies one guest-network control owner for the pooled
Wi-Fi sharing modal. Production still uses the existing renderer. QR tile/button
rendering, demanded QR/details/guest widgets, application-scene wiring, compiled
legacy retirement and physical qualification remain unfinished. Nothing was
flashed; configuration/backup formats and nesting depth remain unchanged.

The entity graph already declares and deduplicates the guest switch dependency
for Wi-Fi cards with a guest tab, including hidden cards. `PageGuestWifiRuntime`
polls that owned state only while a guest control is demanded. It introduces no
observer, HA registration, timer or per-configured-card history. The modal owns
its entity string and must unbind before destroying it. Binding validates the
full WIFI_QR opening and this card's declared edge; another card sharing the
same switch cannot grant undeclared access. Bind, queue, polling, acknowledgement
and retirement allocate nothing. The existing guest state policy now accepts
string views directly, preserving legacy string callers without new copies.

QR credentials/details remain usable offline. The composite modal scene must
preserve that local opening while the guest control separately tracks the HA
connection, clears pending/known state on disconnect and waits for a fresh primary
report after reconnect. A press captures its connection, and queue rejects an old
press even if a new connection has already supplied fresh state. Unknown,
unavailable, stale, exhausted, invalid-domain and undeclared sources cannot act.
Only valid HA `switch.` entities are accepted; the runtime never calls a panel
Wi-Fi control function.

Commands begin the existing ten-second pending/acknowledgement policy but defer
outbound service work until safe maintenance. Only one intent is held. Matching
reports acknowledge; contrary reports do not. If the target state arrives before
dispatch, the redundant send is cancelled while retaining the reported state.
Timeout/send failure keeps the previous reported state and existing failure
caption; duplicate actions are blocked while pending. A unique monotonic command
identity prevents a failed old send from overwriting a newer intent queued
reentrantly in the same opening. The last nonzero identity is usable, then further
commands fail closed. Entity bytes are owned across the send callback, which may
close/reopen the modal or replace configuration; late results cannot affect the
replacement owner. No send occurs in HA, native or LVGL dispatch.

Model tests combine the actual graph, dependency declarations and modal runtime
with source/service I/O doubled. All nine profiles run 1,000 demanded send/ack/
close/reopen cycles without adding registrations. Allocation guards cover binding,
queueing, report delivery and close. Tests also cover hidden controls, cards
without guest dependencies, unavailable/exhausted sources, stale presses, offline
local-modal lifetime, reconnect, deferred/failed/expired dispatch, wrap-safe
timeout, final intent exhaustion, newer intents queued during an old send,
discarded staging and private UINT32_MAX to published generation-zero replacement
during send with old document bytes overwritten. Existing codec/state and legacy
modal lifecycle checks remain required. SDK/compiler checks supplement the model;
they are not physical network tests, factory builds or memory measurements.

After QR widgets and scene integration, use Guition 4848S040 S3 first and a working
twenty-slot JC8012P4A1 P4 next. Confirm QR/details stay usable offline, guest state
recovers only on a fresh HA report, pending/timeout feedback matches current
behavior and reconnect/close/replacement cannot dispatch a stale click. Use a
known test switch for action checks. P4-86 remains reserved for its USB diagnosis.
