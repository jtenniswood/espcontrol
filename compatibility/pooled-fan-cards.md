# Reusable Fan Control cards

The unselected page renderer can bind Fan Control cards to its existing
icon/label widget bank. They retain the configured icon and label, fall back to
the current friendly name or translated Fan label, and show their checked state
from the shared fan state. The V1 reader preserves the current normalization:
Fan Control uses its configured icon in both states; a separate on-icon is not
a saved setting for this type. Numeric/status presentation options do not turn
it into a sensor card.

A validated full grid-binding token goes to the required control-panel hook.
Missing hooks reject preparation; the card view creates no fan context, modal
widgets or subscriptions during navigation. Fan Control children also contribute
their fan's on/off state to an automatic folder indicator. The eventual scene
must resolve the original card type and configuration before opening a demanded
fan panel. A binding token is not a modal lifetime token.

Model tests cover all nine profiles, default/custom labels and icons, the V1
normalization, state/availability transitions, reconnects and 1,000 reads with no
C++ allocations. Native control-family tests exercise real LVGL buttons and slot
reuse, validate panel dispatch, reject an old click after navigation, and check
retained widget/arena/subscription resources over 1,000 cycles per profile. The
focused native test invocation is:

```sh
ESPCONTROL_TEST_FAMILY=controls ctest --test-dir .cache/lvgl-native \
  -R '^page_basic_lvgl_port_test$' --output-on-failure
```

The full native test remains the default when no family filter is supplied.

The separate switch, oscillation and speed cards remain unfinished.
Scene integration, production
selection, physical S3/20-slot P4 qualification and the remaining device profiles
are still cutover gates. These host checks do not measure device heap/PSRAM or
establish physical layout parity. Nesting and persistence formats are unchanged.

## Demanded Fan Control panel

`PageFanControlModalPort` creates one existing `FanCardCtx` in its opening record.
It reads the configuration-owned graph directly and adds no Home Assistant
callbacks or subscriptions. The null grid-slot constructor creates no transient
label timer. Context strings and preset data belong to the opening and are
released on close; no fields are added to legacy configured-card contexts.
The family observer adds fixed storage which must be included in device memory
qualification.

The opening caches ten channel revisions and its configured tab order. Quiet
admission checks do not reparse configuration. Each attribute updates its own
context field, so unrelated reports cannot overwrite local power/speed feedback.
A usable primary preserves the existing Power fallback. An offline primary can
keep a usable, configured Light tab.

Input consumes fresh capabilities and preset choices before dispatch. Command
handlers validate the current opening and relevant capability, retain an owned
preset value, and check again after painting that can invoke callbacks. Capability
and preset-list rebuilding waits until input dispatch returns; input to an old
control is rejected until the replacement view has been painted. Newly built
rows receive the opening's gesture guards. Closing notifies the common runtime,
which defers context release until callbacks and input dispatch finish.

The ten-channel reservation rejects a future expanded schema until its adapter
is updated. Exhausted values and preset data longer than the existing text-state
limit fail panel admission without parsing a prefix. The existing 32-row preset
UI limit remains. Percentage parsing uses a terminated bounded copy, rejects
non-finite data and clamps before conversion; capability extraction consumes
only defined positive bits from a wide parse.

Native tests use the production context constructor, channel projection,
capability/tab policies, command handlers, preset-row construction and common
modal runtime. They cover 1,000 openings with stable retained C++/LVGL/arena and
subscription resources; 1,000 quiet frames without allocations; fresh capabilities
and presets before input; offline Light; extreme readings; oversized/exhausted
options; reconnects; allocation/gesture admission; and reentrant replacement with
poisoned old configuration storage. Platform shell opening, geometry and paint
helpers are doubled. These tests do not establish physical layout or real fan
command parity. Scene selection and device qualification remain required.

## Direction and Preset card views

Direction and Preset cards now use the shared widget bank. Their persistent
status caption overrides a configured/friendly name, matching the existing
cards. Direction is checked for Reverse; Preset is checked for a nonempty mode
other than None/Off. Their folder indicator still follows the primary fan's
power state. Offline captions, unsupported directions/options, custom/automatic
icons and dynamic translated captions preserve the legacy policies.

Reads and formatting use bounded caller-owned storage, with no per-update
strings or option vectors. The option predicate and UTF-8 caption formatter are
compared with the actual legacy helpers, including 30,000 generated cases and
three locales. Both six-slot and twenty-slot adapters compile in the real S3/P4
SDKs. All nine native profiles cover missing hooks, normal dispatch, stale
presses, reconnect invalidation and 1,000 navigation cycles with stable retained
C++/LVGL/arena/subscription resources. Run this focused suite with
`ESPCONTROL_TEST_FAMILY=fan_modes`; the default native suite includes it too.

Direction dispatches the original validated binding to the action hook; Preset
dispatches to the control hook. The future scene must resolve the original card
and consume fresh capabilities before sending a direction command or opening a
Preset panel. The card-view port does not itself create a modal or send commands.
Production selection and physical qualification remain outstanding.

## Demanded Preset panel

`PageFanPresetModalPort` now owns the separate Preset list for one opening.
It shares the bounded graph projection with Fan Control, without adding callback
subscriptions or fields to legacy configured-card contexts. Both family observers
can coexist and route by the exact context record. The additional fixed observer
storage must be included in physical memory qualification.

Fresh options are consumed before a selection; a changed list blocks the old
press until rows have been rebuilt outside input dispatch. The selected string
is owned across callbacks. A selection closes only its original current opening,
so a service callback cannot close a replacement. Closing invalidates the common
runtime and releases the context, option strings, rows and gesture reservation.
The separate Preset card retains its existing options-list admission policy,
including lists reported without the Fan Control preset capability bit.

Both families use `PageFanModalState` for channel revisions and pending paint.
Exhausted/oversized data rejects the panel; recovery forces a complete projection
rather than retaining a temporary unavailable flag. The existing 32 visible-option
limit remains. Native coverage includes selection updates, all 40 parsed options
with only 32 rows, malformed/empty options, stale presses, disconnects, replacement
inside a service callback with poisoned old configuration storage, allocation and
gesture-reservation failures, and 1,000 open/close cycles plus 1,000 quiet updates.
The production context, list construction, selection callback and common runtime
are exercised; platform shell geometry/opening are doubled and require hardware
qualification. Scene integration and production selection remain outstanding.
