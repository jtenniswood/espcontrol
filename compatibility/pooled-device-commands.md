# Pooled device commands

This dependent stage completes the deferred grid command families: fan power,
oscillation and direction; cover commands and toggles; garage/gate/lock cards;
and vacuum/lawn-mower commands. It does not select the pooled renderer in
production firmware or add nested pages. It follows [command routing](pooled-command-routing.md).

`PageDeviceCommandPolicy` borrows the existing HA graph, fan and access state
owners. It admits one scalar `DEVICE_ACTION` selection into the same eight-entry
FIFO used by sliders, timers and media. Admission reads normalized configuration
views and shared card readers without decoding options, allocating, subscribing,
creating widgets or dispatching. The scene must register this kind with the
control router and bind `PageBasicActions::device` to `request_device` using the
connection captured on PRESSED. Modal fan controls remain with their demanded
owners, and required garage confirmations remain with the confirmation owner.

The selected service/direction cannot change while queued. A changed state or
capability that would choose a different service cancels the press. Every send
also rechecks configuration activation, full origin binding, active input,
connection and service availability. Unknown/exhausted state follows the existing
card readers: unavailable gate and fan commands are blocked, plain cover toggles
retain their existing availability behavior, startup mower behavior and stateless
vacuum commands retain their canonical policy. Cover commands preserve normal
versus tilt fallback and position clamping. No unsupported modal/status-only
variant is admitted.

The native adapter uses HA's existing entity-action helper and the canonical
cover helper, including its single outstanding stop response and delayed owned
entity string. The cover helper now returns whether the transport accepted the
send; existing callers may ignore that return. This is not a Home Assistant
execution acknowledgement. Cleaning-area requests retain `cleaning_area_id`.

Queue admission never paints optimistic access/cover feedback. The scene may
call `preview_device` from the native adapter's accepted hook, with the captured
connection and primary revision. It validates the complete current binding and
revision before setting the existing temporary checked visual; failures,
navigation, reconnects and synchronous state updates cannot paint a stale card.

The actor payload and eight-entry FIFO do not grow (host `PageCardCommands` stays
920 bytes). This adds one fixed callback pointer to the view and one policy with
three borrowed references; no per-configured-card or shell state is added.
Target heap/PSRAM savings require production integration and matched device
measurements; host object sizes do not establish physical savings.

Validation covers all nine device profiles, allocation-free admission, every
selected service, root/child presses, bounded queue overflow, stale bindings,
reconnect, capability changes, required confirmation, send rejection and
same-generation replacement. Real LVGL pointer tests exercise repeated root and
child navigation and accepted-only feedback. Transport tests extract the actual
production cover helper and include the production device dispatcher, checking
payloads, failure cleanup and delayed responses. These are host checks, not
current ESPHome factory builds or physical display qualification. The P4-86
remains reserved for its separate USB diagnosis.
