# Reusable number slider views

The unselected renderer now renders numeric Slider cards and existing numeric
`light_brightness`, `fan_speed` and cover-position/tilt aliases. Both `number`
and `input_number` retain their Home Assistant min/max/step ranges and the
existing numeric policy's 10,000-position ceiling. Percent controls continue
to use their separate shared owner.

`PageNumberSliderRuntime` owns ranges and each card's thumb/gesture history;
views borrow its snapshots and the shared HA graph. Configured labels take
precedence over raw friendly names. During an active numeric drag, the icon is
replaced by a value and HA units, with the existing rounding/negative-zero
formatting. The fixed 48-byte formatter also backs the existing string API.
Outside a drag, the normal off icon returns. These cards do not use checked
backgrounds or large-number sizing. Geometry, fill colours and text contrast
reuse the existing vertical slider policies, including asymmetric padding.

A held gesture captures the complete binding and connection. Range changes,
invalid/exhausted state, reconnect, navigation and suspension cancel it and
wait for pointer release. Equivalent metadata resets the numeric overlay while
keeping the physical release valid. Repeated state reports confirm a released
preview; an active numeric drag retains its own thumb. Duplicate cards share
state and keep independent previews. Rebinding resets the widget range before
a percent/media control uses the same shell.

`PageBasicActions::number_slider` receives the snapped double, current range
metadata and validated binding after release. The eventual application scene
must resolve the original card and call its existing numeric-service helper;
the view does not register callbacks with HA or dispatch services itself.
Preparation requires a numeric owner, action hook, fill and slider. No new
per-card prepared metadata or per-shell gesture fields are needed: the port
adds one fixed owner pointer and revision. Immediate drag painting does not
consume pending updates for other visible cards.

Model tests cover all numeric aliases, raw names, units, overlays, exhausted
graph data and allocation-free reads on all nine profiles. Native tests use
real PageStore publication from the private preparation generation to zero,
full numeric ranges, physical pointer endpoints, existing geometry, stale
releases, repeated/changed metadata, mixed numeric/percent page reuse and
1,000 navigation cycles with stable retained resources. Use
`ESPCONTROL_TEST_FAMILY=number_sliders` for the focused native suite.

Production scene selection, remaining drivers, physical S3/working 20-slot P4
qualification and the full factory build matrix remain required. No device is
flashed by this stage. Host retained bytes are not physical heap/PSRAM savings;
saved documents, backups and nesting depth remain unchanged. P4-86 remains
reserved for its separate USB diagnosis.
