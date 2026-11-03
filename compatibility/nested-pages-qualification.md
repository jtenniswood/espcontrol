# Nested-page qualification

Recorded on 3 November 2026 for PR #2424, following runtime PR #2400.
Production still writes native v1 at depth one. Private qualification images
enable native v2/depth two without declaring a published reader floor.

## S3 memory comparison

The before image rebuilds the starting development source `36f482d7c`; it is
not a bitwise readback of the previously running flash. The uninstrumented
after image is private source `d64c2829e`, based on `7c0abe97a`, labelled
`lab-nested-05bf5c0f8`. Both use ESPHome 2026.10.1 and ESP-IDF 5.5.6 on the same
physical 4-inch S3, with its saved settings and plaintext API.

The initial runs apply the same decoded flat workloads in the same order. Each waits
130 seconds, then takes three existing standard memory-sensor publications
61 seconds apart. No extra diagnostic RPC, browser session or synthetic HA
feed runs during sampling. Values below are medians. Total means combined free
internal heap plus free PSRAM; it does not mean firmware image size.

The synthetic nine-sensor and maximum-flat fixtures were found to put their
sensor IDs in the action-entity field instead of the Sensor field. A subsequent
physical dependency check reported zero registered channels. Their measurements
do not qualify sensor workloads; corrected paired runs are required. Only the
valid original-dashboard and empty rows from that initial cohort are shown here.

A fresh corrected before cohort has since completed all four workloads, with
three settled samples each. Reverse HA registration checks verify all nine and
72 expected Sensor states before sampling. The observer waits for delayed grid
refresh and drains the complete registration stream before retrying; an earlier
attempt restarted that stream too soon and is excluded. The paired after run
uses updated private source `8de157565`, based on the off-grid fix `95037c8c3`,
and remains in progress. No completed comparison from this new pair is claimed.

| Workload | Internal heap free, before → after | PSRAM free, before → after | Combined free change |
| --- | ---: | ---: | ---: |
| Original dashboard | 75.3 → 79.9 KiB | 2276.3 → 2284.4 KiB | +12.7 KiB |
| Empty | 88.7 → 81.4 KiB | 2349.0 → 2322.9 KiB | −33.4 KiB |

The original-dashboard after samples span 2,421,007–2,429,347 combined free
bytes; before samples span 2,407,891–2,408,067. The empty workload has identical
samples within each run. Largest free blocks remain 31,744 internal bytes and
2,293,760 PSRAM bytes in every sample. These are settled idle measurements
with the original settings, not a direct observation of display mode or a
navigation/image-decode peak. Real HA values may update, and replacements can
retain caches; subtracting rows does not isolate per-card allocation costs.

The full candidate includes newer main changes and larger RGB bounce buffers,
so this is an end-to-end build comparison. Static RAM falls from 206,455 to
204,503 bytes. OTA image size grows from 5,019,968 to 5,260,560 bytes. ELF code
and read-only sections grow by 240,716 bytes. Both configurations enable
instruction/read-only-data copies into PSRAM, so code growth offsets runtime
savings; the S3 also spends 19,200 more internal bytes on its 30-row RGB bounce
buffers. Section sizes are separate accounting, not an exact measurement of
live PSRAM reservations. See the
[pinned IDF explanation](https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32s3/api-guides/external-ram.html#move-instructions-in-flash-to-psram).

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
- After both memory cohorts, the original-source firmware, exact original
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
  original-firmware recovery and the other P4 tests remain in progress.
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
Corrected paired sensor memory runs, nested visual confirmation, remaining P4 qualification, further replacement/
delayed-event/modal/takeover checks, image peaks and physical storage-failure
qualification remain open. Published-reader and mirror-retirement gates in
[the delivery plan](nested-pages-delivery.md) still apply before release enablement.
