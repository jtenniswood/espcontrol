# Pooled option-selection cards and panels

The unselected `PageBasicLvglPort` now covers dedicated Option Select cards and
their canonical Action form. The primary value remains the exact reported text,
bounded by the existing 96-byte limit. Unknown/unavailable values display `--`;
configured labels, friendly names, entity fallback and the translated empty-entity
Option label retain their current order. The value replaces the icon, uses the
profile's option font (then media-title/label fallback), and does not activate a
folder indicator or active card colour.

The shared bank reserves the visible value buffer during preparation. Each
visible shell retains its original value font, widths and long-text mode, so an
option card's full-width dotted text cannot affect the next numeric card in the
same slot. The fixed shell fields are included in `PageBasicMemory::fixed_bytes`;
the additional font pointer belongs to the caller-owned theme. Device
qualification must include both. No configured-card context, timer, subscriptions or option vector is
created during navigation. A required control hook receives the original full
binding; the scene must resolve it against the current configuration before
opening the demanded panel.

`PageOptionSelectModalPort` creates one existing context in its opening record
and reads the configuration-owned graph. Opening adds no HA callbacks or
subscriptions. The existing parser remains authoritative for exact wire values,
escapes, case-sensitive choices, the 64-option/96-byte limits and malformed data.
Oversized/exhausted channels reject the panel. An options change invalidates the
opening, matching the existing close-on-list-change policy; a different encoding
of the same decoded choices leaves it open.

Input pulls fresh state before validating the exact selected value. The service
value is copied before callback-capable dispatch. A rejected dispatch keeps the
panel open; an accepted one closes only that exact current opening. Displayed
values remain authoritative HA state, rather than optimistic acceptance. Rows,
choice strings, context and gesture storage are released on close. Late input,
disconnection, configuration replacement and display takeover invalidate the
opening; teardown waits until input dispatch finishes.

Model tests exercise all nine profiles, both saved forms, labels, raw values,
limits, unavailable states, reconnects and 1,000 reads without allocations.
Native grid tests cover font/layout restoration, full bindings, stale presses,
slot reuse and 1,000 navigation cycles per profile with stable retained resources.
Native panel tests use the production context, parser, list construction and
click callback: both entity domains, exact punctuated/space-containing values,
rejected calls, authoritative selection, changed/equivalent options, 64 rows,
empty lists, oversized state, reconnects, replacement during a service callback
with poisoned old configuration storage, takeover during row construction, and
allocation/gesture/heap/style failures. Each domain runs 1,000 open/select/close
cycles plus 1,000 quiet updates. Test output-capture strings are reserved before
memory baselines; they are instrumentation, not retained panel resources.

Use `ESPCONTROL_TEST_FAMILY=option_select` for the focused native grid suite; the
unfiltered suite includes it. Platform shell geometry/opening are doubled in the
panel fixture. Actual device layout, commands, heap/PSRAM and peak allocations
still require S3 and a working 20-slot P4 qualification, followed by the remaining
profiles. Production selection, remaining drivers, persistence compatibility and
nested-page delivery remain outstanding. Saved formats are unchanged.
