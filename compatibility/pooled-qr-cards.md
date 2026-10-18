# Pooled Wi-Fi sharing cards

This #1825 prerequisite ports `wifi_qr` buttons and `wifi_qr_card` inline tiles to
prepared metadata and the [visible QR owner](pooled-qr-slots.md). Production still
selects the existing renderer. The demanded QR/details/guest modal, application
scene selection, compiled legacy retirement and physical qualification remain.
No firmware was flashed and no physical memory improvement is claimed.

Preparation decodes the existing Wi-Fi codec once into reserved text storage.
The payload reuses the definition's mutually exclusive text-view field and its
tile flag reuses the options union: the host `BasicCardDefinition` remains 264
bytes. Hidden cards retain prepared text and scalar metadata only. Details,
password labels, QR matrices, timers and modal contexts are not created per
configured card. Malformed credentials preserve the legacy blank inline tile
and visible normal button; they cannot open a sharing modal.

Normal buttons retain their configured/Wifi icon and translated Connect default
or custom label. Both forms route a full, press-captured binding to the sharing
modal callback. They never call a switch/toggle service, even if the configured
guest entity is a switch. Static modal opening is independent of HA state and
connection; the demanded guest controller retains its separate connection guard.
A bare click or a click after page/configuration replacement cannot adopt the
replacement card's identity.

Inline tiles reuse the generic draw surface and hide ordinary icon, caption and
sensor widgets. The card and quiet-zone surface stay white in normal, pressed
and checked states, including custom-color configurations. An isolated style
state preserves other card families' pressed styles after reuse. QR styles and
background properties are reserved during initialization, before first binding.
The surface accounts for both padding and card borders, matching legacy canvas
centering. The white quiet zone is a rectangle behind the streamed module mask;
it is not a retained pixel canvas.

`PageQrTileView` separates the card port from concrete pixel ownership.
`PagePooledQrTileView` uses physical shell indices in the fixed visible bank,
checks the full binding/asset on drawing and applies the existing compact 1×1
versus spanning layout rules and 48-pixel minimum. Preparation rejects missing
callbacks, draw surfaces, unavailable owners and banks smaller than the selected
profile. Bind/sizing queue work; the application loop must maintain the QR owner
outside HA/LVGL/native dispatch, then refresh the renderer when its revision
changes. Unbind revokes visibility before a later draw-fenced reset.

Pure metadata tests cover both forms, prepared payload ownership, invalid
credentials and unchanged definition size, with forecast enabled and disabled.
The real-LVGL card test uses the actual renderer, prepared graph, visible owner,
frame decoder and bridge with a deterministic module encoder; actual QR encoding
and 7,597,818-pixel legacy parity are covered by the separate QR matrix test.
Each of nine profiles exercises 1,000 Home/subpage round trips with stable arena,
C++/LVGL bytes and registrations. Checks cover initial-bind/navigation allocation,
white state styling, compact/spanning geometry and black/white pixel positions,
default/custom captions, guest reports/reconnect, valid/invalid/stale modal taps,
failed admission, discarded staging and generation-zero replacement after old
configuration bytes are overwritten. No new graph registration is added by a
visible QR tile.

After modal and scene integration, qualify Guition 4848S040 S3 first, then a
working 20-slot JC8012P4A1 P4. Scan compact/spanning cards and the sharing modal,
check Back/tab/credential/guest controls, rotation and offline access, repeat
navigation/configuration replacement, and compare matched heap/PSRAM, largest
block, minimum heap and drawing peaks. Keep P4-86 reserved for USB diagnosis.
Document/backup formats and navigation depth are unchanged by this stage.
