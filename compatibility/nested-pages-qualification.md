# Nested-page qualification

Recorded on 3 November 2026 for PR #2424, following runtime PR #2400.
Production still writes native v1 at depth one. Private qualification images
enable native v2/depth two without declaring a published reader floor.

## S3 memory comparison

The before image rebuilds the starting development source `36f482d7c`; it is
not a bitwise readback of the previously running flash. The final admission-guard
after image is private source `d8e850ebc`, based on `ac0fad1ff`, labelled
`lab-nested-ac0fad1ff`. Both use ESPHome 2026.10.1 and ESP-IDF 5.5.6 on the same
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
| Original dashboard | 76.1 → 79.6 KiB | 2276.5 → 2106.9 KiB | −166.0 KiB |
| Empty | 89.2 → 81.4 KiB | 2349.1 → 2321.3 KiB | −35.8 KiB |
| Nine sensors | 85.9 → 80.7 KiB | 2435.5 → 2399.9 KiB | −40.9 KiB |
| 72 sensors in nine folders | 43.2 → 73.9 KiB | 2217.4 → 2347.9 KiB | +161.2 KiB |

The original dashboard includes a cover-art media card. Its combined free range
is 2,409,011–2,409,039 bytes before and 2,238,487–2,239,211 after. The protocol
does not directly observe image buffers or display mode; that regression is
not yet isolated. Real HA content can update, and fixed-order replacements can
retain caches. Subtracting rows therefore does not isolate per-card costs.
These are settled measurements, not navigation or image/decode peaks.
The maximum Sensor layout also improves the largest internal block from
24,576 to 31,744 bytes. Its largest PSRAM block improves from 2,228,224 to
2,293,760 bytes. These figures apply to that workload; the original dashboard's
largest PSRAM block instead falls from 2,293,760 to 1,835,008 bytes.

Earlier synthetic fixtures put Sensor IDs in the action field rather than the
Sensor field, so their nine-sensor and maximum-layout claims are withdrawn.
The original raw samples and failed proofs remain preserved. An observer issue
in a later attempt restarted the subscription stream before its last entry;
that attempt is also excluded. The accepted pair waits for delayed grid refresh
and drains the complete stream before retrying. Both cohorts restore exact
original dashboard data and verify all 78 settings/PIN status. Final recovery
also verifies the original-source firmware, exact native-v1 bytes at generation
1037, reset status and the original automatic-update setting restored.

This checkpoint includes newer main changes and larger RGB bounce buffers, so
it is an end-to-end build comparison. Static RAM falls from 206,455 to 204,503
bytes. OTA image size grows from 5,019,968 to 5,260,656 bytes. ELF code and
read-only sections grow by 240,812 bytes. Both builds enable instruction and
read-only-data copies into PSRAM, so code growth offsets runtime savings; the
S3 also spends 19,200 more internal bytes on its 30-row RGB bounce buffers.
Section sizes are separate accounting, not exact live PSRAM reservations. See
the [pinned IDF explanation](https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32s3/api-guides/external-ram.html#move-instructions-in-flash-to-psram).

A separate host allocation census uses the same corrected payloads and the
real LVGL scene, with mocked hardware, HA transport and image decoding. On that
64-bit host, the original dashboard reserves 38,968 bytes in the scene arena
versus 13,981 empty and retains one unloaded artwork lease. This is not an ESP32
heap measurement and does not isolate the media regression. No HA values were
supplied in either that census or the settled Sensor fixtures; physical tests
with populated state caches and real image buffers remain required.

A later ESPHome 2026.10.2 attribution check uses private baseline `99b63a37b` (base
`1963f016a`) and candidate `48c32de02` (base `bb94956b0`), both reading the exact
original native-v1 dashboard. Three on-demand snapshots follow a 130-second
settle. Both load a 310×288 tile with 178,560 pixel-buffer bytes; their other
image pixel/download/retired buffers are empty. The candidate's scene arena
reserves 34,172 bytes, its populated state cache uses 800 of 4,096 reserved
bytes, and no scene allocation failures occur. Combined free medians are
2,216,675 baseline and 2,241,619 candidate bytes, a 24.4 KiB improvement.
Diagnostic logging, SDK, input format and uncontrolled live content differ
from the earlier cohort. This does not replace that comparison or establish
the cause of its regression; historical image-buffer observations are absent.
Both probes restore the exact upgraded firmware, original dashboard/generation,
78 settings, PIN/reset status and original automatic-update preference.

Host coverage also fills the complete S3 128 KiB HA cache and P4 512 KiB cache
through same/disjoint climate replacements, rejects stale-generation callbacks
and navigates with new allocations disabled. The exact prepared S3 flat-v1
climate fixtures independently pass with 1,440 channels and a full 131,072-byte
cache across four replacements. These host proofs do not measure physical heap
or pixel stability.

The ESPHome 2026.10.2 S3 probe subsequently passes four physical flat-v1 climate replacements
(same, then disjoint entities), each with 1,440 verified registrations and the
complete 131,072-byte cache. Each replacement completes two folder round trips;
an API reconnect and refill also pass. The scene arena remains at 290,960 bytes
with seven live allocations, 81 borrowed widgets and no reported scene allocation
failures. Internal heap remains about 78 KiB; the final snapshot has 1,651,444
free PSRAM bytes and a 786,432-byte largest PSRAM block. Inputs use an ordered
protocol response barrier after every 16 reports. An earlier unsynchronized
burst times out awaiting the post-fill diagnostic response and is not credited;
burst replay performance remains unqualified. The exact upgraded firmware,
original native-v1 bytes at generation 1044, all 78 settings/PIN/reset and the
original automatic-update preference are verified restored. These checks do
not establish pixel stability, image/decode peaks or physical P4 cache behavior.

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
  Original dashboard data, 78 protected settings and PIN/reset status are retained;
  the exact cached original firmware and native-v1 dashboard are now restored,
  with all 78 protected settings/PIN/reset and both original automatic-update settings
  verified. Other P4 tests remain in progress.
- The actual 10-inch embedded editor also passes creation inside an existing
  folder, a Sensor leaf bound through its Sensor field, reload, immediate-parent
  Back and the two-level creation limit. Both saves are atomic; root records,
  native settings, a sibling folder and stored inactive page remain unchanged.
- The timing-only 10-inch image `d7f300074` completes 1,000 cycles, 3,000 actual
  page changes and three API reconnects. All 21 retained-resource fields stay
  unchanged; exact configuration bytes/generation and all 78 protected settings/PIN/reset
  checks pass. Full resource snapshots occur only while stopped at Home.
  Each request waits for the renderer's page-change acknowledgment. This does
  not establish a reliable fixed 500 ms request interval: an earlier sender
  missed 11 of 300 transitions. The completed-transition run has a median
  acknowledgment latency of 522 ms, 95th percentile 544 ms and maximum 2,126 ms,
  including network and polling time. These counters do not prove pixel stability.
- The 4.3-inch reader bridge boots with its exact original native-v1 dashboard,
  all 78 protected settings/PIN/reset status and a complete v2 read projection. The initial
  nested candidate rejects saving that dashboard: its folders retain six and
  seven records behind a five-card grid. Both v1 and v2 host reproduction fail
  write admission before the fix. PageStore now retains all records while
  binding only the grid prefix after Back, including making descendants of an
  off-grid folder inactive. An explicitly overfull visible order is still
  rejected; stored inactive rows and requested grid positions are checked
  separately. Sanitized regression coverage passes on all nine
  profiles; physical qualification of this fix remains required. No dashboard
  replacement was committed on the 4.3-inch panel during the rejected checks.

P4 snapshots also contain one live backlight observation, for 79 entries in
total. Wake/sleep and automatic brightness can change that output; configured
brightness controls remain among the 78 protected settings.

After these tests, the separately authorized SDK-update task installed ESPHome
2026.10.2 on the local displays and verified the saved dashboard generations
unchanged. It corrected the replacement 7-inch panel to the V2 profile and
verified its dashboard byte-for-byte. Further qualification must preserve those
newer images as recovery targets. The measurements above remain a 2026.10.1
comparison. The native API subscription bridge has been audited against both
patches; matching API sources are byte-identical and guard tests accept only
those two audited versions.

## Remaining gates

All nine production factory profiles compile at `ac0fad1ff` with ESPHome
2026.10.1. CI also passes at `36b5b9cd0`, including the audited SDK guard.
Compilation does not establish physical layout parity or qualify the full
feature on ESPHome 2026.10.2. The original-dashboard memory investigation,
nested visual confirmation, remaining P4 qualification, further replacement/
delayed-event/modal/takeover checks, populated P4 HA state caches, burst replay,
image peaks and physical storage-failure
qualification remain open. Published-reader and mirror-retirement gates in
[the delivery plan](nested-pages-delivery.md) still apply before release enablement.
