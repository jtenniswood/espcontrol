# Reusable light-temperature views

The unselected renderer now renders Light Temperature cards through the shared
`PageLightTemperatureRuntime`. Preparation requires that owner, a validated
action hook, and a complete slider/fill bank. Production scene selection and
physical qualification are still unfinished; production uses the existing
renderer.

Configured labels take precedence over raw friendly names. The normal lightbulb
or configured icon remains visible while the bottom label shows a Kelvin drag
preview rounded to 50 K. Releasing or cancelling restores the latest label;
the command target retains its exact mapped Kelvin value. A same-position press
does not invent a drag label. Remote Kelvin updates can move the thumb without
replacing an existing drag label, matching the production callbacks.

Default/custom ranges and independent duplicate previews live in the state
owner. An on child contributes to the existing parent-folder indicator. Off reports empty the fill while retaining Kelvin history for the next
on report. Colour mode uses the absolute 2000–6500 K gradient, even for narrow
configured ranges; it does not stretch the palette or apply display correction
to those raw Kelvin colours. The existing renderer and new view share the same
pure colour helper. Plain fills retain configured/theme slider colours. Text
contrast follows the actual fill colour.

Vertical slider/fill geometry retains the existing padding and endpoint policy.
Vertical numeric/percent/temperature cards place the icon at the content's top
left and the label at its bottom left. Temperature icons use clipping; normal
label geometry and modes are restored when the shell is reused. Bind resets the
range for percent/media reuse. No new prepared metadata or per-shell interaction
fields are added; the port has one fixed owner pointer and revision.

Input captures the full binding and connection. Unavailable/exhausted state,
navigation, reconnect and suspension cancel an unfinished gesture and wait for
release. The action hook receives a validated binding and exact Kelvin target;
the application scene must resolve the original card and use its light service
policy. Immediate drag painting does not consume other visible cards' pending
updates. Views add no HA callbacks, timers, subscriptions or permanent contexts.

Model tests cover raw names, ranges, duplicate history, drag labels, exhausted
graph data, palette endpoints and allocation-free reads on all nine profiles.
Native tests use real private-to-zero store publication, physical endpoints,
legacy slider geometry, label alignment/restoration, raw Kelvin versus plain
colours, contrast, off/on ordering, remote/label reports while dragging, stale
input and 1,000 navigation cycles with stable retained resources. Use
`ESPCONTROL_TEST_FAMILY=light_temperature` for the focused native suite.

Remaining view ports, scene/action integration, physical S3/working 20-slot P4
qualification and the complete factory build matrix remain required. No device
is flashed by this stage. Host retained bytes are not physical heap/PSRAM
measurements. Saved documents, backups and nesting depth remain unchanged.
P4-86 remains reserved for its separate USB diagnosis.
