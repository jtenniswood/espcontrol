# Fan power and oscillation status history

The unselected renderer supports Fan Switch and Fan Oscillation cards through
`PageFanStatusRuntime`. A configuration-owned source record shares power and
oscillation history across duplicate consumers and hidden pages. It owns no
widgets, callbacks or LVGL timers. The source limit defaults to 32 distinct
entities with 2,048 bytes of copied names, including terminators.

Prepare this owner before the HA graph and include it in the fixed
`PageEntityObserverSet`. Every new configuration starts with the legacy eager
card's initial Unavailable history; copied HA snapshots then produce the same
status transitions as retained subscription reports. Discard/failed preparation
preserves active history, and an empty configuration releases the arena.

Changed status appears for three seconds, then returns to configured text or
the reported friendly name. Initial Unavailable only seeds history; it does not
replace the initial Fan/Oscillation label. Equivalent reports do not extend the
timeout. Friendly-name changes, including an explicitly empty name, take effect
when the transient status expires. Hidden pages keep their original deadline:
navigating back never restarts the three seconds. Unsigned elapsed time handles
clock wrap, and the main loop expires deadlines without per-card timers.

Power uses the existing on-state predicate. Oscillation preserves the last
checked value on an invalid attribute report while showing Unsupported, as the
legacy reader does. A reconnect clears old semantic state; fresh reports can
restore it. Exhausted cache data fails closed in the card and action path.
Power retains its custom on-icon; oscillation keeps the same icon for both
states. Folder indicators follow primary fan power for both card types.

The port requires this owner and the existing full-binding action hook. It
rejects stale presses and unavailable/unsupported controls before dispatch.
The eventual scene must resolve the original card and send the existing power
or oscillation action using fresh state. A view binding is never a retained
legacy context.

Memory counters separate fixed owner storage, active/staged/peak arena bytes,
source count and preparation failures. The port adds one fixed owner pointer
and revision, with no extra fields in each visible shell. These figures exclude
allocator overhead and do not prove physical heap/PSRAM savings.

The host reference test compares 10,000 state/attribute/time sequences with the
actual legacy status and transient-label functions. Other tests cover duplicate
consumers, copied snapshots, exact deadlines, equivalent reports, invalid values,
reconnect, failed preparation/discard, clock wrap, limits and 1,000 replacements.
Model and native LVGL checks cover all nine profiles. The native suite exercises
hidden-page expiry, full-binding actions, stale presses, unsupported controls,
folder indicators and 1,000 navigation cycles with stable retained resources.
Use `ESPCONTROL_TEST_FAMILY=fan_status` for the focused native suite.

Private prepared generations are promoted at commit, including generation-zero
initial loads. The renderer polls owners before forced as well as quiet frames,
so the first frame after navigation/reconnect has current deadlines and state.

Production selection, remaining drivers and physical qualification are still
unfinished. No device was flashed by this stage. Saved/backup formats and the
nested-page feature remain unchanged.
