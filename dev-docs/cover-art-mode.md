# Cover Art Mode

Cover art is a shared now-playing takeover, not a separate implementation for
each display. Preserve the existing user experience while keeping policy,
layout, downloading, and presentation responsibilities distinct.

## Ownership

- `components/espcontrol/cover_art.h` owns testable policy, runtime-state,
  progress, constants, and device layout decisions.
- `common/device/screen_cover_art.yaml` wires Home Assistant attributes and
  LVGL widgets to those helpers.
- `components/artwork_image/` owns URL validation, transfer, decoding, memory
  budgets, and safe image-buffer replacement.
- Device `packages.yaml` files provide fonts and genuine hardware-specific
  presentation values. Do not add device-slug branches to the YAML screen.
- `common/addon/backlight.yaml` owns display takeover suspend/resume behaviour.
  Cards and modals must not hide cover-art widgets or edit its state directly.

## Presentation Contract

- Square screens use artwork as the full background. Metadata may temporarily
  overlay it and long artist text truncates with an ellipsis.
- Rectangular screens use a square artwork region and a dedicated metadata
  panel. Artist text may wrap only when the panel has enough vertical space.
- Titles have priority over artist text. Missing titles use an em dash; a
  missing artist hides that line.
- Text must remain high contrast against the sampled dark accent. White is the
  default; an intentional warmer colour is allowed only as a device profile
  choice with equivalent contrast.
- The progress bar stays at the bottom edge and updates only when its rounded
  percentage changes. Elapsed text updates once per visible second.
- While replacement artwork downloads, keep the previous good image visible.
  When no good image exists, show the black metadata fallback without an error
  message.
- A touch dismisses the takeover and the configured return delay controls when
  it can appear again.
- An optional secondary media entity may become the active player while the
  primary player reports TV, line-in, or HDMI. Routing switches the complete
  playback presentation together and invalidates callbacks and artwork owned by
  the previous entity.

## Memory and Performance Contract

- Decode at the device profile's configured size; do not silently increase it.
- The compressed download buffer must not exceed the smaller of its absolute
  safety ceiling and the raw target image size.
- Keep the active image during replacement, but expose peak download-buffer and
  heap diagnostics at debug log level.
- Shrink temporary compressed buffers after every completed, cancelled, or
  failed transfer.
- Recalculate layout only when the screen dimensions or rotation changes.

## Required Checks

Run these before compiling firmware:

```bash
npm run check:cover-art-contract
npm run check:firmware-ha-bindings
npm run check:firmware-modals
python3 scripts/generate_device_slots.py --check
```

For behavioural changes, extend `check_cover_art_contract.py` with the event
sequence before modifying the implementation. Important sequences include a
track changing during download, stop during retry, reconnect, external-input
takeover, rotation, and rapid play/pause changes.
