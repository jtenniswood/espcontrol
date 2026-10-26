# Deferred slider actions

Percentage (brightness, fan speed, cover position/tilt), numeric and Kelvin
slider releases share the eight-entry `PageCardCommands` FIFO with basic card
actions. Slider background toggles use a separate typed intent; numeric cards
cannot toggle. `PageSliderCommandPolicy` borrows the existing three history
owners and entity graph, adding no fields to configured cards or subscriptions.
The queue has a fixed larger scalar payload independent of card count.

Admission and dispatch validate the full binding, configuration activation,
HA connection, service availability and current state. Numeric intents retain
the exact min/max/step and snapped value; metadata changes cancel a queued edit.
Native dispatch uses the existing percent/tilt and numeric helpers, and sends
the captured Kelvin integer directly without a percent round trip. Calls occur
only from a safe main-loop turn, outside LVGL events/timers and native callbacks.
`DISPATCHED` means the helper accepted the send, not an HA acknowledgement.

Rejected, cancelled or failed edits restore the authoritative history if that
edit still owns the preview. A newer queued value, active numeric/Kelvin gesture,
source report or configuration activation prevents an older failure from
rolling back newer state. Configuration activation has its own monotonic epoch,
so even same-generation replacement during a reentrant send is isolated.
Put the command owner before the histories and graph in the resource group.
Cancel it when input is covered/locked; merely hiding the grid is insufficient.

The normal host fixture exercises 1,000 six-action cycles on all nine profiles,
plus overflow, invalid values, changed ranges, unavailable state, reconnect,
failed sends, covered input and reentrant replacement after the old arena is
poisoned. Admission allocates no C++ objects. Native helper spies check tilt,
numeric metadata, exact Kelvin payload and helper failure propagation. The
optional real LVGL fixture (`ESPCONTROL_TEST_FAMILY=slider_commands`) runs 1,000
pointer releases for each slider kind on every profile, verifies no send inside
the callback and checks failure/cover rollback with stable arena allocations
and graph registrations.

Production still selects the legacy renderer. This stage does not enable
nested pages or establish physical heap/PSRAM savings. Scene composition,
remaining command families, production cutover and S3/P4 qualification are still
required before the depth-two editor/navigation work can proceed.
