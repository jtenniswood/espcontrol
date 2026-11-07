# Prepared confirmation routing

This #1825 prerequisite connects prepared switch, garage and script cards to the
[demanded confirmation owner](pooled-confirmation-runtime.md) and
[Yes/No panel](pooled-confirmation-modal.md). Production still selects the legacy
renderer. Application-scene wiring, compiled legacy retirement, factory builds
and physical qualification remain. No firmware was flashed or device heap/PSRAM
saving claimed.

Preparation now accepts these confirmation variants and records two flags in
existing card-definition padding. The record remains 264 bytes on the host and
156 bytes with both actual S3 and P4 compilers, before and after this change.
There is no added per-configured-card context, string, widget, subscription or
timer. A missing confirmation callback fails preparation rather than accepting
a feature that could send an unconfirmed command. The scene's one callback
forwards the binding, captured press connection and coherent chosen direction
to `PageConfirmationRuntime::request`.

Required directions route only to confirmation admission, without falling
through to the ordinary toggle/action/access callback if admission declines.
The other direction keeps its existing action. Script prompts always route to
confirmation. Explicit garage Open/Close retains that direction; garage toggles
use their coherent state history. Confirmable inputs capture the HA connection
on press and reject a later click after reconnect. Card/view replacement and
navigation continue to invalidate the complete binding identity.

Confirmable toggles require a fresh available primary state before choosing
either direction, including the direction whose flag is off. Otherwise an
unknown state could look off and accidentally bypass an Off-only confirmation.
The command owner and grid share that known-state predicate and the same
allocation-free policy. Explicit garage commands and scripts need no new
primary-state subscription. Non-confirmed card families keep their existing
input handling at this stage.

Native tests use the actual prepared port, renderer, entity graph, access owner
and publication lifecycle. Nine profiles run 1,000 root/child navigation cycles
with confirmation routing, fixed widgets/arenas/registrations and stable retained
LVGL bytes. They cover both flag directions, ordinary opposite-direction
callbacks, script/garage routing, absent adapter, stale and bare clicks, unknown
state, fresh reports before an old click, reentrant navigation, staging discard
and generation-zero publication. Admission creates no C++ or arena storage;
LVGL's existing transient input/style-animation allocations remain, and settled
view retention is checked across complete navigation cycles. Metadata/command
models separately cover all card families and legacy policy parity.

After scene integration, qualify Guition 4848S040 S3 and a working 20-slot
JC8012P4A1 P4. Test root/subpage On-only, Off-only and both-direction prompts,
unknown/reconnecting state, garage Open/Close/toggle, scripts, cancelled admission
and source replacement. Compare matched heap/PSRAM, minimum/largest block,
latency and transient draw/transition peaks. Keep P4-86 reserved for USB
diagnosis. Navigation depth and backup/document formats remain unchanged.
