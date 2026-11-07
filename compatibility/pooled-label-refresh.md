# Pooled label refresh callbacks

The pinned LVGL 9.5.0 schedules a changed label by allocating a display event
descriptor. Its refresh callback removes that descriptor, then checks and clears
the label's refresh flag. Persistent text buffers alone therefore did not remove
all label-update allocations.

`PageLabelRefresh` retains those descriptors for the selected renderer's fixed
label bank. The original LVGL callback continues to handle text sizing and
refresh flags. GNU linker wrappers intercept display callback addition/removal
only for the discovered label callback, update-layout filter, selected display
and whitelisted label addresses. Everything else forwards to LVGL. The component
registers the linker flags through normal ESPHome code generation, following the
existing OTA-wrapper mechanism. No generated SDK cache, private flag or library
source is patched.

Initialization owns a spare, initially blank bank on one screen/display. It drains
constructor callbacks, blanks one label to identify the library's own callback
through public event-descriptor accessors, reserves a sorted arena table and
prewarms one persistent display descriptor per label. Ordinary updates use a
bounded binary search and neither reserve memory nor create widget/context state.
Keeping the callback registered also invokes a cheap no-work check at idle layout
completion; physical latency must be qualified with the final renderer.

Per-label deletion guards remove persistent descriptors before the label's
destructor. A display-deletion guard disables interception before LVGL clears its
event list and deletes the screen objects. Shutdown must run outside LVGL event
and timer dispatch. It completes pending label refreshes before releasing the
descriptors, allowing surviving labels to resume normal scheduling. The bank must
be shut down before moving it to another display. The application-owned cache and
allocator must outlive its labels or a successful shutdown. Do not select or reuse
a bank whose initialization failed.

Only one bank is selected at a time. With no selected bank, including every
current production build, the wrappers forward all calls unchanged. The exact
LVGL-version guard requires another audit if the dependency changes. The existing
read-only dispatch guard is separately pinned to the same version.

Native tests cover 24, 36, 60 and 80 labels over 1,000 update/layout cycles, with
zero LVGL allocations in that isolated path. They verify deletion during display
callback iteration, display teardown, outstanding refreshes at shutdown, shutdown
refusal during dispatch, allocator/duplicate/ownership failures, another display's
untracked labels and normal scheduling after release. The pooled card fixture
covers 1,000 navigation cycles on all nine profiles plus configuration replacement
and portrait layout. It records zero label-event/C++/arena allocations and 4,000
remaining grid scratch allocations per profile. Address and undefined-behaviour
sanitizers also cover the real climate-modal adapter sharing this library.

The fixed callbacks increase retained memory: the current native fixture uses
23,501 LVGL bytes on the nine-slot profiles and 44,720 on the twenty-slot profiles,
2,360 and 5,176 bytes above the preceding fixture. The separate arena table uses
576 and 1,280 host bytes respectively. These host figures exclude draw buffers,
images, network state and the complete application. They establish bounded
ownership, not ESP32 heap/PSRAM savings or the final minimum-heap gate.

Darwin's test linker lacks GNU `--wrap`; optional native tests route only the
pinned label translation unit through the same wrappers using compile definitions
and an original-display-function shim. The standalone label-lifecycle test also
passes with actual GNU `--wrap` in the pinned ESPHome Docker image. Other native
platforms use GNU wrapping.
The real S3 SDK syntax check includes the implementation and application. Actual
firmware linking and physical text/style/touch/rotation/reconnect measurements
remain required before the bank is selected in production.
