# First real reusable card port

`PageBasicLvglPort<slotCount>` is the first real LVGL port for the prepared
renderer. It is **not selected by production firmware**. Existing displays keep
the current grid and all its card types until the remaining ports, main-loop
configuration wiring and physical parity tests are complete. It does not enable
nested pages.

The port borrows one profile-sized bank of existing button, icon, label and
sensor widgets. The widget bank, fonts and event owner must outlive the port.
Initialize it on the LVGL/main loop before the first configuration transaction;
place the renderer before the state owner in `PageRuntimeResourcesGroup`.
Preparation compiles card metadata, resolves firmware-lifetime glyphs and reserves
one active/staged arena for metadata, decoded mapping/unit options and reusable
NUL-terminated text buffers.
Configuration strings remain borrowed from PageStore; retained HA values remain
owned by the shared state cache. Navigation creates no widgets, driver contexts,
state owners or transport registrations.

The port covers ordinary toggles (including numeric/text overlays), numeric,
icon, text and duration HA sensors, plain folders and Back. Text sensors retain
the existing Unicode title/whitespace rules and configured state labels. Duration
cards use the shared formatter with automatic or configured source units and
one/two displayed components according to span. Local sensors, folder summaries,
confirmations, action/complex cards and modals still require ports.
Unsupported active variants reject preparation rather than silently dropping
behaviour. Stored pages that are not navigable are preserved without requiring
their cards to be supported by this port. Styling/layout parity, temperature
unit offsets, display takeover integration and real action dispatch remain
cutover gates; this is not a production renderer selection.

Widget event handlers are registered once. A press captures its origin token;
a click must match that token and pass the renderer's current-generation check
before dispatch. Unbind and takeover clear pending gestures. Action callbacks
receive the validated origin and borrowed entity, and must consume it
synchronously or copy/tag any deferred work before returning. No callback may
retain a borrowed state/configuration view across replacement.

`copy_display_text` reads explicit lengths, writes to reserved buffers, preserves
complete UTF-8 characters and applies the existing S-with-dot-below fallback.
Malformed UTF-8/NUL bytes are excluded from display buffers without altering
stored configuration. The direct copy helper reports malformed input. The copy API can report changes when given a
fully initialized destination, allowing unchanged labels to skip LVGL updates.
Numeric parsing terminates a bounded copy of the packed cache value before
calling libc; it never reads the following packed state as part of a number.
The ordinary decimal path retains the existing integer formatter's rounding
and avoids floating-point printf for normal values. A defined fallback handles
large finite values which would overflow its integer intermediate.

## Verification and allocation limits

The normal firmware host suite compares bounded text output with the existing
formatter across 1,000 generated Unicode/whitespace strings and their byte
limits. It tests state-label mappings, time-unit changes, manual-unit dependency
counts, strict bounded duration parsing and allocation-free value reads. It also
tests metadata, all nine profiles, unsupported
variants, packed state boundaries, reconnect invalidation, decimal formatting
and bounded UTF-8 copying. The optional native LVGL integration uses the actual
LVGL 9.5.0 sources from the pinned ESPHome build:

```sh
cmake -S tests/firmware -B .cache/lvgl-native \
  -DESPCONTROL_LVGL_SOURCE_DIR=/absolute/path/to/managed_components/lvgl__lvgl
cmake --build .cache/lvgl-native --target page_basic_lvgl_port_test
ctest --test-dir .cache/lvgl-native -R '^page_basic_lvgl_port_test$' --output-on-failure
```

The integration runs 1,000 navigation cycles per profile with real buttons,
labels and grid layouts. It counts C++ allocations, arena reservations and
LVGL's allocator separately, checks stable widget/channel/retained-byte counts,
and exercises delayed clicks, 100 successful replacements per profile, failed
replacement, Back, reconnect and takeover. Repeated refreshes of unchanged
labels do not schedule new LVGL events. The fixture includes text/duration
sensors and a configured 600-byte state label, which is reserved before
activation and displayed without truncating configuration.
It is a host layout test, not a rendered-screen, network or physical-touch test.
The real device toolchains instantiate the port without retaining an instance.

The complete LVGL test is **not allocation-free**: LVGL 9.5 allocates temporary
label-refresh event descriptors and grid-layout scratch storage. In the current
fixture, 1,000 cycles schedule 9,000 label-event allocations and 5,000 layout
allocations per profile. They are released after layout; retained LVGL bytes
stay constant. Port-owned buffers and C++ state do not allocate during those
cycles. These counters do not include image/draw-buffer/network work, and the
host byte counts are not device heap/PSRAM measurements. Complete allocation-free
label updates remain an explicit qualification issue before production cutover.
