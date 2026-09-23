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
  overlay it; titles grow up to three full lines whether the playback control
  is enabled or disabled, and long artist text truncates with an ellipsis.
- Rectangular screens use a square artwork region and a dedicated metadata
  panel. Artist text may wrap only when the panel has enough vertical space.
- Titles have priority over artist text. Missing titles use an em dash; a
  missing artist hides that line.
- On landscape screens, titles grow naturally up to three full lines on 4.3-inch
  displays, four on 7-inch displays and five on 10-inch displays, using their
  configured line spacing.
  Artist text uses the remaining whole lines with an ellipsis when needed,
  keeping elapsed time above the bottom-right playback button. Portrait retains
  its shorter title limit because its metadata panel sits below the artwork.
- Text must remain high contrast against the sampled dark accent. White is the
  default; an intentional warmer colour is allowed only as a device profile
  choice with equivalent contrast.
- The progress bar stays at the bottom edge and updates only when its rounded
  percentage changes. Elapsed text updates once per visible second.
  Refit artist text when elapsed-time visibility changes, including duration
  arriving after metadata or disappearing for an external input.
- While replacement artwork downloads, keep the previous good image visible.
  When no good image exists, show the black metadata fallback without an error
  message.
- A background touch dismisses the takeover and the configured return delay
  controls when it can appear again. Raw touchscreen wake defers to LVGL while
  cover art is visible, so the playback button cannot also dismiss the screen.
- The bottom-right playback button remains visible independently of metadata.
  Its circle and icon use the active display's width compensation together,
  including the 7-inch profile's 95% correction and portrait axis switching.
  `PlaybackControl` retains a pause only after this session requests it and HA
  confirms it within five seconds. Pending commands suppress repeat taps;
  unconfirmed commands expire. Dismissal, player changes, stopped/unavailable
  media, loss of the HA state connection, and higher-priority display modes clear
  ownership. Real playback state remains
  paused so progress does not advance. Commands target the active routed player.
- A retained local pause reveals track details and cancels their hide timer.
  Confirmed playback resumes the configured square-screen timer from the start;
  a zero duration hides details immediately on resume and -1 keeps them visible.
  Rectangular layouts and
  the no-artwork fallback keep details visible. The button uses 90% of the raw
  extracted RGB values, while the background uses one third; pressing lightens
  the button colour. Choose a black or white icon for the strongest minimum
  contrast across both normal and pressed colours; reset it to white for the
  neutral no-artwork fallback.
- The persistent playback setting defaults on and is saved/restored with the
  other display settings. Only square displays expose and honour the toggle;
  larger displays keep the button. Disabling it releases a retained pause,
  rejects queued button presses, and restores the unreserved metadata layout.
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
