# Demanded expanded-image source

This #1825 prerequisite supplies one expanded camera/image source for the pooled
renderer. It uses the [shared tile refresh handoff](pooled-image-refresh-handoff.md)
and the separate [typed modal image lease](pooled-modal-image-ownership.md).
Production still selects the existing renderer. [Bounded decoded-cache reuse](pooled-modal-image-cache.md) has model coverage.
Expanded widgets, application-scene wiring, legacy retirement and device qualification
remain required before cutover or any RAM-saving claim.

`PageExpandedImageSource<2/6>` owns one fixed observer/record for the scene. It
borrows entity/trigger metadata from the demanded modal context, which must call
`unbind` before destroying those strings. Opening, closing, source delivery and
handoff allocate no C++ storage and register no additional HA channels. URL
construction and download admission run only in maintenance, outside HA, native
and LVGL dispatch. A shared URL helper preserves tile proxy/token and public URL
selection while capping expanded decode dimensions as well as proxy dimensions.
Requests use MODAL priority, the existing headroom gate and a 100ms opening delay.

An existing tile must transfer its policy successfully; a hidden-card opening
seeds independent state from the graph. The observer owns that state before
cancellation can deliver fresh reports. Other pending tile transfers are cancelled
with completed buffers retained and deferred by one second. The scene must also
mark the grid covered/expanded to stop new tile work and hidden activity replay.
Grid bindings stay alive underneath the image overlay. Screensaver image takeover
on the S3 must instead release the tile pool before its larger allocation.

The source preserves periodic/activity windows, sustained motion on entry, the
last tile completion guard, separate tile/modal image-revision acknowledgements,
and camera error cooldowns. A rotated token alone cannot bypass a closed motion
window. New image revisions coalesce while a request runs; a changed camera URI
also remains queued for its next eligible request instead of being lost during
pending work. Failed admission/downloads use 2/4/8/16/30-second retry backoff.
Unavailable/recovery hides old pixels and explicitly fetches one fresh frame;
this avoids the old unavailable latch when a motion window has already closed.
Closing returns policy without an unconditional healthy tile refresh.

Snapshots prefer a healthy expanded asset, otherwise an exact transferred tile
binding. Unavailable/error, stale geometry/endpoint, reconnect and exhausted
request identity hide both. The UI must distinguish tile-preview and expanded
asset roles: their separate counters can have the same numeric value. Validate
the full opening and role-specific asset/binding at draw time, including after
external draw callbacks. `content` exposes a borrowed cache-key candidate only
for an applied, clean, completed expanded result. It does not retain pixels; a
cache copies metadata during safe maintenance and explicitly parks the raw pixel
lease when its environment permits retention; otherwise unbind authorizes deferred
pixel release.

Model tests combine the real entity graph, renderer and modal lifecycle with
mock image I/O. Each of nine profiles runs 1,000 navigation/handoff cycles and
1,000 demanded open/download/complete/close cycles without adding channels;
bind, graph delivery and close allocation guards are checked separately. Tests
cover hidden sources, healthy tile preview, size caps/fit/background, pending
revision coalescing, motion/token timing, unavailable recovery, admission/error
backoff, other-tile preemption, stale replies, synchronous completion, cancellation
that closes the modal, discarded configuration staging, private UINT32_MAX to
published generation-zero replacement during request start with old arena bytes
overwritten, and final request-nonce exhaustion. Image/core/adapter regression
suites supplement these tests. SDK syntax checks and native LVGL tests do not
replace factory builds, measured heap/PSRAM or physical testing.

For device qualification after scene integration, use the Guition 4848S040 S3
first and a working twenty-slot JC8012P4A1 P4 next. Exercise motion and periodic
cameras, static image revisions, repeated close/reopen, unavailable/reconnect,
configuration replacement and competing tile downloads. Record decode peaks and
matched idle/total memory separately. The P4-86 remains reserved for its existing
USB boot diagnosis. This stage changes no configuration format, backups or
supported nesting depth.
