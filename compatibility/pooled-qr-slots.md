# Visible QR leases

This #1825 prerequisite adds a bounded owner and concrete LVGL adapter for the
[packed QR renderer](pooled-qr-matrices.md). Production still selects the existing
renderer. QR card/modal adapters, application-scene selection, compiled legacy
retirement and physical qualification remain. No firmware was flashed and no
physical memory improvement is claimed.

`PageQrRuntime` preflights one external-arena lease per selected visible slot,
independent of the smaller image download pool. Each lease owns a fixed packed
matrix, a maximum 255-byte copied pending payload and value-only binding/state
metadata. `PageLvglQrFrames` preflights matching persistent descriptors in the
same allocator and one scene decoder. The scene supplies a shared encoder scratch
buffer. No resource count depends on hidden configured QR cards.

Bind, sizing, unbind and retirement allocate nothing, encode nothing and change
no LVGL styles. Revocation immediately hides old drawing; descriptors and pixels
remain untouched until safe maintenance. Work copies its input into a bounded
local buffer before crossing the draw fence, encoder or frame adapter. Full
binding and unique 64-bit operation identity are rechecked afterward. Replacement
at any boundary cannot publish the old result or overwrite a newer intent.
Nonce exhaustion hides the old frame and fails closed, without wrapping.

Only maintenance may reset descriptors or write pixels, after the hardware fence
and all decoder sessions have finished. Held sessions defer reuse and shutdown;
shutdown refuses new binds until the caller can safely retry. Invalid payloads,
failed encoding, wrong-sized encoder output and rejected descriptor publication
leave no visible QR. Unchanged sizing and idle pumps do no work. The binding
origin deliberately has no HA connection requirement: local QR contents remain
usable offline, while guest controls must use their separate connection guard.

QR decoder registration now uses the pinned LVGL 9.5.0 constructor's checked
list insertion directly. The public constructor asserts when allocation fails;
scene preflight instead returns failure and releases both arenas normally.
Registration remains protected by the existing version guard and is paired with
LVGL's public deletion helper. The scene must hold the LVGL lock for preflight,
maintenance and final shutdown, and keep these calls outside dispatch. Value-only
retirement is safe during dispatch.

Pure sanitizer tests run 1,000 complete visible-bank replacements on each of nine
profiles, with no bind/size/unbind allocation or arena growth. Tests overwrite
old payload bytes, publish configuration generation zero, reject stale identities,
exercise preflight/encode/adapter failure and the last nonce, and replace bindings
reentrantly at the fence, descriptor reset, encode and publication boundaries.
The real-LVGL test fills every profile's visible capacity and then performs 1,000
encode/draw/replacement cycles per profile with stable retained C++/LVGL/arena
bytes. It also injects failures into both arena allocations and decoder
registration, and holds a real decoder session across retirement and shutdown.
The earlier 7,597,818-pixel legacy parity comparison continues to pass.

After card/modal and scene integration, flash Guition 4848S040 S3 first and a
working 20-slot JC8012P4A1 P4 next. Qualify QR scanning, compact/spanning layout,
rotation, repeated navigation/configuration replacement and offline access, then
compare matched heap/PSRAM, largest block, minimum heap and draw peaks. Keep
P4-86 reserved for USB diagnosis. Document/backup formats and navigation depth
remain unchanged by this stage.
