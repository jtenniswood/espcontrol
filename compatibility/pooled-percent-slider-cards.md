# Reusable percent-slider views

The unselected renderer supports light brightness, fan speed, cover position and
cover tilt cards through the shared percent-slider owner. Generic Slider cards
with those domains use the same path. Numeric and light-temperature variants
remain unported and reject preparation. Production selection is unchanged.

Each visible shell reuses its existing slider, fill, icon and wrapped label.
There are no new widgets, contexts, timers or HA registrations during navigation.
The port adds one fixed owner pointer and revision; it reuses an existing shell
counter for report ordering. Prepared definitions share one enum field with the
mutually exclusive forecast role, instead of growing every card's metadata.

The view preserves raw slider values, cover fill inversion, vertical geometry,
custom/automatic on-icons, configured/friendly labels and primary-power folder
indicators. Slider cards retain their base background and use the configured
fill colour; checked state does not replace the fill. Custom-background contrast
is chosen after display colour correction. The same existing draw callback
handles text across the fill boundary. Labels keep their card inset.

Geometry matches the production slider helpers, including asymmetric padding,
parent borders and rounded fills. Padding compensation preserves the reusable
button's styles for subsequent non-slider cards. Pointer input uses the existing
vertical endpoint mapping. An initial cover without a fill report has no
fictitious closed-cover fill. A valid zero/off report can subsequently produce
the normal inverted cover fill.

The state owner now exposes a fill-known flag and a per-source write revision.
Valid level and primary Off reports advance that revision even when cache bytes
are unchanged. Incoming reports still update a thumb during a drag, and a later
pointer sample takes precedence, matching the existing callback ordering.
Independent card previews survive navigation and reconcile through shared
reports. Invalid reports do not acknowledge them. Exhausted graph data hides
the fill and disables input until recovery.

Preparation requires the state owner, the percent action hook and slider/fill
widgets for every shell. A press captures the full binding and connection;
release validates both plus current availability before publishing a preview
and dispatching its 0–100 value. Navigation, replacement, takeover and reconnect
cancel an old gesture. Reentrant action callbacks can navigate without a later
release dispatching again. The eventual scene must resolve the original card,
including cover tilt, then call the existing action helper. Remote/card-level
toggle actions remain part of that scene integration.

Model tests cover all nine profiles, domain aliases, labels/icons, independent
previews, exhaustion, reconnect and allocation-free reads. Native LVGL tests
compare actual slider/fill geometry against the production helpers and exercise
vertical 0%/100% input, reports during dragging, navigation, stale releases,
takeover, reconnect, reentrant dispatch and 1,000 cycles with stable retained
C++/LVGL/arena/label/subscription resources. Missing resources reject preparation.
Use `ESPCONTROL_TEST_FAMILY=percent_sliders` for the focused native suite; the
unfiltered suite includes these cards alongside the previously ported families.

These are host/SDK checks, not physical layout or heap/PSRAM qualification.
Production scene selection, the remaining drivers, current factory build matrix
and physical S3/working 20-slot P4 checks remain required. No device was flashed;
saved/backup formats and nested-page support are unchanged.
