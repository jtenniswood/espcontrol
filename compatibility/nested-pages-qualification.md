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

A mixed SDK2 S3 fixture has 56 climate controls, two image cards and seven
folders, with exactly 1,126 raw subscriptions and 96 KiB of climate values in
a 128 KiB cache reservation. With temporary sleep overrides tracked and later
restored, two PNG tiles load at 150×139 (41,700 bytes each), and 12 expanded-PNG
open/close cycles pass. The expanded image is 470×436 (409,840 bytes); its closed
cache remains resident. Scene reservations return to 239,547 bytes and seven
live allocations after each close, with no scene allocation failures.

A following 480×480 GIF download reaches 290,052 observed free PSRAM bytes,
below the diagnostic runner's 512 KiB stopping margin. The first original-data
restore is rejected with HTTP 503 and leaves the test document at generation
1047. A recovery observation then sees animation playing, 244,584 free PSRAM
bytes, a 126,976-byte largest PSRAM block and one failed scene reservation.
Normal display-off releases all observed image buffers/leases, returning free
PSRAM to 1,704,896 bytes. The exact original native-v1 save then succeeds at
generation 1048, and exact upgraded firmware, all 78 protected settings/PIN/reset,
original sleep values and automatic-update preference are verified restored.
These are sampled observations, not isolated image allocation peaks. The GIF
modal and delayed-download replacement checks were not reached. An earlier
attempt also remains excluded from modal qualification because the saved
sleep policy interrupted its first opening; its recovery at generation 1046
is preserved separately. The completed PNG checks do not qualify the GIF
pressure case or pixel stability.

Private SDK2 probe `d496ecdbd` adds owned GIF-source/workspace counters and
explicit IDF local-minimum windows. With the same mixed fixture, it records:

| Phase | Internal minimum free | PSRAM minimum free |
| --- | ---: | ---: |
| PNG modal open/close | 74,355 bytes | 926,636 bytes |
| 480×480 GIF tile download/decode | 74,243 bytes | 193,496 bytes |
| Original-dashboard save with GIF resident | 75,391 bytes | 103,280 bytes |

The GIF plays and owns 921,600 canvas-workspace bytes, a 369,953-byte compressed
source, a 17,500-byte player and 4,200 resampling-axis bytes, separately from
its visible pixel buffers. The save returns HTTP 204 in this run, with no failed
scene reservations; the earlier HTTP 503 does not reproduce. The first GIF
observation has a 172,032-byte largest PSRAM block, whereas the earlier run has
126,976 bytes. Source inspection confirms that preparation requests a full
131,072-byte document arena before compacting even a small dashboard. This is
an avoidable contiguous-allocation requirement; the earlier firmware did not
record the failed request's size, so its exact failure site is unproven.
The diagnostic restores exact upgraded firmware and original data at generation
1050, with all protected settings/PIN/reset/sleep/update preferences verified.
These windows exclude boot lows, but include diagnostic/network work and
aggregate per-region minima. They are not a production before/after comparison.

The follow-up runtime change starts with a smaller private parsing reservation
and grows only when needed, releasing unsuccessful attempts before growth and
compacting before resources borrow the model. The existing admission ceiling,
source bytes and allocation-free activation remain unchanged. Host checks pass
small v1/v2 replacements on all nine profiles with an 8 KiB maximum allocation,
larger-layout growth and failed-reservation rollback. All four grid-size scene
integrations pass. CI also passes at `43a914249` (run `41645151214`). Updated
factory builds subsequently pass on all nine profiles with ESPHome 2026.10.1;
the SDK2 S3 diagnostic also compiles and runs this implementation. Earlier
compile/device results below apply to their recorded source revisions.

The same SDK2 S3 protocol is repeated with private `3a7c4e0a1`, whose only
changes from `d496ecdbd` are the public `43a914249` staging implementation and
project label. Both runs use identical synthetic configuration/image fixtures,
diagnostics and phase boundaries. The original save returns HTTP 204 with no
failed scene reservations in both runs. During that phase, minimum free PSRAM
increases from 103,280 to 190,316 bytes (+87,036 bytes, about 85 KiB); the internal
minimum changes from 75,391 to 75,359 bytes. PNG/GIF phase PSRAM minima are
925,784/190,140 bytes afterward, versus 926,636/193,496 before. This is one pair
of diagnostic phase windows, not a new idle-memory or instantaneous combined
heap comparison. Exact upgraded firmware, original native-v1 data at generation
1052 and all protected settings/PIN/reset/sleep/update preferences are verified
restored. Other physical image/modal/delayed-transfer combinations remain open.

The same S3 stage probe also passes a disjoint replacement during an observed
delayed GIF request. The server cancels that 369,953-byte response after sending
57,344 bytes. A 409,840-byte parked PNG modal cache is present at write start
and absent afterward; the disjoint graph initially contains no old cached
values. Fresh observers receive exactly 1,126 registrations for each graph.
After new PNGs load, no old GIF source/workspace/player/axis allocations remain.
A timer takeover with an open modal releases every observed image buffer and
modal context; waking reloads the current PNGs with all 14 checked retained
fields equal (239,547 scene bytes, seven allocations, 81 widgets). Exact
upgraded firmware, original data at generation 1057 and protected settings,
PIN/reset, sleep values and update preference are verified restored. This
qualifies cooperative in-flight cancellation and that takeover/wake path;
completed-but-delayed callbacks and pixel content retain separate gates.
An earlier attempt stops before replacement because it assumes optional cache
retention is mandatory; that excluded attempt restores generation 1054. The
accepted run lets image ownership settle before closing and records the cache
actually present at write start.

Private SDK2 S3 storage probe `e84ef5c78` passes 29 cases using the production
two-slot store and transient NVS backend at the deployed 16 KiB scratch
capacity. Real IDF NVS uses a separate guarded scratch namespace with 2,048/3,072-
byte payloads spanning multiple I/O windows. The cases cover a normal commit,
injected cuts before and after all ten write/sync boundaries, seven initial
header/payload read failures and newest-payload checksum corruption. Fresh
handles/backends select complete old or new payloads after cuts, preserve the
old payload without mutations after initial read failures, and fall back to
the older valid slot after corruption. Each operation returns the single
16,384-byte scratch reservation; final scratch keys and live scratch bytes are
zero. Exact upgraded firmware, unchanged original data/generation 1057 and all
78 protected settings/PIN/reset/update preference are verified restored.
These are hardware tests with injected faults, not physical power interruptions
or raw-partition failure tests; clearing scratch keys is not a bitwise NVS
partition rollback.

Current SDK2 probe `105e108cf` also repeats the full-cache workload with a
true unpaced 1,440-message send loop. The first ordered DeviceInfo response
exceeds the client's default ten-second timeout. Its cleanup then encounters a
connection timeout and cannot reconcile the still-running candidate; an
independent identity/data check enables targeted recovery of exact original
firmware, native-v1 bytes at generation 1062 and every protected preference.
A separate diagnostic uses a sixty-second response observation window with
sleep disabled and Home timeout zero. That response also times out; the asleep
case is never reached. Its improved cleanup restores exact original firmware,
data at generation 1064 and all settings, including both sleep overrides.
Neither run supplies a post-burst cache/ownership snapshot or qualifies burst
handling. Their source of delay remains unisolated; they do not establish an
allocation failure or reboot. The earlier successful paced cache checks remain
scoped to their recorded input protocol.

A subsequent diagnostic compares individual writes with one grouped write of
the same 1,440 messages. Both ordered responses remain pending after 30 seconds.
Local macOS TCP counters show an established connection with about 106 KiB
still queued for sending and an advertised receive window below 1 KiB near the
end of each observation. Grouping does not resolve the slow drain; this does
not yet attribute it to the firmware receive loop, rendering or transport.
No packet payloads are captured. Exact original SDK2 firmware, native-v1 bytes
at generation 1067, all 78 settings/PIN/reset/update preferences and temporary
sleep overrides are verified restored.

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
  limiting active membership after Back, including making descendants of an
  off-grid folder inactive. An explicitly overfull visible order is still
  rejected; stored inactive rows and requested grid positions are checked
  separately. Sanitized regression coverage passes on all nine
  profiles. The physical recheck below qualifies the corrected save path. No dashboard
  replacement was committed on the 4.3-inch panel during the rejected checks.
- The corrected 4.3-inch SDK2 probe `15b2d4377` (base `c28c5b43f`) subsequently
  boots with the exact original dashboard and passes the complete v2 read
  projection. A native-v1 save that retains every original field and adds only
  a qualification setting commits successfully; restoring the exact original
  bytes also succeeds. Scene reservations return to 22,199 bytes with 54 widgets
  and no allocation/accounting failures. Exact upgraded firmware, original
  native-v1 bytes at generation 133, 78 protected settings/PIN/reset and original
  automatic-update preferences are verified restored. This qualifies the
  retained-record save fix on hardware, without a nested write or pixel claim.
- A long-identifier 10-inch flat climate fixture occupies 25,087 native-v1
  bytes but 53,407 bytes in its v2 projection. Its native-v1 commit returns HTTP
  202 with `X-Panel-Config-Legacy-Mirror: failed`; it is not credited as a
  completed full-cache test. The exact upgraded firmware, original data at
  generation 349 and protected preferences are verified restored. Both update
  switches are observed ON; a delayed switch acknowledgment is reconciled
  without repeating the device write. This illustrates separate native-storage,
  compatibility-mirror and format-conversion constraints.
- A compact 10-inch fixture keeps the same 400 cards (20 folders and 380 climate
  controls) and 7,600 state/attribute pairs, while shortening synthetic IDs and
  labels. Its native-v1/v2 sizes are 9,443/29,713 bytes. Four physical SDK2
  same/disjoint replacements commit with HTTP 204, fill the complete 524,288-byte
  cache and complete two folder round trips each. Fresh observers receive
  exactly 7,600 raw registrations per replacement, with no duplicates, missing
  or unexpected pairs. All 18 checked retained-resource fields remain equal:
  scene reservations are 1,171,219 bytes with seven live allocations and 180
  widgets. An API reconnect, fresh exact registration audit and full-cache
  refill also pass. The final snapshot has 341,671 free internal bytes and
  17,582,652 free PSRAM bytes; no scene allocation/accounting failures are
  reported. Exact upgraded firmware, original native-v1 bytes at generation
  354, all 78 protected settings/PIN/reset and both original automatic-update
  preferences are verified restored. These checks use ordered input batches
  and do not establish burst throughput, pixel stability or isolated
  image/decode peaks. The larger fixture's compatibility warning remains an
  independent gate.

P4 snapshots also contain one live backlight observation, for 79 entries in
total. Wake/sleep and automatic brightness can change that output; configured
brightness controls remain among the 78 protected settings.

After these tests, the separately authorized SDK-update task installed ESPHome
2026.10.2 on the local displays and verified the saved dashboard generations
unchanged. It corrected the replacement 7-inch panel to the V2 profile and
verified its dashboard byte-for-byte. Further qualification must preserve those
newer images as recovery targets. The paired idle-memory measurements remain a
2026.10.1 comparison; the separate SDK2 diagnostics retain their own scopes.
The native API subscription bridge has been audited against both
patches; matching API sources are byte-identical and guard tests accept only
those two audited versions.
The user subsequently makes the replacement 7-inch V2 available through the
coordinated SDK-update task. Current SDK2 probe `fff8da163` uses the public
`43a914249` runtime. It passes the complete v2 read projection of the original
dashboard and 52 acknowledged Home/Devices transitions, with full resource
snapshots only while stopped at Home. Retained owners remain equal, allowing
the live HA value cache to grow within its budget. The user confirms cards,
labels, icons, colours and Back are correct and stable without flashes or
jitter. Exact upgraded firmware, unchanged original native-v1 data/generation
2, all 78 protected settings/PIN/reset and both original automatic-update
preferences are verified restored. This is original first-level navigation
and visual coverage; it does not establish nested writes or maximum-load
pixel stability.

The same 7-inch V2 probe then passes four same/disjoint native-v1 replacements
with 225 cards (15 folders and 210 climate controls), 4,200 exact state/attribute
registrations, and 393,216 bytes of values in a 524,288-byte reservation. Each
phase passes two folder round trips. All 18 retained fields remain equal:
953,413 scene bytes, seven allocations and 135 widgets. A fresh API reconnect,
exact raw-registration audit and cache refill also pass, with no scene
allocation/accounting failures. The final workload snapshot has 353,655 free
internal bytes and 21,334,348 free PSRAM bytes. Exact upgraded firmware,
original native-v1 bytes at generation 7, all protected settings/PIN/reset and
both original update preferences are verified restored. Inputs use ordered
barriers every 64 reports. This is a populated 384 KiB cache within a 512 KiB
reservation, not a full-value-capacity or burst-throughput claim; the separate
original-dashboard visual confirmation does not establish maximum-load pixels.

## Review corrections after device qualification

Runtime correction `39a159354` uses explicit runtime membership when a page holds
extra saved records. An order such as `B,15` can activate card 15 while retaining
other rows without subscriptions. Shared iteration and membership also cover
local producers, inherited state, modal styles, commands, descendant targets
and restoration of the current navigation path. V1/v2 host regressions on all
nine profiles verify the higher-numbered Sensor's rendered cell and exact
subscription, hidden-row exclusion and activation of a reordered folder's
subtree. The four store/dependency checks and all four complete grid-size scene
integrations pass. This correction follows the recorded physical checkpoint;
those measurements do not qualify the later membership change on hardware.
A new sequential all-nine factory matrix passes at `39a159354` with ESPHome
2026.10.1. Its firmware/configuration sources remain unchanged at the later
browser-only follow-up.

Browser correction `d540f39fc` keeps complete native snapshots authoritative over late
legacy chunks, refreshes the native snapshot after configuration events, allows
read-only folder/Back navigation while preserving write protection, and captures
the current orientation for each card transfer. A follow-up distinguishes a complete editor snapshot from the cache of a
successful save on older depth-one native firmware, whose legacy events must
remain enabled. Configuration events arriving during a read queue one fresh
follow-up; disconnect cancels it. Type checking, 254 browser-unit
checks, generated-bundle freshness and focused real-browser nested journeys on
all four grid sizes pass. The full browser run passes on all nine generated layouts after the
compatibility follow-up. It includes older native/legacy loading, current graph
editing, read-only navigation with disabled edits, backups and reconnects.
Earlier excluded runs exposed an obsolete read-only lock expectation and the
need to distinguish older native-save caches from complete editor snapshots.

A further review correction announces the durable configuration generation
through the event stream independently of compatibility text entities. This
covers nested-only edits from another browser. Busy streams retry and coalesce
to the latest generation. An editor blocked by a draft, drag or focused title
keeps a deferred refresh and rereads once it can apply safely; disconnect and
local replacement cancel it. Focused browser journeys on all four grid sizes
verify nested-only updates and title-draft cancellation without a write.
The event-stream lifecycle tests also cover notification delivery and retry
under backpressure. A stale host-test assertion is updated for the already
introduced 20-byte page record with its sparse-membership mask.

Private SDK2 S3 probe `105e108cf` then qualifies the membership correction on
hardware with a sparse `B,9` page containing eight retained Sensor rows and a
visible ninth climate control. Fresh observers receive exactly 20 climate
registrations and none for retained rows. Both disjoint native-v1 replacements
bind card 9 correctly and open its modal. The second save occurs with the old
climate modal open; it retires that context. Replaying the held old binding/modal
origin is rejected both before and during the new modal, which remains intact.
All 14 checked retained-resource fields match after closing and returning Home:
23,067 scene bytes, seven allocations and 81 widgets, with no scene allocation
failures.
This is synthetic-origin replay and modal/binding coverage, not pixel proof or
a delayed completed-download callback test. Exact upgraded firmware, original
native-v1 bytes at generation 1060, all 78 protected settings/PIN/reset and the
original update preference are verified restored.

Main `738840a02` brings in ESPHome 2026.10.2. Integration
`c05fde64f` changes only the SDK pin, preserves parent runtime ancestry and
passes all 13 wrapper tests; firmware/browser sources remain byte-identical to
the tested corrections. A new sequential factory matrix passes all nine profiles on this
current pin, with completed log hashes, SDK markers and unchanged sources
verified. The preceding all-nine SDK1 matrix remains a separate checkpoint.

The independent generation notification and deferred-refresh correction at
`7560bf91a` passes 256 web-unit checks, type checking, generated-bundle freshness
and the complete real-browser suite on all nine layouts. Six focused host checks
and both SDK variants of the event-stream lifecycle test pass. Linux CI passes
243 of 244 firmware tests; the remaining HTTP test still expects every retained
row to be active despite an order of `B,1`. Its follow-up checks exactly one
active row and preservation/exclusion of the others. The complete sanitized
HTTP v1/v2 suite passes with those corrected assertions. The full local firmware
build is not credited: macOS Clang stops on an existing unused constant in the
timer fixture.

A verified SDK2 S3 display-mode diagnostic uses the same grouped 1,440-message,
128 KiB value workload. With display-off verified before and after delivery,
the response arrives in 11.634 seconds and the full cache is observed. With
display-active verified before delivery, no ordered response arrives within
30 seconds; local TCP counters still show 108,659 outgoing bytes near the end.
This narrows the investigation to a contribution from active-display work;
it does not identify the specific work or qualify unpaced burst handling.
Exact original firmware, native-v1 bytes at generation 1073 and all protected
settings/PIN/reset/sleep/update preferences are verified restored. A preceding
attempt completes the asleep condition but fails to wake through Home navigation;
it is excluded as a pair and restores generation 1070. Fixed timing-only
instrumentation is prepared separately; no performance fix is claimed.

All nine factory profiles compile at `7560bf91a` with ESPHome 2026.10.2.
Completed logs, hashes and SDK markers are verified. The assertion-only follow-up
and this record do not change those tested firmware or browser sources.

A later instrumented awake S3 observation records 590 state callbacks in a
30.062-second window: callback scopes total 0.128 seconds, while 61 application
loop scopes total 1.767 seconds, including 522 card paints totalling 1.686 seconds.
These scopes measure elapsed time, overlap and omit other framework/display
work; they cannot be summed into a CPU attribution. The ordered response still
times out at 30 seconds. Exact original firmware/data/settings are restored at
generation 1079. Two earlier timing setups send no burst: one fails to enter
sleep within its setup window (recovery 1075), and another opens the observer
before closing its audit client, exceeding three API connections (recovery 1077).
The accepted timing setup opens the observer only after audit disconnect.

A host reproduction feeds the exact 1,440-channel fixture in 144 batches and
counts 32,400 LVGL invalidation events on unchanged Home cards. A runtime
follow-up checks visible cards' declared state channels before painting; folder
indicators also check their descendant sources. It reuses existing channel
revision storage and adds no per-card allocations. The reproduction then counts
zero invalidations with the same 301,420 retained host scene bytes. Cache tests
cover unrelated/shared channels, reconnects and revision wrap boundaries; full
scene tests pass all four grid sizes, including visible-update and child-indicator
positive controls. The new hidden-update regression fails against the isolated
old renderer. Implementation `ede961755` passes CI and all nine sequential factory
builds on ESPHome 2026.10.2; log hashes, SDK markers and frozen sources are verified.
Browser sources are unchanged from the completed nine-layout run.

The controlled S3 after-probe `c632284c7` changes only the three runtime files
and its label from timing baseline `7feec25d9`. The same observer/feed protocol
now completes all 1,440 reports in 11.610 seconds with no card paints and a full
131,072-byte cache. Application-loop scopes total 0.165 seconds, including
0.141 seconds in scene scopes; callback scopes total 0.283 seconds. The scopes
overlap and omit other framework work. Instrumented static RAM remains 204,879
bytes and code grows 268 bytes; these are not settled heap measurements. Exact
original firmware/data/settings are verified restored at generation 1081.

A separate ordinary-client replay omits the timing observer and counter
activation, retaining the library's default ten-second response limit. Its
first ordered response times out, so no burst/phase completes; cleanup verifies
exact original firmware/data/settings at generation 1083. This rules out the
extra observer as the sole cause. The redraw correction is physically observed,
while unpaced-burst qualification and attribution of the remaining delay remain
open. No longer timeout is credited as a performance pass.

A further timing-only probe `c4c62a2f0` keeps the same runtime and adds
within-component callback-gap and public LVGL timer scopes. Both hooks are
exercised: 1,296 gaps between successive callbacks in the same API component
iteration total 8.555 seconds; 146 LVGL timer calls total 0.070 seconds. The
configured core loop interval is 16 ms. All 1,440 callbacks complete with a full
cache and no card paints; the ordered response arrives in 11.585 seconds.
Between-callback gaps include SDK dispatch/matching, framing and socket reads;
they are not an isolated measurement of subscription lookup or CPU execution.
The pinned SDK scans its subscription list for each incoming state and recomputes
C-string lengths during comparison, making that path the next investigation
target. No SDK/framework optimization has been implemented. Exact original
firmware/data/settings are verified restored at generation 1085.

Browser-only follow-up `a955299e9` also passes required CI, all 257 web-unit
checks, type/bundle checks and the complete nine-layout browser suite. Its
flat native root order/accent tests hold a snapshot response while stale legacy
events arrive, then verify independent setting refreshes without writes. C++
firmware, device configuration and SDK sources remain at the previously tested
`ede961755` checkpoint; the regenerated web assets are checked separately.

A private shared-entity-name experiment `9ee372c9f` keeps SDK, artwork and timing
hooks unchanged from `c4c62a2f0`. Its grouped 1,440-report diagnostic completes
in 9.948 seconds, with 6.323 seconds between callbacks. Unlike the baseline,
it records 18 paints, two finishes and 0.891 seconds in LVGL timer scopes.
This single observation does not establish an isolated causal effect or total
heap improvement. Exact original recovery completes at generation 1087.
The ordinary client then completes two full 128 KiB bursts in 9.107 and 8.969
seconds, but the third, disjoint-entity burst exceeds the unchanged ten-second
limit. The qualification fails. Exact original firmware/native bytes and all
78 settings, PIN/reset and sleep/update preferences are restored at generation
1091. No experimental registry change is promoted to production.

Allocator-tracked host checks include every registry name, pool node and
container allocation. The first sharing prototype saves 36,920 bytes in the
72-entity climate fixture but adds 60,088 bytes for unique entities. A host-only
pre-install batch-sharing refinement saves 36,848 bytes for the shared fixture
with zero retained-byte difference for unique/empty-attribute fixtures. These
are 64-bit host registry counts, excluding SDK storage and allocator headers,
not device heap results. Ownership and actual transport checks pass ASan/UBSan,
including installed-pointer stability, retirement of a shared anchor and 200
replacement batches. The refinement has no firmware or physical qualification.
API burst handling remains open.

A forwarding-only SDK handler probe `54ecaf644` narrows that attribution.
The built handler, header, generated dispatcher and StringRef match the pinned
SDK sources exactly. The timer invokes the unchanged original method and
records exactly 1,440 handler calls and coordinator callbacks for each of two
disjoint full-cache workloads. Handler scopes total 9.592 and 9.583 seconds;
their nested coordinator scopes total 0.309 and 0.297 seconds. Ordered responses
take 12.064 and 11.620 seconds. These elapsed scopes include matching, registry
dispatch and preemption; they are not isolated CPU/string-comparison times.
Exact original firmware/native bytes and all protected settings are verified
restored at generation 1094.

A private borrowed-row lookup prototype preserves exact matching and native
callback order in differential host tests, including forced hash collisions,
foreign/once registrations, null/empty attributes, stale indexes and reentry.
ASan/UBSan checks pass; its 7,600-entry table uses 60,800 bytes and queries
allocate nothing. At that checkpoint it had no native SDK integration,
owning-buffer lifecycle, firmware build or physical optimization qualification.

The subsequent private integration `6b8add896` passes fallible ownership,
retry/rollover, exact-capacity, source-identity and teardown tests. Tests compile
the actual wrapper/transport delivery code, checking stock fallback, foreign
callbacks, state termination, net-zero replacements, interrupted delivery and
zero per-frame allocation under ASan/UBSan. The integrated lookup uses 11,624
bytes for 1,453 native registrations, including 13 foreign registrations.
Its instrumented ELF adds 200 bytes of DRAM data/BSS/noinit and 3,120 OTA bytes
against the stock-handler probe; these are not settled heap measurements.

The two matching controlled S3 workloads complete in 2.531 and 2.518 seconds,
against 12.064 and 11.620 seconds with the stock handler. Indexed delivery scopes
total 0.375 and 0.379 seconds. Each stream records 1,440 fast-path deliveries
and callbacks, zero fallback/interruption and the full 128 KiB cache. The
ordinary client, without timing-counter activation or a timing observer, then
passes all four same/disjoint replacements and reconnect/refill within the
unchanged ten-second limit: 2.836, 2.277, 2.390, 2.226 and 3.006 seconds. It also
passes folder round trips and unchanged retained-owner/table checks. Exact
original firmware/native bytes and all protected settings are restored at
generation 1102. The S3 burst gate passes for this private source.

The production port retains the same generic lookup and buffer owner, removing
private timing/services and retaining version/signature guards. Six focused
production-source checks pass, including the existing ownership bridge and
expanded SDK guard; the actual production wrapper/transport also passes
ASan/UBSan. Production checkpoint `d3cb407ae` passes required CI and all nine
ESPHome 2026.10.2 factory builds. Log hashes, actual compiled lookup sources,
SDK version, MCU and both original/wrapper ELF symbols are verified for every
profile. Production-source physical qualification remains separate from the
private-image results above.

Public component checkpoint `4783dc76c` then passes the same S3 cohort using
private overlay `08b8ca0fc`, which changes only the S3 development YAML to add
on-demand owner/member/index reports. All compiled project component sources
and the unchanged SDK API sources are hash-verified. There are no private timing
hooks, image-memory instrumentation or production writer-policy changes.
All five ordinary ten-second bursts pass: 2.644, 2.357, 2.343, 2.424 and
2.472 seconds. Each delivers exactly 1,440 fast frames with zero stock fallback
or interruption; the 11,624-byte table remains unchanged within each feed.
Four same/disjoint replacements fill the full 128 KiB value cache, exercise two
folder round trips each, and preserve every checked retained scene owner.
The tracked scene arena remains 291,000 bytes across those replacements;
the native lookup table is counted separately. These are cohort ownership
measurements, not an uninstrumented original-dashboard heap comparison.
The exact original SDK2 firmware/native bytes and all protected settings,
PIN/reset and temporary sleep/update preferences are verified restored at
generation 1107.

The same public component checkpoint passes the actual 4.3-inch P4 RISC-V
lookup cohort through YAML-only overlay `e00e28f70`. All declared external
component sources match the public checkpoint; the API and built-in MIPI DSI
driver match the pinned SDK. Four same/disjoint replacements and reconnect
deliver exactly 600 fast frames per stream with no stock fallback/interruption
during each feed: 0.657, 0.816, 0.679, 0.683 and 0.711 seconds. Each replacement
checks two folder round trips and unchanged retained owners. The table uses
4,920 bytes for 615 native registrations, including 15 foreign registrations;
the tracked scene arena remains 138,796 bytes. This cohort fills 48 KiB of
values in a 64 KiB elastic allocation, not the profile's full 512 KiB ceiling.
An initial host attempt incorrectly expected the full ceiling to be reserved;
the corrected exact-fixture host check passes and the failed expectation is
excluded. Host-client timezone notices from separate event loops do not qualify
timezone behaviour in this synthetic climate cohort. Exact original SDK2
firmware/native bytes, protected settings, PIN/reset and update preferences
are verified restored at generation 138. This establishes the P4 native lookup
path for this cohort; broader P4 workload and pixel gates retain their scope.

A browser follow-up separates saved card identities from visible cell count
in both transfer validation and preview rendering. Five model regressions cover
retained cards above all four grid capacities and the saved-ID boundary 32;
inactive siblings, Back appearance and complete subtrees stay intact. Browser
journeys exercise actual copy, duplicate, cut and delete menus on all four grids.
All 262 web-unit checks, types and generated-bundle freshness pass. The complete
nine-layout browser suite also passes at `4783dc76c`, as does required CI.
C++ sources are unchanged from the completed `d3cb407ae` factory matrix;
this follow-up's rebuilt web assets have separate browser qualification.

## Remaining gates

All nine production factory profiles compile at `c05fde64f` with the current
ESPHome 2026.10.2 pin. The `39a159354` and `43a914249` checkpoints also pass
all nine profiles with ESPHome 2026.10.1. Historical matrices also pass at `ac0fad1ff` with ESPHome
2026.10.1 and at private `c61cda45b` with ESPHome 2026.10.2 (base `c28c5b43f`
plus the SDK pin). The rebase onto main `6c10c3d5a` preserves the parent runtime
branch and every tested firmware/browser source; its three changed files only
add main's host flashing guard and guidance. CI also passes at `36b5b9cd0` and
`c28c5b43f`, including the audited SDK guard and the checks selected for each change.
Compilation does not establish physical layout parity or qualify the full
feature on ESPHome 2026.10.2. The original-dashboard memory investigation,
nested visual confirmation, remaining P4 qualification, further replacement/
delayed-event/modal/takeover checks, additional P4 workload coverage, burst replay,
remaining image/decode combinations and physical storage-failure
qualification remain open. Published-reader and mirror-retirement gates in
[the delivery plan](nested-pages-delivery.md) still apply before release enablement.
