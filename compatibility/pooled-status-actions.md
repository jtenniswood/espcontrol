# Pooled status and simple action cards

The unselected basic port now also presents door/window and presence cards,
ordinary service actions, local one-shot actions, Push, Webhook and Screensaver.
It preserves configured/default icons and labels, friendly-name fallback, the
presence `detected` predicate, status backgrounds and optional active colour.
Action cards preserve plain, icon, text and numeric state displays, including
their distinct positive-number predicate and existing 96-byte text limit.

Action state is the sole primary dependency declared by `PageCardDependencies`
for that card. The presenter reads its existing channel directly instead of
retaining another entity-name or state copy. Decoded state units and generated
local-action labels are copied during preparation. Unit reservation is counted
separately from label reservation, including long configured units. Preparation
rejects actions without the required dispatch hook.

The action hook receives a validated binding token after a press/release/click
sequence. It must resolve and execute the existing service/local/push/webhook/
screensaver policy using the current configuration. This PR supplies presentation
and a guarded handoff, not the final application dispatcher. In particular, the
existing root/subpage Push payload differences remain the dispatcher's concern.
Confirmation dialogs, option selectors, screen-lock/local-relay state, local
sensors and the other unported families still require their own integration.
Unsupported active variants fail preparation; they are not partially displayed.

Push feedback uses the existing 400 ms colour transition. Unbind disables the
transition and writes the existing background properties to cancel a running
LVGL colour animation before reusing its shell. The native test draws a frame,
starts an actual press animation, navigates to another active card in that shell,
and checks that the old animation cannot repaint it.

The rendered test also exposed an LVGL 9.5.0 boundary: an explicitly stored null
transition descriptor is dereferenced when a drawn object's state changes. The
port now uses a valid descriptor with an empty property list for its neutral
state. It neither removes/reallocates style properties during navigation nor
patches the SDK. Earlier layout-only tests did not exercise that rendered path.

Pure-reader tests compare state predicates and text formatting with the existing
helpers across all profiles, including unavailable/non-finite values and long
units. Native LVGL tests use actual press/release/click ordering, perform a
software-rendered frame, verify animation cancellation, reject stale clicks and
missing hooks, and run 1,000 navigation cycles per profile. They track callback
descriptor additions explicitly. C++ allocations, arena reservations, label
descriptor additions and retained bytes stay fixed. With drawn widgets, the
fixture records 8,000 temporary style-state and 4,000 grid scratch allocations
per 1,000 cycles; these are separately bounded and released. This is not a
zero-total-allocation claim. Address/undefined-behaviour sanitizers cover both
the original and expanded profile fixtures and the modal-gesture checks.

The production renderer is unchanged. Full service dispatch, appearance/touch
parity, device heap/PSRAM, frame timing and replacement peaks still need physical
qualification before selection. There is no document, backup or nesting change.
