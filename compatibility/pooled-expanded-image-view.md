# Demanded expanded-image viewer

This #1825 prerequisite supplies the visible expanded viewer for the
[pooled image source](pooled-expanded-image-source.md) and its
[bounded cache](pooled-modal-image-cache.md). Production still selects the
existing renderer. Application-scene selection, concrete device memory-policy
wiring, compiled legacy retirement and physical qualification remain. No firmware
was flashed and no device memory improvement is claimed.

`PageExpandedImageModalPort` owns one demanded context and an eight-entry gesture
reservation. It uses the common control-modal shell, existing Back styling and
350ms opening close guard. A generic draw surface replaces an attached image
widget; a separate full-panel loading surface retains the current icon, centered
translated Loading/Unavailable labels, theme colors and opacity. Fit/fill uses
the already-tested cover-art draw helper. Modal captions use configured labels or
the owned graph's 64-byte/NUL-limited friendly name, falling back to the entity.
No legacy ImageCardCtx, image callback array, subscription or timer is created.

The context owns configuration/trigger/caption strings until retirement is
collected. Source ownership is marked before bind can synchronously close the
opening, and unbind occurs before those strings are destroyed. Failed metadata,
style, gesture, shell or source admission cleans up the same demanded record.
The style resolver supplies actual page/profile fonts, width compensation and
fit background and is refreshed during maintenance for theme/geometry updates.
The application scene receives a covered flag to keep original grid bindings
alive while suppressing tile work, animation and input underneath the overlay.
That callback must change scene state only; UI changes and resumed work belong
in safe maintenance.

`PageArtworkExpandedImageView<2/6>` selects either the separate modal component's
current descriptor or the exact transferred tile preview. A frame identity
includes expanded-versus-preview role, the full preview binding, asset and source.
Separate tile/modal counters can both equal one without confusing those roles.
Every draw and extent query pulls fresh descriptors and rechecks coherent frame
identity plus full modal admission after the second read. A reentrant close can
return an apparently unchanged descriptor; admission still rejects it. No source
pointer is retained in the widget or context. Concrete image release continues
to wait for pending hardware draw work through the existing adapter fence.

Retirement invalidates value/gesture ownership immediately. It does not hide or
delete widgets from inside HA/LVGL/native dispatch: LVGL forbids invalidating an
object during rendering. The safe maintenance collection deletes the overlay
and gesture descriptors afterward. This also lets configuration publication
retire a viewer without freeing borrowed context metadata during a draw callback.

The real-LVGL test uses the actual demanded adapter, gestures, entity graph and
modal runtime with shell geometry and source I/O doubled. Each of nine device
profiles executes 1,000 stable open/ready/close cycles after warm-up, comparing
retained C++/LVGL bytes, allocator records and HA channels. It checks captions,
loading/preview/unavailable states, stray/retired clicks, synchronous close during
bind, admission/setup failures, reconnect, discarded staging, generation-zero
replacement and overwritten old document bytes. Actual drawing exercises a role
change between coherent reads, a close during the second read and retirement
inside an image draw-task callback; destruction waits until dispatch ends.
Seven representative portrait/landscape/square dimensions cover loading geometry
and live theme colors. These tests do not substitute actual device font/layout
comparison or physical memory measurements. Actual S3/P4 SDK checks compile the
complete common shell and both concrete bridge pool sizes; strict GCC and
sanitizers supplement the native test. Default CI remains separate from optional
real-LVGL tests and current factory firmware builds.

After scene integration, qualify the Guition 4848S040 S3 first, then a working
twenty-slot JC8012P4A1 P4. Compare Back/loading appearance, friendly captions,
fit/fill, cached preview, motion/periodic refresh, rotation/theme changes, repeated
close/reopen, reconnect and screensaver takeover. Measure matched heap/PSRAM,
largest block, minimum heap, decode peaks and opening latency. Keep P4-86 reserved
for its USB diagnosis. Configuration, backup formats and nesting depth remain
unchanged at this stage.
