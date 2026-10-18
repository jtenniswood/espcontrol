# Bounded QR rendering

This #1825 prerequisite replaces full-size QR canvases with a fixed packed
module bitmap and a streaming LVGL draw helper. Production still selects the
existing renderer. Visible-slot ownership, card/modal integration, application
scene selection, compiled legacy retirement and physical qualification remain.
No firmware was flashed and no device memory improvement is claimed.

`PageLvglQrEncoder` accepts the existing codec's maximum 255-byte payload. Each
matrix reserves 668 bytes for packed modules, plus its layout; one shared
668-byte encoder scratch buffer is reused sequentially. Allocate these resources
with the scene's preflighted storage rather than on the small UI task stack.
Encoding performs no C++ or LVGL allocation. Hidden configured cards must retain
only prepared payload text, not a matrix, canvas or widget.

Compact encoding preserves the existing non-hidden suffix shortening, LOW error
correction and integer quiet-zone layout. Standard encoding preserves pinned
LVGL 9.5.0's MEDIUM correction, version selection, strict tie handling and module
rounding. Both preserve automatic masks and error-correction boosting. The
version bound includes the two additional version candidates used by LVGL.
Compile-time guards require a deliberate parity audit when LVGL changes.

`PageQrDecoder` installs once per scene. It recognizes only its own signed,
owner-tagged internal source descriptors. Each active decode allocates one
aligned target-width A8 row and a small session record; repeated module rows
reuse the same mask. It draws at the final size without an image transform,
retained decoded pixels or a full-size temporary canvas. Black module edges and
white quiet zones match the current canvas exactly. Session and row allocation
failure clean up without publishing a partial frame. Shutdown refuses live
frames or decode sessions.

`PageQrFrame` owns the persistent descriptor; LVGL never receives a stack source.
Hide revokes drawing without touching descriptor or pixels. The caller must hide,
wait for pending hardware draw work, and call `reset_after_draw` before changing
the matrix, reassigning a frame or releasing scene storage. Frame reset drops
image/header cache entries. Full binding and asset admission remains the visible
owner's responsibility. Bind/navigation/configuration activation must queue work
rather than encode or overwrite resources that an old draw may still read.

The pure layout test covers sizes through 1280 pixels, version bounds, compact
suffix compatibility and malformed layouts. The optional real-LVGL test compares
98 payload/size/layout cases against both the production compact encoder and
`lv_qrcode_update`: 7,597,818 canvas and rendered pixels match. A further 1,000
frame replacements keep retained C++/LVGL bytes stable. Tests also exercise
clipped row decoding, separate decoder ownership, allocation failures and held
sessions across frame retirement. Sanitizers, strict GCC and actual S3/P4 SDK
header compilation pass. SDK header checks are not factory firmware builds or
physical device tests.

After owner and scene integration, qualify the Guition 4848S040 S3 and a working
20-slot JC8012P4A1 P4. Check compact/spanning QR cards, modal QR size, rotation and
repeated navigation/configuration replacement; scan real credentials and measure
heap/PSRAM, largest block and draw peaks against matched legacy workloads. Keep
P4-86 reserved for its USB diagnosis. Document and backup formats and the current
nesting limit are unchanged by this component stage.
