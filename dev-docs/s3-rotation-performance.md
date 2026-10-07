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

### Stage 1 — compiler optimisation (not retained)

- Firmware source: `ac1ab9063`; ESPHome 2026.9.1 and ESP-IDF 5.5.5. The generated
  application compiler command used `-O2` with `PERF` enabled.
- `npm run prepare:ci` passed: 48 CI tasks, 75 host firmware tests, docs build,
  and browser installer checks. Regeneration produced no tracked changes.
- S3 development build passed. Application size: 5,464,976 bytes, with
  1,809,520 bytes remaining in the 7,274,496-byte OTA partition.
- OTA upload succeeded, the panel rebooted into `dev`, and its 180-degree
  setting was preserved. The verified image was uploaded directly after
  `run` unnecessarily started rebuilding the generated application.
- Before flashing, five samples spaced 65 seconds apart held at 85,955 bytes
  of free internal heap and a 31,744-byte largest internal block. The panel
  reported v2.11.2, so this is not a comparison against an otherwise identical
  firmware build.
- After two startup samples, five settled samples ranged from 78,703 to
  85,435 bytes free internally. The temporary dip recovered; the first-to-last
  change was +216 bytes. The largest internal block held at 31,744 bytes.
  Free PSRAM ranged from 1,922,864 to 1,931,260 bytes. These observations do not
  establish long-term stability or the minimum heap during every operation.
- User feedback: updates were about the same and still slower at quarter turns.
  Return compiler optimisation to the default `SIZE` for Stage 2.

### Stage 2 — blocked rotation in the owned RGB driver

The S3 opts into `rotation_backend: driver`; other configurations retain the
existing LVGL rotation path. ESPHome's hardware-rotation metadata delegates
rotation to the driver, but the S3 still rotates in software. Touch coordinates
continue to use LVGL's rotation handling.

The driver replaces LVGL's rotation scratch buffer with an equally sized
one-eighth-frame buffer explicitly allocated in PSRAM. It processes quarter
turns in 32-by-32 blocks without allocating tile storage. Larger flushes split
into source strips so the scratch buffer never grows. This stage retains the
separate copy into the RGB framebuffer to isolate the blocked-copy trial from
the later direct-framebuffer change.

Test all four rotations against an independent pixel reference, including
rectangular panels, odd sizes, source/destination padding, clipped areas, source
offsets, small scratch capacities, and buffer guards. Compile and flash after
the tests pass, repeat settled memory observations, and wait for physical
feedback before Stage 3. Stages 3 and 4 remain pending.

Stage 2 verification:

- Firmware source: `050ed35fe`; generated configuration enables the driver
  rotation backend and restores `-Os`. The driver compiled successfully with
  ESPHome 2026.9.1 and ESP-IDF 5.5.5.
- The new rotation CMake/CTest test passed. AddressSanitizer and
  UndefinedBehaviorSanitizer also passed; leak detection was disabled because
  the sandbox uses ptrace, which LeakSanitizer does not support.
- `npm run prepare:ci` passed all 48 CI tasks, all 76 host firmware tests, docs
  build, and browser installer checks. Regeneration produced no tracked changes.
- Application size: 4,868,080 bytes; OTA headroom: 2,406,416 bytes.
- Upload attempted, but the target's web, API, and OTA ports and ICMP all timed
  out. A second known panel on the same subnet also timed out; the Tailscale
  route probe received no reply. No Stage 2 image was transferred.
- The last confirmed running firmware is Stage 1. The compiler reversion and
  blocked rotation take effect only after connectivity returns and this image
  is uploaded. Post-Stage-2 RAM observations and physical feedback remain pending.
