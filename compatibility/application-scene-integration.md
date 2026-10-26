# Application scene integration

This #1825 stage connects a prepared scene's lifetime to the actual ESPHome
application. It also incorporates current main, including ESPHome 2026.10.0 and
stale forecast cards retaining normal geometry. The concrete pooled scene and
device widget/port wiring are still being assembled; production still selects
the existing renderer. No nesting, migration, backup format or physical memory
saving is enabled by this bridge alone.

`ApplicationSceneRuntime` implements the existing configuration runtime adapter
and provides structural readiness, safe-maintenance readiness, loop and stop.
The firmware application's UI owner installs one fully initialized scene before
native configuration begins. The scene and its resources outlive the component.
An unready scene, replacement owner, late installation or shutdown registration
is rejected. Reinstalling the same ready owner is idempotent. A running legacy
grid is never silently replaced.

When installed, the actual application chooses this adapter for the native
configuration service, so browser writes and boot restore use the existing
prepare/commit/activate protocol and downgrade mirror. Without an installed
scene the legacy adapter and startup timing remain selected.

The application only processes queued configuration transactions when scene
maintenance is safe. The initial boot timeout still establishes the restoration
delay. If a producer/native/LVGL callback boundary is busy when it fires, boot
application stays pending for a later safe loop without allocating more timers.
The reload uses the latest durable document, so a browser save cannot be undone
by an earlier startup snapshot. Scene readiness is queried again after HA/core
maintenance before the scene's loop runs.

Shutdown stops the transaction queue before revoking scene input/actions. The
scene's stop implementation must defer widget/context collection if dispatch is
active. Further app loops, delayed boot application and installations are
rejected after shutdown. This does not add an alternative store, HTTP path,
per-card callback or document copy.

Host sanitizer tests extract the complete production application methods and
exercise them with the real configuration service, durable store and main-loop
transaction queue. ESPHome endpoint/core/device storage objects are doubled.
Checks include selected/legacy adapter behavior, registration timing, queued
browser writes while busy, a due boot restore reloading the newest committed
document, prepare failure, an HA callback changing readiness, pending requests
released during shutdown, and 1,000 startup/restore/shutdown cycles.

Current-main forecast behavior is shared by the legacy widgets and pooled draw
frame: stale status stays in the details modal and cannot shrink card geometry
or introduce the retired side-by-side list. The independent forecast widget
checks and native frame parity remain required after integration.

Next assemble and install the concrete scene, retire compiled eager grid/action
wiring, qualify all current factory builds and capture matched S3/20-slot P4
runtime memory and interaction behavior. Keep P4-86 reserved for its separate
USB diagnosis. Device testing and storage/compatibility gates precede nested
pages; do not merge hardware-dependent work before user confirmation.
