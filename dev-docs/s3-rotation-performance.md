# S3 rotation performance trials

The Guition ESP32-S3-4848S040 updates more slowly at 90 and 270 degrees than at
0 and 180 degrees. Its RGB display uses software rotation. Quarter-turn copies
have poor memory locality and require a rotated intermediate buffer before the
pixels are copied into the PSRAM framebuffer.

## Trial order

Implement, compile, and flash one stage at a time to the S3 at `192.168.6.105`.
Wait for the user's physical-display feedback before proceeding to the next
stage. Keep successful changes; revert a trial if it regresses the display or
memory behaviour. Do not merge before the user confirms device testing.

1. **Compiler speed optimisation.** Change only the S3's compiler optimisation
   from `SIZE` to `PERF`. Record application size and runtime memory. This is a
   configuration experiment, not a proven rotation speed improvement.
2. **Improve quarter-turn memory access.** Replace the column-wise rotation
   copy with small-block processing. Prefer reusing existing buffers. Verify
   90/270-degree pixel placement, byte order, partial areas, odd sizes, clipping,
   and 0/180-degree behaviour with host tests before flashing. Keep P4 behaviour
   unchanged. Decide whether this belongs in the owned RGB driver or an upstream
   LVGL change after inspecting the actual integration.
3. **Rotate directly into the RGB framebuffer.** Combine rotation and the final
   copy, removing the intermediate PSRAM rotation buffer if the integration
   permits it. Preserve RGB scan-out, bounce buffers, touch rotation, and partial
   redraw correctness. Measure whether the new destination stride offsets the
   benefit of removing one copy before retaining this stage.
4. **Audit redundant redraws if still necessary.** Measure invalidated areas and
   update frequency. Skip setters whose displayed value did not change where
   this is not already handled. Preserve responsive controls, status updates,
   and the existing appearance. Only change paths supported by measurements.

## Memory and regression checks at every stage

- Keep the 32 KiB data cache and two 20-row RGB bounce buffers unchanged.
- Record free internal heap, largest internal free block, free PSRAM, and largest
  PSRAM free block before flashing and after startup has settled. Existing memory
  sensors update once per minute; faster polling is not an independent sample.
- Observe at least five settled minute-spaced readings. Record ranges and trend,
  not just one favourable snapshot. A short observation does not establish
  long-term stability or replace testing under load.
- Investigate a sustained drop in free internal heap or its largest block. Use
  4 KiB below the comparable settled baseline as an investigation trigger, not a
  guarantee that a smaller difference is safe. Account for different firmware
  versions, active screen, network clients, images, and configuration.
- During user testing, repeat page/modal navigation and control updates at all
  four rotations; check touch alignment, tearing, corruption, and responsiveness.
  Exercise artwork/image loading and a normal OTA update before final acceptance.
- Do not shrink transfer buffers, raise panel or PSRAM clocks, or move a large
  drawing buffer into internal RAM as part of these trials.

## Evidence

Initial live snapshot, before any trial: internal heap 86,067 bytes, largest
internal block 31,744 bytes; free PSRAM 2,458,976 bytes, largest PSRAM block
2,293,760 bytes. This is a point-in-time reading, not a stability result.

Stage 1 build, flash, and observation results will be recorded here after the
checks run. Stages 2–4 remain pending user feedback and supporting measurements.
