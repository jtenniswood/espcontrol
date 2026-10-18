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

Demanded fan-panel ownership and the separate switch, preset, direction,
oscillation and speed cards remain unfinished. Scene integration, production
selection, physical S3/20-slot P4 qualification and the remaining device profiles
are still cutover gates. These host checks do not measure device heap/PSRAM or
establish physical layout parity. Nesting and persistence formats are unchanged.
