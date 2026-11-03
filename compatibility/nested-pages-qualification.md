# Nested-page qualification

Recorded on 3 November 2026 for PR #2424, following runtime PR #2400.
Production still writes native v1 at depth one. Private qualification images
enable native v2/depth two without declaring a published reader floor.

## S3 memory comparison

The before image rebuilds the starting development source `36f482d7c`; it is
not a bitwise readback of the previously running flash. The corrected checkpoint
after image is private source `8de157565`, based on `95037c8c3`, labelled
`lab-nested-95037c8c3`. Both use ESPHome 2026.10.1 and ESP-IDF 5.5.6 on the same
physical 4-inch S3, with its saved settings and plaintext API.

The corrected runs apply the same decoded flat workloads in the same order.
Reverse HA registration checks verify all nine and 72 expected Sensor states,
then disconnect the observer. Each workload waits 130 seconds before three
existing standard memory-sensor publications 61 seconds apart. No extra
firmware diagnostic RPC, browser or fabricated HA feed runs during sampling.
Values below are medians. Total means combined free internal heap and PSRAM;
it does not mean firmware image size.

| Workload | Internal heap free, before → after | PSRAM free, before → after | Combined free change |
| --- | ---: | ---: | ---: |
| Original dashboard | 76.1 → 79.8 KiB | 2276.5 → 2108.6 KiB | −164.1 KiB |
| Empty | 89.2 → 81.5 KiB | 2349.1 → 2323.0 KiB | −33.9 KiB |
| Nine sensors | 85.9 → 80.8 KiB | 2435.5 → 2401.6 KiB | −39.0 KiB |
| 72 sensors in nine folders | 43.2 → 74.0 KiB | 2217.4 → 2349.6 KiB | +163.0 KiB |

The original dashboard includes a cover-art media card. Its combined free range
is 2,409,011–2,409,039 bytes before and 2,240,747–2,240,951 after. The protocol
does not directly observe image buffers or display mode; that regression is
not yet isolated. Real HA content can update, and fixed-order replacements can
retain caches. Subtracting rows therefore does not isolate per-card costs.
These are settled measurements, not navigation or image/decode peaks.
The maximum Sensor layout also improves the largest internal block from
24,576 to 31,744 bytes. Its largest PSRAM block falls from 2,228,224 to
1,998,848 bytes, so increased combined free memory does not imply that every
allocation shape improves.

Earlier synthetic fixtures put Sensor IDs in the action field rather than the
Sensor field, so their nine-sensor and maximum-layout claims are withdrawn.
The original raw samples and failed proofs remain preserved. An observer issue
in a later attempt restarted the subscription stream before its last entry;
that attempt is also excluded. The accepted pair waits for delayed grid refresh
and drains the complete stream before retrying. Both cohorts restore exact
original dashboard data and verify all 78 settings/PIN status; final firmware
and automatic-update cleanup is tracked separately.

This checkpoint includes newer main changes and larger RGB bounce buffers, so
it is an end-to-end build comparison. Static RAM falls from 206,455 to 204,503
bytes. OTA image size grows from 5,019,968 to 5,260,592 bytes. ELF code and
read-only sections grow by 240,752 bytes. Both builds enable instruction and
read-only-data copies into PSRAM, so code growth offsets runtime savings; the
S3 also spends 19,200 more internal bytes on its 30-row RGB bounce buffers.
Section sizes are separate accounting, not exact live PSRAM reservations. See
the [pinned IDF explanation](https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32s3/api-guides/external-ram.html#move-instructions-in-flash-to-psram).

The subsequent visible-order admission guard `ac0fad1ff` passes its focused
host regressions. Its firmware images and factory matrix remain to qualify;
these checkpoint measurements do not claim to test that later build.

## Physical checks and recovery

- The earlier instrumented S3 image `10fd5ea69` completed 1,000 cycles, 3,000
  actual page changes and three API reconnects. All 21 checked retained-resource
  fields stayed unchanged; native bytes and generation also stayed unchanged.
  This source predates the later backup-negotiation and byte-quota fixes.
- The actual embedded editor exercised nested creation/save/reload,
  immediate-parent Back, the depth limit, subtree code paste/delete and exact
  graph backup import. An old-format replacement left bytes/generation intact.
- Empty, 81-card flat and 97-card nested documents committed. The 55,997-byte,
  657-card shape returned HTTP 413 without changing the preceding document.
  The current candidate advertises the real 40,932-byte durable limit.
  Those synthetic Sensor references were unbound; state-registration load
  qualification is being repeated with corrected Sensor fields.
- Direct rollback with stored v2 data left the old v1-only configuration API
  unavailable (HTTP 503), without erasing the data. Recovery used a qualified
  dual-reader/v1-writer bridge and an explicit save of the complete original
  flat dashboard as v1 before installing the original-source image.
- After the initial memory cohorts, the original-source firmware, exact original
  native-v1 bytes, all 78 saved settings, PIN/reset status and original automatic
  update setting were verified restored. This recovery of a known flat backup
  does not make arbitrary nested data readable by old v1-only firmware.
- On the 10-inch V3, private image `7e962eb3f` (base `7c0abe97a`) admits a
  corrected 58-card navigation fixture with 56 Sensor channels and a 419-card
  fixture with 398 Sensor channels. The latter uses short sensor identifiers
  and occupies 38,838 native bytes. Both retain a 180-widget pool with bounded
  model/state resources and no allocation/accounting failures. Longer 49,972-
  and 52,558-byte fixtures return HTTP 413 without changing the preceding
  configuration. The full 7,620-card shape is not submitted because its
  948,591-byte representation exceeds the advertised transport ceiling.
  Original dashboard data and all 79 settings/PIN/reset status are retained;
  the exact cached original firmware and native-v1 dashboard are now restored,
  with all 79 settings/PIN/reset and both original automatic-update settings
  verified. Other P4 tests remain in progress.
- The actual 10-inch embedded editor also passes creation inside an existing
  folder, a Sensor leaf bound through its Sensor field, reload, immediate-parent
  Back and the two-level creation limit. Both saves are atomic; root records,
  native settings, a sibling folder and stored inactive page remain unchanged.
- The timing-only 10-inch image `d7f300074` completes 1,000 cycles, 3,000 actual
  page changes and three API reconnects. All 21 retained-resource fields stay
  unchanged; exact configuration bytes/generation and all 79 settings/PIN/reset
  checks pass. Full resource snapshots occur only while stopped at Home.
  Each request waits for the renderer's page-change acknowledgment. This does
  not establish a reliable fixed 500 ms request interval: an earlier sender
  missed 11 of 300 transitions. The completed-transition run has a median
  acknowledgment latency of 522 ms, 95th percentile 544 ms and maximum 2,126 ms,
  including network and polling time. These counters do not prove pixel stability.
- The 4.3-inch reader bridge boots with its exact original native-v1 dashboard,
  all 79 settings/PIN/reset status and a complete v2 read projection. The initial
  nested candidate rejects saving that dashboard: its folders retain six and
  seven records behind a five-card grid. Both v1 and v2 host reproduction fail
  write admission before the fix. PageStore now retains all records while
  binding only the grid prefix after Back, including making descendants of an
  off-grid folder inactive. An explicitly overfull visible order is still
  rejected; stored inactive rows and requested grid positions are checked
  separately. Sanitized regression coverage passes on all nine
  profiles; physical qualification of this fix remains required. No dashboard
  replacement was committed on the 4.3-inch panel during the rejected checks.

## Remaining gates

All nine production factory profiles compile at `7c0abe97a`; its PR CI passes
all 52 required tasks. Compilation does not establish physical layout parity.
Final-build paired memory runs, the original-dashboard memory investigation,
nested visual confirmation, remaining P4 qualification, further replacement/
delayed-event/modal/takeover checks, image peaks and physical storage-failure
qualification remain open. Published-reader and mirror-retirement gates in
[the delivery plan](nested-pages-delivery.md) still apply before release enablement.
