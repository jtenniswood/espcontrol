# Nested-page qualification

Recorded through 6 November 2026 for PR #2424, following runtime PR #2400.
Production still writes native v1 at depth one. Private qualification images
enable native v2/depth two without declaring a published reader floor.

## Rebased validation and power-test readiness (6 November)

Runtime PR #2400 now includes main `6a94fceed` as `14cb34df8`, preserving the
earlier shared PR stack. PR #2424 is rebased onto it at `a1b0ae01e`. Its source
tree is byte-identical to the validated `1c8747ff4` tree. Both published heads
pass GitHub CI. The feature passes all 51 local CI tasks outside the separately
run firmware host task, plus 253 core firmware host tests under Linux/GCC.
The runtime dependency separately passes 240 core host tests.

All nine supported factory profiles compile with ESPHome 2026.10.2. Each build
passes SDK and ELF-wrapper checks; the eleven affected weather/runtime source
files independently match in all nine actual build directories. Focused browser
checks cover weather editors and nested creation/reload, immediate-parent Back,
subtree operations, retries and durable-byte rejection. Real-LVGL weather tests
pass 1,000 navigation cycles on each profile, and the all-profile nested-folder
test passes. Four affected runtime modal tests also pass. The focused Linux
LVGL builds disable the existing compact-line indentation warning; earlier
failed or stopped broad attempts remain separate records.

These are build, host and browser results. No display is flashed during the
rebase, and earlier physical appearance and memory results retain their recorded
source scope.

The prepared encrypted power qualifier `66f2315df` still has byte-identical
production storage code: `configuration_store.cpp/.h`,
`panel_config_storage_backend.h`, `panel_config_transient_storage.h` and
`panel_config_espidf_storage.cpp/.h`. The two private raw-partition/session
cores also match their RAM-loss host qualification hashes. This establishes
storage-source equivalence, not whole-firmware or renderer equivalence.
A new read-only preflight verifies the 10-inch V3 on official `v2.13.0`, its
exact original generation-375 dashboard, protected settings, PIN/reset state
and encrypted API identity. No API key is read or exported and no test is armed.
At this readiness checkpoint, actual power cuts still awaited an operator.
The user subsequently requested testing without disconnecting power; the
automated crash-recovery qualification below replaces that manual workflow.

The latest published stable release remains
[v2.13.0](https://github.com/jtenniswood/espcontrol/releases/tag/v2.13.0).
Its release contract declares native document version one; this branch still
has no published minimum reader release and remains in dual-write mode.
The rebase therefore does not establish the published depth-two reader floor
or retire the legacy mirror required for production nesting.

## Latest S3 results (5 November)

Public `d183c6898` uses a 6 MHz RGB clock, 30 bounce-buffer rows and conditional
VSYNC recovery. Its normal development and affected S3 factory builds pass
source, SDK, partition, native-handler and actual ELF-identity checks; GitHub CI
also passes. The uninstrumented, component-identical `5fab98845` visual control
loads the actual 240 × 240 expanded GIF at the maximum layout/cache before a
window without device polling. The user confirms it is stable and correctly
positioned as its colours change, without jitter, breakup or flicker. The lower
clock reduces observed refresh from approximately 37.7 to 22.6 Hz.

A matched original-dashboard comparison uses retained `1963f016a` and public
`d183c6898`, both ESPHome 2026.10.2, the same 1,151-byte v1 document, explicit
awake/Home controls, a reboot and at least 130 seconds of settling per arm.
Three standard HTTP samples per arm are at least 61 seconds apart. These are
median whole-build steady-state values, with live inputs and caches not frozen:

| Free memory | Before (KiB) | After (KiB) | Change (KiB) |
| --- | ---: | ---: | ---: |
| Internal heap | 74.2 | 80.1 | +6.0 |
| PSRAM | 2092.3 | 2166.8 | +74.5 |
| Heap plus PSRAM | 2166.4 | 2246.9 | +80.5 |
| Largest internal block | 31.0 | 31.0 | 0.0 |
| Largest PSRAM block | 1952.0 | 2016.0 | +64.0 |

Exact combined medians are 2,218,431 and 2,300,871 bytes. Linked static DRAM
data/BSS decreases by 2,424 bytes; that section accounting is separate from live
free memory. This pair supersedes the earlier `fd394cdc1` pair for the installed
S3 fix, without explaining individual allocation changes or historical SDK1
measurements.

Private `0c3e308b1` repeats the resource and completed-callback cohort against
public `d183c6898`. Only three existing callback/lease observer hooks and the
completed-result hold are added; the clock, buffering, decoder, admission guards
and receiver remain unchanged. Descriptor liveness reads at most 64 bytes per
sample, rather than scanning full images. Host ASan/UBSan checks exercise the
actual callback receiver and bounded sampler; negative controls reject a removed
request-cookie guard and an unbounded sampler. These samples establish changing
descriptors, not panel pixels or full-frame golden hashes.

All 11 physical cases pass with 81 cards, 70 climates, two images, nine folders,
1,406 routes and the complete 128 KiB HA cache:

- The 369,953-byte 480 × 480 GIF plays as a card; full-screen expansion is refused
  as RESOURCE before publication, and the retained card resumes without download.
- The 25,825-byte 480 × 480 GIF now plays full-screen, with changing descriptor
  samples and retained-card resumption. Its new full-screen appearance
  receives subsequent manual confirmation in the visual/recovery section below.
- The 6,897-byte 240 × 240 GIF plays full-screen and resumes its card.
- Both 2 MiB 640 × 640 disposal variants are safely refused. The previous image
  and independent PNG remain available, and a fresh replacement PNG completes.
- Actual completed GIF results held for 0, 2 and 8 seconds retain the correct
  lease/cookie, then animate after release. A no-hold control and stale results
  after modal replacement and disjoint full-cache configuration replacement
  also pass without changing the replacement's ownership or state.

Separate local minima are 44,259 internal bytes and 113,364 PSRAM bytes; these
are not simultaneous measurements and must not be added. The latter occurs in
the newly fitting 480 × 480 playback case. The sampled settled 128 KiB stop
margin is not a hard minimum throughout decoding/playback.

Recovery verifies the currently installed public `d183c6898` firmware, exact
original dashboard at generation 1203, all 78 protected preferences, PIN/reset
and update settings, plus over 70 seconds of stability. Retained recovery
artifacts now target this fixed firmware, rather than the earlier 10 MHz image.
No production depth-two writer is enabled by these results.

## S3 memory comparison

### Controlled production pair on ESPHome 2026.10.2

Retained original firmware from source `1963f016a` (compiled 3 November) is
compared with unmodified production source `fd394cdc1` (runtime `a0a4988b2`,
compiled 4 November). The latter has no private YAML probes, diagnostic RPCs,
local heap monitors or API encryption. Both use the exact original 1,151-byte
native-v1 dashboard at generation 1162: two saved subpages and 14 populated
saved card records, including inactive placements.

Each arm disables the screensaver and Home timeout, keeps screen scheduling
off, restarts, returns to Home, settles for 130 seconds and takes three standard HTTP
sensor samples at least 61 seconds apart. Backlight is ON at 255 in all six
samples; no synthetic HA feed or browser runs during collection. Values below
are medians, in KiB. Combined free means internal heap plus PSRAM free bytes.

| Measurement | Before | After | Change |
| --- | ---: | ---: | ---: |
| Internal heap free | 74.1 | 79.8 | +5.7 |
| PSRAM free | 2091.9 | 2109.9 | +18.0 |
| Combined free | 2166.0 | 2189.7 | +23.7 |
| Largest internal block | 31.0 | 31.0 | 0.0 |
| Largest PSRAM block | 1952.0 | 1792.0 | −160.0 |

Combined raw ranges are 2,217,763–2,218,131 bytes before and
2,242,219–2,242,755 after. Free RAM improves about 1.1%; the smaller contiguous
PSRAM block remains relevant to large-image admission. The candidate's RGB
buffers request 19,200 additional internal bytes. Its static data/BSS falls
2,432 bytes while code/read-only sections grow 246,260 bytes; both SDK configs
copy instructions and read-only data into PSRAM. Section sizes are separate
accounting, not exact live heap reservations or isolated renderer savings.

These are settled whole-firmware measurements on the original dashboard, not
image/navigation peaks or a causal explanation of the earlier SDK1 regression.
The protocol does not directly observe image buffers or freeze real HA content.
One first attempt is excluded because its collector expected newer capability
fields on the original v1-only firmware; it performs no candidate upload or
samples and verifies recovery. The corrected run verifies the older contract
and the candidate's explicit depth-one/v1 writer policy. Neither measured arm
writes dashboard data. After sampling, one same-payload HTTP 204 save verifies
both native and legacy completion; exact original firmware, all 78 protected
preferences and PIN/reset/updates are restored at generation 1163, with over
70 seconds of original-firmware stability before automatic updates resume.
Later browser-only transfer corrections retain all 491 component files and
device YAML; their rebuilt web assets have separate host checks.

### Earlier source-specific measurements

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


A private raw-flash harness subsequently passes 78 ASan/UBSan host cases using
unchanged production `ConfigurationStore`, transient backend and ESP-IDF port.
SDK wrappers confine the test to initially erased scratch sectors, retain the
deployed 40 KiB slot geometry and compare the entire simulated 2 MiB partition
after cleanup. Cases cover all seven mutation boundaries and 22 read boundaries
before/after failure, partial writes/erases, newest-payload corruption and
maximum 40,940-byte payloads. Both negative controls fail as intended. These
host results do not establish hardware recovery or physical power interruption.

The first 4.3-inch P4 hardware attempt (`1b23ea650`) refuses before any scratch
read, erase or write. A separate read-only geometry image (`4b2f94fd6`) proves
that this unit's deployed table has no `card_images` partition. The actual
configuration service uses its 16 KiB NVS fallback and advertises a 16,356-byte
durable-document limit. It still advertises reader depth two and writer depth
one. The compiled candidate/recovery partition tables match, but OTA does not
replace the deployed table; artifact matching alone therefore cannot prove the
running storage geometry. The fault run is excluded from raw-flash qualification.

Both attempts restore the exact original SDK2 firmware, unchanged native bytes
and generation 147, all 78 protected settings, PIN/reset and both update
preferences. A transient original-boot configuration GET 503 in the first
cleanup is followed by verified readiness and completed restoration; the
observer now bounds retries to known read-only readiness responses. The
inspection makes no raw-case call. No repartitioning or manual data migration
is performed. The subsequent 10-inch cohort below uses its verified raw-backed
device, with deployed geometry checked before writes.

Private 10-inch V3 image `3ba1e7528` then passes the same 78 cases on real flash.
A preflight observes the writable, unencrypted 2 MiB `card_images` partition and
actual 40,932-byte durable document limit. Only an initially erased 90,112-byte
window at offset 1 MiB is mapped to the unchanged production 40 KiB-slot port.
Task and range guards fence the private mapping. The selected custom MIPI driver,
stock SDK API, original/wrapped symbols and compiled component hashes are
verified before flashing.

The normal commit exposes seven physical mutation and 22 read boundaries.
Every before/after mutation failure, partial erase/write and before/after read
failure recovers a complete old or new synthetic payload through fresh port,
backend and store instances. Initial read failures make no commit mutation.
Corrupting the newest payload selects the older valid slot. A maximum 40,940-byte
opaque payload takes 44 mutation steps; selected partial writes/erases and
publication-marker failures also pass at that size. These are storage-envelope
payloads; native configuration
retains its separate, smaller document limit.

Every case restores the initially erased window and compares all 2 MiB of the
partition byte-for-byte against its pre-case snapshot. The production raw backend
uses no slot-sized transient allocation; the private observer separately borrows
a 2 MiB verification snapshot and payload buffers. Neither the live native
configuration nor its generation changes. Exact original SDK2 firmware/native
generation 363, all 78 protected settings, PIN/reset and both update preferences
are verified restored. This qualifies injected raw-driver failures in isolated
sectors, not faults applied to the live primary configuration, a power cut or a
production memory measurement. Physical power-interruption qualification remains
open; the live primary sectors are never deliberately corrupted by this cohort.


The prepared power-session image (`3f9ba3881`) adds a persistent ownership guard
in the older synthetic slot's unused padding. It pauses a worker immediately
before or after the new slot's outer publication marker; the ordinary loop/API
remain available. Fresh readers can recover the guard after RAM is lost and
refuse cleanup for an unknown or mismatched session. SHA256 checks mask only the
initially erased scratch window during recovery, then verify the full partition
after cleanup through a separate readback action. No production file or writer
policy changes; source/SDK/display/partition/worker/symbol checks pass.

ASan/UBSan core tests pass 22 assertion groups, including four abrupt child
process exits that retain simulated flash while losing the entire private call
stack and RAM. Three negative controls catch missing session, outside-data and
pause guards. All 78 earlier raw-driver host cases still pass with the optional
pause hook; those two negative controls remain effective. Fifteen actual
controller-function checks reject software/reset-pin/brownout reports and
network reconnects as clean power-on evidence, retain the human-confirmation
gate and reconcile a lost cleanup reply without another erase.

On the 10-inch, both cooperative cancellation boundaries pass on this exact
image. The default API response barriers remain responsive while the worker is
paused, inspection is fenced until it stops, and fresh readers select complete
old/new payloads as expected. Both cleanup replies and independent SHA256
readbacks match the pre-session full-partition digest. The observer waits past
the pinned SDK's 60-second successful-boot interval before arming. Exact original
SDK2 firmware/native generation 363, all 78 protected settings, PIN/reset and
both update preferences are restored. These are cancellation and preparation
results; no actual power cut is credited. The prepared controller requires a
fresh power-on reset plus separate human confirmation for the physical cases.

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

Private completed-image probe `7a1587cbf`, based on `0c40d159c`, holds one real
adapter callback after decode and descriptor publication. Image downloaders,
decoders and the public runtime receiver remain unchanged; private read-only
lease reports and a fixed callback record make the delayed boundary observable.
ASan/UBSan host checks cover current/stale results, modal reopening and teardown;
removing only the receiver's request-cookie equality makes the stale-pending
negative control fail.

The S3 PNG run replays nine actual callbacks: two current-result positive
controls update their leases, while seven stale results leave request, pending,
ready, failure, asset and revision fields unchanged within the replay call.
Cases include a new request held behind a server gate, an already-ready image,
folder navigation, disjoint configuration replacement, modal close/reopen,
a failed result and display-off/wake. No callback target is destroyed or
silently dropped instead of replayed, and no scene allocation/accounting errors
occur. The exact original SDK2 firmware/native bytes and all protected
settings, PIN/reset and sleep/update preferences are restored at generation
1112. An initial same-binding protocol waits for a refresh that intentionally
coalesces behind the pending result; it is excluded and restores generation
1109 before the accepted rebinding run. These checks qualify the PNG
adapter/runtime boundary, not panel pixels or production heap usage; the
private instrumentation has its own footprint.

The same private image exercises the actual decoder with a 480 × 480,
369,953-byte GIF. An initial run passes changing tile frames and stale
completions after page rebinding and disjoint configuration, but the expanded
view does not advance after its current completion is artificially held and
released. That run remains failed for animation resumption; its exact original
firmware/data/settings recovery passes at generation 1115. A diagnostic attempt
then stops before the expanded view because its capture check incorrectly
requires an already-retired request to remain current; it is excluded and
restores generation 1117.

A separate run with normal, immediate callbacks passes changing decoded frames
in both the tile and expanded view after navigation and configuration changes.
It reports no allocation or GIF-frame errors and restores the exact original
firmware/data/settings at generation 1120. Its lowest sampled free PSRAM is
218,972 bytes during expanded playback; this is neither an isolated peak nor a
production before/after measurement. Normal playback does not resolve the
forced-delay animation failure above.

The targeted modal follow-up replays two actual completed GIF callbacks after
close/reopen onto a new PNG and after disjoint configuration/new opening.
Both leave request, pending, ready, failure, asset and revision unchanged
within the replay call; both captured requests are current at capture, and
neither target is dropped. No allocation or GIF-frame errors are reported.
Exact original SDK2 firmware/native bytes and all protected settings, PIN/reset
and sleep/update preferences are verified restored at generation 1123.
Together these runs qualify the recorded GIF stale-result boundaries and
normal decoded-frame playback on S3, not delayed-current animation resumption,
panel pixels, other formats or P4 parity.

Private read-only playback probe `182896eac` subsequently reproduces the
expanded-view stall with allocation diagnostics. The corrected collector gates
the intended GIF's HTTP bytes while forwarding earlier completions, avoiding
both a captured PNG refresh and a wait behind an older pending callback.
Those earlier collector attempts are excluded; exact original recoveries pass
at generations 1128 and 1130. An initial ungated diagnostic separately records
five 409,840-byte allocation failures with a 376,832-byte largest block and
restores generation 1126.

With the corrected collector, all three tile/playback/stale-result checks pass.
All four expanded-view cases (zero, two and eight extra seconds of callback
delay, then normal delivery) stall. Each logs a 409,840-byte next-frame
allocation failure with a largest block of 327,680 bytes, followed by a GIF-frame
error. The first case has 650,592 bytes of free PSRAM before that allocation:
enough total bytes, but insufficient contiguous space. Playback is visible and
unblocked before the error, then its animation owner stops. Exact original
firmware/native bytes and protected preferences are restored at generation
1133. This establishes the failure mechanism in this reproduction; the earlier
`7a1587cbf` run lacked allocation logs, so its cause is not retroactively proven.

Private candidate `047637d8e` retains the same diagnostics and changes only the
GIF canvas admission code plus its firmware label. It conservatively reserves a
restore canvas whenever source bytes may contain disposal method three; other
files need one canvas. Actual decoder host measurements for this fixture fall
from 921,600 to 460,800 workspace bytes. ASan/UBSan checks preserve independent
golden compositing for static/interlaced images, animated disposal-one/two
images without a restore canvas, and disposal-three images with both canvases.
They also cover conservative false positives, missing required backup storage,
source mutation, truncation and corrupt inputs. Existing full playback/recovery
checks pass with both host heap variants.

The compiled pair verifies identical SDK/API sources, runtime receiver, hold
instrumentation and collector logic; only the two GIF components differ.
Static RAM section bytes remain 138,500, and the candidate OTA image adds
144 bytes. All seven S3 candidate cases pass, including changing expanded frames
after all three delays and normal delivery, with no allocation/frame errors or
dropped callback targets. Retained scene owners match the before-build.
Tile free-PSRAM samples rise from 1,041,972 to 1,512,712 bytes; the pre-GIF
baselines differ by 12,044 bytes, giving a baseline-adjusted improvement of
458,696 bytes. Expanded playback finishes its four observations with
702,400–702,716 bytes free. These are sampled diagnostic measurements, not
isolated peaks or an uninstrumented production comparison. Exact original
firmware/native bytes and all protected settings, PIN/reset and sleep/update
preferences are verified restored at generation 1136. At that checkpoint the
optimization remains private, with source-scan latency and broader qualification
still open.

The cooperative follow-up scans at most 64 KiB of candidate positions per call,
checking elapsed time between 4 KiB slices and yielding after four milliseconds.
The existing high-frequency requester keeps scanning prompt between ordinary
component loops. Scan progress counts as active decoding, including for the S3
background-result consumer. `Player::open` no longer repeats the full scan;
missing restore storage fails safely at the disposal guard if needed.
Host ASan/UBSan checks cover chunk boundaries, all packed control values,
random/corrupt inputs, 2/8 MiB sources, late restore markers, known/unknown
length, no early decoder allocation, cancellation, pause/resume, failure,
allocation exhaustion and clock wraparound. Negative controls removing active
scan reporting or delaying the time guard fail their targeted assertions.
The real 480-pixel fixture keeps its 460,800-byte canvas workspace and matches
an independent Pillow 12.3 first-frame golden.

Private timed source `775e313b5` adds three fixed `uint32_t` counters per decoder
and read-only scan reports. All seven S3 cases pass, with six scan calls per
369,953-byte GIF and a maximum observed scan duration of 3,876 microseconds.
The predeclared limits are 8,000 microseconds and 65,536 candidate positions per
call. No allocation/frame/scene-accounting errors or dropped callback targets
occur. Static RAM section bytes remain unchanged from `047637d8e`; the diagnostic
OTA image adds 400 bytes. Exact original firmware/native bytes and all protected
settings, PIN/reset and sleep/update preferences are restored at generation 1139.

P4 overlay `5da168eb9` has identical components and passes the same seven cases
on the 4.3-inch RISC-V panel, with a maximum scan duration of 1,914 microseconds.
Its profile-specific host fixture retains seven card definitions on the six-slot
grid; four replacements and folder round trips pass with six state registrations
and no new scene-arena allocation. Original SDK2 firmware/native bytes and all
78 configured settings, PIN/reset and both update preferences are restored at
generation 141. Host-client cross-loop timezone notices in this first cohort do
not qualify timezone behaviour.

A separate P4 run admits 2 MiB and 8 MiB inputs containing an 8 × 8 animated GIF
prefix and repetitive trailing marker bytes. Complete source scans take 59/233
calls, reach positions 2,097,149/8,388,605, and peak at 4,290/4,305 microseconds
per call. Both inputs animate without allocation/frame errors. Free PSRAM after
source reclamation is 25,328,176/25,328,260 bytes against a 25,328,180-byte
baseline, while retaining the four-MiB stop margin. Exact original firmware,
native bytes and protected preferences are restored at generation 143.
An earlier attempt stops on a native GET 503 immediately after boot, before any
fixture write or decode, and restores generation 141. Its response body was not
captured. The corrected collector only retries documented temporary native GET
503 responses for thirty seconds; writes/actions never retry. It disables host
time provision. These cases qualify source admission/scanning, decoded animation
and reclamation, not maximum pixel dimensions, isolated peaks, physical panel
pixels or simultaneous GIF pressure.

The production port keeps only the functional changes in the three GIF files;
all private timing counters/getters and artwork/runtime/YAML instrumentation are
excluded. Actual production Player/runtime ASan/UBSan checks pass with both heap
variants. The four GIF fixtures and their independent goldens reproduce exactly
with Pillow 12.3. Factory snapshot `b1847f0bb` passes all nine sequential
ESPHome 2026.10.2 profiles. Each build verifies the eight protected GIF/lookup
source files, MCU, unmodified SDK API, distinct original/wrapped handler symbols
and unchanged native-v1/depth-one writer policy.

YAML-only observation overlay `43342c2c6` then compiles with every external
component byte identical to that production snapshot and unchanged SDK API
sources. It observes current image URLs, published descriptor hashes and public
memory snapshots, without callback holds or private decoder counters. Each
accepted playback case requires a fresh real GIF-source notice and at least
three distinct descriptor hashes. These are decoded-image checks, not panel
pixels or isolated heap peaks.

The production-component S3 passes tile, folder and expanded 480 × 480 GIF
playback, the disposal-three fixture, and another card playback before the
combined test. The disposal fixture changes only its repeat count for sampling;
Pillow verifies all four composited frames remain identical. **The combined
card/expanded GIF case fails:** five attempts cannot allocate a 409,840-byte
expanded surface, each reporting a largest block of 327,680 bytes. The expanded
image never becomes ready, so card resumption is not tested and the seven-case
cohort is not a pass. Total free PSRAM at those failures was not captured. The
five earlier passing cases do not qualify simultaneous GIF pressure. Exact
original firmware/native bytes, all 78 protected preferences, PIN/reset, sleep
and original update preference are verified restored at generation 1141.

The production-component 4.3-inch P4 passes all six cases: 480 × 480 tile,
folder and expanded playback, disposal-three, and the 2/8 MiB animated 8 × 8
sources with repetitive trailing markers. Source reclamation returns within the
one-MiB baseline tolerance; no allocation/frame/fallback or scene-accounting
errors are recorded. These source-ceiling checks do not qualify maximum pixel
images or simultaneous GIF pressure. Exact original firmware/native bytes,
all 78 protected preferences, PIN/reset, sleep and both original update
preferences are verified restored by the outer ownership finalizer at generation
145. All displays are back on their original firmware after these tests.

The follow-up `b0b4f286b` releases a hidden S3 GIF's working player, compositor,
resize axes and staging surface while retaining its compressed source, completed
picture and GIF ownership. Visibility restores the working state locally through
cooperative replay, preserving exact frame position and finite repeat counts.
Busy replacement staging is protected. P4 does not request suspension; its shared
decoder initialization is separately requalified. No new source limits, storage
format, writer capabilities, callback holds or private decoder counters are added.

Actual host ASan/UBSan checks pass for normal/S3 policy with both heap variants,
including exact disposal-one/two/three frames, finite repeats, a ready next frame,
partial/repeated reconstruction, time yielding, mandatory allocation failure and
destruction. A 16-frame fixture independently verified with Pillow exercises
re-covering after a yielded replay. Three copied-input negative controls catch
missing loop restoration, a lost replay destination and disabled hidden-owner
release. The first host assertion incorrectly classified the fixture source as
external allocation; that assertion-only attempt is excluded and preserved.

With the same real GIFs, observations and 128-KiB sampled free-PSRAM floor, the
S3 now passes all seven cases. The combined expanded view animates while its
covered card retains a loaded/current completed descriptor. Closing it resumes
card animation with the server request count unchanged at 16. No allocation,
frame/fallback or scene-accounting errors are recorded. The combined sample has
330,592 bytes free PSRAM and 81,171 bytes internal heap (411,763 combined), with
a 143,360-byte largest PSRAM block after the required surfaces are allocated.
This is a settled diagnostic sample, not an isolated peak or a numerical
before/after difference: total free memory at the earlier failure was not
captured. Exact original firmware/native bytes and all 78 protected preferences,
PIN/reset and sleep/update preferences are restored at generation 1143.

P4 overlay `0ca7e9039` changes only its observation label from `b0b4f286b`;
every component byte is identical. All six normal/disposal-three/2-MiB/8-MiB
cases pass without allocation/frame/fallback or scene-accounting errors, with
source reclamation and outer ownership finalization. Exact original firmware,
native bytes, 78 protected preferences, PIN/reset, sleep and both original
update preferences are restored at generation 147.

All nine factory profiles pass for immutable component snapshot `b0b4f286b`.
Every build verifies ten protected source files, stock SDK API, MCU, distinct
original/wrapped native-handler symbols, no lab actions and unchanged native-v1/
depth-one writer policy. The production port copies the five functional C++
files and runtime tests byte-for-byte; observation YAML stays private. These
checks do not establish panel pixels, maximum-layout simultaneous image pressure,
isolated peaks or resolution of the historical original-dashboard regression.

A separate populated-cache S3 follow-up on these same components passes four
playback/navigation cases with 56 climate models and two image cards (65 cards,
eight pages). A fresh client audits exactly 1,126 unique/raw subscriptions;
1,120 ordered climate updates supply 98,304 value bytes. Tracked cache use is
98,430 bytes after image state arrives, within its 131,072-byte reservation.
The 480 × 480 GIF has 16 solid-colour frames and is 25,825 compressed bytes,
independently decoded with Pillow. This extends pixel dimensions and populated
state coverage, not maximum compressed-source pressure or individual cache-value
readback.

Card playback, expanded playback, card resumption and folder/Home return each
produce at least three distinct current descriptors. The covered card keeps its
completed picture; modal-close resumption makes no additional HTTP request
(three requests before and after). Store/layout/view/widget/metadata/channel/
cache-capacity counters remain equal to the populated baseline. The counted
arena stays at 239,579 bytes/seven allocations outside the modal, adding only
its known 544-byte/two-allocation context while open. No image allocation,
frame/fallback or scene-accounting errors are recorded. The expanded-view sample
reports 222,516 bytes free PSRAM and 80,651 internal heap (303,167 combined),
above the unchanged 128-KiB free-PSRAM floor. These are observed samples, not
isolated peaks, a before/after comparison or panel-pixel confirmation.

The first attempt passes the first three image cases and reaches the correct
child page, but its checker incorrectly equates root slot three with PageId
three. The exact host fixture verifies root slots 3–9 map to child PageIds 1–7
across all four replacements, with Home zero. The corrected collector changes
that expectation to one and passes the full cohort; inputs, memory margins and
ownership requirements stay unchanged. Both attempts verify complete original
recovery, latest generation 1147 with exact firmware/native bytes, all 78
protected preferences, PIN/reset and sleep/update preferences restored.

The same populated A/B fixtures also pass two disjoint replacements with a real
incomplete HTTP image transfer: GIF cancellation after 4,384 of 25,825 bytes and
PNG cancellation after 4,368 of 219,904 bytes. The server sends an initial 4 KiB,
then 16 bytes every 200 ms while gated, maintaining read activity so a timeout
cannot stand in for cancellation. Two live-GIF warm-ups and both replacements
pass. Each new configuration audits all 1,126 routes and refills its climate
cache; another 1,126 retired climate/image updates are then delivered. The
current PNG component URL/descriptor hash stays unchanged across three samples,
tracked cache/resource counters remain equal, and no new HTTP request appears
in the retired-update observation windows. No image/scene errors are recorded.
These are incomplete-transfer cancellation and component/resource checks, not
an intentionally delayed completed-decode replay, consumer lease-field readback
or panel pixels. Exact original firmware/native bytes and all 78 protected
preferences, PIN/reset and sleep/update preferences are restored at generation
1151 after both replacements.

Current-source private probe `e10e0b464`, based on public `2fbeb8af4`, then
passes seven actual completed-GIF callback cases. The private hooks hold a
non-cached result after real 480 × 480 / 369,953-byte GIF decoding and descriptor
publication, before the unchanged runtime receiver. A current result becomes
ready and animates; stale results after page and disjoint-configuration changes
leave the new request, pending/ready/failed flags, asset and revision unchanged.
Modal completion held for zero, two and eight seconds, plus normal playback,
also passes. No allocation, frame/fallback or scene-accounting errors are
recorded. The receiver host checks pass under ASan/UBSan; stripping the private
hooks restores the public receiver and functional image code byte-for-byte.
These small image-only fixtures cover the completed-result boundary separately
from the populated incomplete-transfer cases above. They do not establish panel
pixels or isolated peaks. Exact original firmware/native bytes, all 78 protected
preferences, PIN/reset and sleep/update preferences are verified restored at
generation 1154.
A browser follow-up separates saved card identities from visible cell count
in both transfer validation and preview rendering. Five model regressions cover
retained cards above all four grid capacities and the saved-ID boundary 32;
inactive siblings, Back appearance and complete subtrees stay intact. Browser
journeys exercise actual copy, duplicate, cut and delete menus on all four grids.
All 262 web-unit checks, types and generated-bundle freshness pass. The complete
nine-layout browser suite also passes at `4783dc76c`, as does required CI.
C++ sources are unchanged from the completed `d3cb407ae` factory matrix;
this follow-up's rebuilt web assets have separate browser qualification.

A further transfer follow-up retains the first hidden order token when Back is
implicit and allocates destination identities across all 32 supported saved
records on subpages, independently of visible grid capacity. Home retains its
profile's renderable identity limit. Sixteen model regressions cover implicit/
explicit Back, subpage paste/duplicate with occupied invisible records and
dormant descendants, and rejected Home copy/cut across all four grids. Eight
reproduce the original failures; four further Home cases catch an overbroad
allocation limit in the first correction. All 278 web-unit checks, direct
TypeScript checking, generated-bundle freshness, web manifest and backup
contracts pass. Rejected Home moves keep the source and all dormant data;
existing full-storage tests still reject partial batches. These are host checks;
this follow-up changes no C++ or device YAML.

## Maximum GIF canvas and mixed-layout host checks

At public checkpoint `476ea10d6` (runtime code unchanged from `efc6dcf09`),
four independently generated GIFs combine the decoder's maximum 640 × 640
canvas with exact 2 MiB and 8 MiB sources, with and without restore-previous
disposal. More than 99.9% of each source consists of encoded image blocks
(LZW data and sub-block framing);
each contains five or seventeen frames, including a partial final rectangle.
Valid comments before the trailer fill the remaining bytes. Under ASan/UBSan,
the production player matches every Pillow 12.3.0 RGB565 reference pixel over
two plays. Normal playback uses an 819,200-byte canvas; restore-previous needs
1,638,400 bytes for both canvases. The checks also verify bounded restore scans,
rejection beyond the pixel/dimension/source limits, and safe failure when the
required restore canvas is absent. A deliberately corrupted reference fails
the pixel comparison. These are host player checks, not physical allocations
or full download/resize/modal pipeline measurements.

A separate current-source scene/state/renderer check uses the real LVGL owner
with fake hardware, artwork and HA transport. Its exact native-v1 fixtures
contain 400 first-level cards: 20 folders, 378 climate controls and two image
cards. Four A/A/B/B activations retain all 7,566 unique expected entity/attribute
pairs and a full 524,288-byte value cache, including image URLs and state.
Retired-generation updates are ignored. All 80 folder round trips keep every
visible cell bound and require no new arena allocation, even with further
arena allocations disabled. The counted scene reservation stays at 1,303,402
bytes and seven live allocations across all four activations. This qualifies
the mixed fixture and owner lifecycle on host; real downloads, modals and
physical heap measurements require separate device evidence.

The subsequent physical 10-inch V3 cohort uses private `68643cc0d`, based on
public `526cf5153`, with all 491 existing component-source hashes unchanged.
Only YAML adds resource/descriptor observations and an ESP-IDF local-minimum
monitor. Native-v1/depth-one writes remain unchanged. A fresh observer receives
all 7,566 unique expected registrations in 126.052 seconds; an ordered feed
fills the entire 524,288-byte cache, including image URLs/state, in 72.773
seconds. Eight tile/expanded cases pass with the same 400-card mixed layout:
640 × 640 GIFs at both source ceilings, normal and restore-previous disposal.
Each publishes at least three distinct current component descriptors, and all
eight HTTP GIF responses complete (40 MiB total). Tile cases also retain a PNG;
expanded cases start from Home with the child tile inactive. This does not
qualify simultaneous large tile/expanded GIF ownership or physical pixels.

| Surface | Source | Disposal | Internal RAM local minimum, bytes | PSRAM local minimum, bytes |
| --- | --- | --- | ---: | ---: |
| Tile | 2 MiB | Keep | 298,659 | 13,893,440 |
| Tile | 8 MiB | Keep | 298,695 | 7,601,916 |
| Tile | 2 MiB | Restore previous | 298,567 | 13,074,172 |
| Tile | 8 MiB | Restore previous | 298,071 | 6,782,720 |
| Expanded | 2 MiB | Keep | 297,279 | 12,465,568 |
| Expanded | 8 MiB | Keep | 297,883 | 6,174,088 |
| Expanded | 2 MiB | Restore previous | 300,195 | 11,646,360 |
| Expanded | 8 MiB | Restore previous | 299,387 | 5,354,904 |

Each monitor window starts before the request and ends after playback is
observed; full heap snapshots occur outside it. These are instrumented workload
low-water marks, including background/API activity. ESP-IDF aggregates minima
per heap region, so the two pools must not be summed into a simultaneous
combined minimum. No allocation/frame/fallback or scene-accounting failures
are recorded. After final sleep/takeover release, the full cache and seven scene
allocations remain, reserving 1,252,986 scene bytes. Free PSRAM is 17,164,744
bytes, within 24 bytes of the initial full-cache reading; image leases and
active/retired modal contexts are zero. This extends the earlier source/pixel
checks to one real maximum-layout image workload, not an original-dashboard
before/after comparison or uninstrumented isolated decoder peak.

The initial restoration exposes a qualification-controller gap: its read-only
bridge makes two exact-original native commits but leaves the legacy copy written
by this DUAL_WRITE cohort. The original-image upload acknowledges success, but
the device reconnects on the cleanup image; one controlled restart does not
establish the original image either. A subsequent exact-original DUAL_WRITE
save acknowledges complete native/legacy persistence with HTTP 204. The same
original firmware then reconnects and stays up for over 70 seconds. Exact
original dashboard bytes, all 78 protected settings, PIN/reset, sleep and both
update preferences are verified restored at generation 367. The failed attempt
is preserved separately. Corrected controller recovery now requires the legacy
repair before old-grid boot; four host policy checks pass and the original
native-only helper fails the stale-legacy negative control. This observation
does not establish a bootloader rollback reason or count as a physical power
interruption.

The next, separate overlap cohort uses infinite-loop versions of the same
maximum-canvas assets; Pillow pixels and two production-player plays plus the
next frame still match under ASan/UBSan. On the unchanged `68643cc0d` observer,
the 2 MiB normal card/expanded pair passes and the card resumes without another
HTTP request during five seconds after close. The 8 MiB pair reaches the
explicit 4 MiB transfer-reserve guard and `ESP_ERR_NO_MEM` before buffering the
second response. This attempt stops when an expanded retry occurs before
close. That retry alone does not establish unsafe behavior: the probe's
one-request expectation was stricter than the existing backoff policy.
The completed restoration now repairs the legacy copy automatically, validates
the original image for over 70 seconds and restores all protected state at
generation 371.

A separate host regression confirms the associated policy defect: a local
expanded-image failure is treated as a source/camera failure, invalidating a
complete tile and starting a replacement after close. The working fix carries
the resource-failure classification through the stamped completion and gives
it a modal-only retry delay, leaving the tile's source state intact. Targeted
host checks now pass for image/camera tiles and both pre-request admission and
completed resource failures; source/network retry checks continue to pass.
Actual-component adapter checks cover classification, successful replacement,
rejected start and stale completion.

The separate fixed observer `7da689e07` uses implementation snapshot
`f0a12293e`, with only YAML observation changes. All four physical overlap cases
pass on the 10-inch V3 with the same 400-card fixture, exactly 7,566 routes and
full 512-KiB value cache. Both 2-MiB cases play in the expanded view; both 8-MiB
cases explicitly refuse the second source at the transfer-reserve guard.
Normal and restore-previous disposal both preserve the covered card's complete,
current descriptor. After close, each card resumes changing frames for at least
five seconds without another HTTP request. Host tests separately enforce the
modal-only retry delay; these short physical refusal windows do not establish
repeated-retry timing.

| Source / disposal | Expanded result | Internal / PSRAM low-water bytes |
| --- | --- | ---: |
| 2 MiB / normal | Playback | 300,067 / 9,253,812 |
| 8 MiB / normal | Resource refusal | 304,143 / 7,329,112 |
| 2 MiB / restore previous | Playback | 298,363 / 7,615,344 |
| 8 MiB / restore previous | Resource refusal | 303,187 / 6,509,900 |

These instrumented per-region low-water marks cover the expanded attempt;
they are not simultaneous combined totals or an original-dashboard before/after
comparison. This qualifies the existing transfer resource classification and
pre-request admission path, not every decoder allocation failure or panel
pixels. Exact original firmware and dashboard bytes remain stable for over
70 seconds; all 78 protected settings, PIN/reset, sleep and both update
preferences are restored at native generation 375, following the corrected
native-plus-legacy recovery procedure.

A subsequent decoder audit finds that generic preparation and streaming-decode
errors discard the explicit `DECODE_ERROR_OUT_OF_MEMORY` classification. The
follow-up maps only that result to RESOURCE; malformed and unsupported content
keep their existing classification. The new registered host test executes the
actual production methods with controlled GIF/JPEG/PNG/BMP backends. Preparation
and decode allocation failures, cleanup, content errors, partial/full/invalid
consumption and unsupported formats pass. Removing either new mapping makes
its resource assertion fail. Five selected runtime/adapter/ownership/policy
checks pass on AppleClang; decoder, ownership, resize and release checks pass
with pinned-image Linux GCC. This is host classification evidence, separate
from the preceding physical transfer-refusal cohort; updated firmware and S3
maximum-pressure qualification remain to be run.

The explicit-error follow-up is public `db06ef47a`; all nine factory profiles
and GitHub CI pass. Its YAML-only S3 observer `ab819502d` reaches the maximum
81-card layout (70 climates, two images, nine folders), exactly 1,406 unique
registrations and all 131,072 cache bytes. The 480 × 480 / 369,953-byte card GIF
advances. Expansion then fails a 409,840-byte output allocation with a largest
available block of 90,112 bytes. The component still reports RESOURCE false:
the GIF frame writer collapses that failure to INVALID_TYPE, bypassing the
explicit-error mapping. The trial stops before the 640-pixel pressure cases.
An earlier attempt is excluded because its measurement script queried modal
slot one; the corrected slot-zero probe has its own host regression. Both
attempts restore exact original firmware/dashboard, PIN/reset and all 78
protected preferences, with acknowledged native/legacy persistence and over
70 seconds of original-image stability; the latest native generation is 1158.

The resulting fix records RESOURCE at the failed output allocation itself,
and at a failed non-empty filtered-resize workspace allocation. Invalid resize
geometry keeps its content classification. The production resize/receiver
regression reproduces the lost classification; the actual resize integration
test covers workspace exhaustion and invalid geometry. Four Linux GCC checks
and five selected host checks pass; removing either allocation-origin mapping
fails its resource assertion. These are host results pending the next hardware
retest; the full-layout expanded GIF is not claimed to fit on the S3.

With public `5d8d92bd5` and YAML-only observer `61685e22e`, the same maximum
S3 layout now reports the 369,953-byte expansion failure as RESOURCE. The
covered card remains complete and resumes five distinct frames during a
5.25-second window; server requests stay at four across close. A separate
480 × 480 / 25,825-byte control exposes a later admission gap: its first
expanded frame publishes, but allocating the next 409,840-byte staging surface
fails against a 376,832-byte largest block. Playback stops at the first frame.
This is not a passing full-layout GIF qualification. The two 640-pixel cases
remain unrun. Exact original firmware/dashboard, native/legacy persistence,
PIN/reset and 78 protected preferences are restored at generation 1160 and
observed stable over 70 seconds. The next fix must reserve playback staging
before accepting an animated download; a successful first frame alone does
not establish sustainable playback.

Public `a0a4988b2` reserves the next-frame surface before promotion,
checks the existing 96-KiB PSRAM headroom and transfers the reservation through
cleanup before notifying success. Failure leaves the preceding image intact.
Slot preparation preserves the current owner, rejects occupied visible slots
and reclaims only eligible hidden owners before admission. The S3 decoder keeps
and reuses its resize workspace between visible frames; suspension/cancellation
still releases it. Other platforms retain their frame-release policy.
Production first-frame publication tests pass for normal/S3 allocation,
headroom, cleanup, stale generation, static fallback and hidden-cache cases.
The actual GIF runtime passes all four normal/S3 heap variants under ASan/UBSan,
including independent frame goldens and advancing admitted S3 frames with
further allocations disabled. Five GCC lifecycle/resize checks pass. Four
targeted negative controls catch disabled reservation, missing staging transfer,
missing generation fencing and premature workspace release, after verifying
their positive baselines. An earlier negative run is excluded because a test
inherited the preceding case's low-memory setup.

YAML-only observer `a5909b710` verifies all 491 production component files
against that public source and compiles on ESPHome 2026.10.2. Its five physical
S3 cases pass with 81 first-level cards, 1,406 registered routes and exactly
131,072 cached value bytes throughout:

| Source | Expanded/source result | Internal heap minimum | PSRAM minimum |
| --- | --- | ---: | ---: |
| 480 × 480, 369,953 bytes | Expansion refused before publication | 71,203 B | 241,648 B |
| 480 × 480, 25,825 bytes | Playback staging refused before publication | 71,507 B | 78,524 B |
| 240 × 240, 6,897 bytes | Expanded playback, three changing descriptors | 71,415 B | 417,408 B |
| 640 × 640, 2 MiB, normal disposal | Source refused; previous PNG retained | 69,155 B | 1,431,224 B |
| 640 × 640, 2 MiB, restore-previous | Source refused; previous PNG retained | 70,023 B | 1,432,408 B |

These are separate region minima during each operation, never a simultaneous
combined minimum. The 96-KiB check governs admission after reservation; attempted
allocations can temporarily reduce free PSRAM below that threshold before being
refused. Both 480-pixel refusals report RESOURCE with no expanded image
published; the old first-frame-then-freeze outcome is explicitly rejected.
Closing the three expansions resumes five, five and eight changing card
descriptors over more than five seconds, with HTTP counts unchanged at 4, 7
and 10. Both 640-pixel refusals preserve the independent PNG, and subsequent
fresh PNG requests complete and publish their expected colour. There are no
unexpected decoder, frame or accounting errors. The additional 240-pixel control
has independent Pillow 12.3.0 pixel goldens verified against the production
player over three plays; it supplements all original 480/640 cases.

Exact original firmware, native and legacy dashboard, PIN/reset and all 78
protected preferences are restored at generation 1162. Original firmware stays
stable over 70 seconds before updates resume. This is instrumented component
and allocation evidence, not panel-pixel confirmation, a matched original-
dashboard memory delta or power-loss qualification. All nine factory profiles pass at `a0a4988b2`, verifying 25 runtime source
files, SDK, MCU, stock API, distinct native-handler symbols, no lab actions
and unchanged writer policy. Each completed build retains hash-verified ELF/bin
artifacts while removing only compiler intermediates. Previous matrices retain
their source scopes.

## Flat storage migration on the 10-inch V3

Public checkpoint `efc6dcf09` passes all nine sequential ESPHome 2026.10.2
factory builds, including the lossless durable-format selector. A private
10-inch V3 image (`d123867f1`) changes only the preferred/current writer to v2,
selects read/import-only legacy handling and adds YAML memory/index observations.
Writable depth remains one; the other compiled components match the public
checkpoint. This qualifies the future flat-writer transition separately from
production enablement or nested writes.

The exact historical 400-card/380-climate fixture imports as 25,087-byte native
v1; its v2 representation occupies 53,407 bytes against the 40,932-byte durable
limit. The real current browser-client module reads v2, submits a 53,423-byte
label edit with v2 CAS, and saves losslessly as 25,095-byte v1. Its backup reads
v2 then follows the durable-version header to export the exact stored v1 bytes.
Both the old fixture and that new backup restore successfully. Stale CAS returns
409 and a nested write returns 422, without changing data or generation. A
software restart preserves the exact saved bytes and generation.

Fresh connections audit all 7,600 unique climate routes before and after restart
in 126.63/128.56 seconds. Each 7,600-frame grouped feed fills all 524,288 state
bytes, with an ordered DeviceInfo barrier every sixteen frames, taking
85.97/78.71 seconds. These are paced aggregate observations, not isolated raw
throughput or individual value readback. Folder/Home navigation retains the
full cache and scene owners: 1,355,893 arena bytes in seven allocations. The final
Home sample has 328,183 internal heap bytes and 16,933,460 free PSRAM bytes.
No sampled allocation, index or scene-accounting errors occur. This is neither
an original-dashboard memory delta nor panel-pixel confirmation.

The final cohort restores the exact original native document twice through a
read/import-only v1 bridge (`e560de7e0`), then reinstalls the exact original SDK2
firmware. Native hash, generation 363, all 78 protected settings, PIN/reset,
sleep and both update preferences are verified. Two ordinary commits/readbacks
qualify cleanup; they do not inspect raw slots or test power interruption.

Two earlier attempts remain excluded: the first observed an acknowledged timeout
setting before its asynchronous publication, and the second allowed only ninety
seconds for stock SDK subscription enumeration. The runner now waits for setting
publication and bounds enumeration at 240 seconds with a thirty-second stall
guard and progress counts. That second attempt's extra original-firmware native
commit returned 202 with a legacy-mirror warning; exact native data and protected
settings were subsequently verified restored. The final passing cohort performs
both cleanup commits on the read/import-only bridge, each returning 204, and
makes no legacy-mirror equivalence claim.

## S3 display recovery history

The user reports whole-screen shifting/breakup during the current S3 completed-
GIF test at maximum layout/cache load. Three delayed-current cases (0, 2 and
8 seconds) pass their component/lease checks before the run is cancelled; this
is a visual failure, not a completed qualification. Exact original firmware,
native/legacy dashboard and all protected preferences are restored at generation
1165, with over 70 seconds of stability; the user confirms the original is stable.
A production control removes all image hashing, private hooks and memory polling
during animation. The same whole-screen shift still occurs, so diagnostic reads
alone do not explain the failure. Its exact-original recovery passes at generation
1167. A 40-row RGB bounce-buffer candidate (`b86c85036530`) compiles for both
development and factory S3 profiles, retains 60,827 bytes of free internal heap
before animation, but still flickers according to the user. Exact-original
recovery passes at generation 1169, including native/legacy HTTP 204, 78 protected
preferences and over 70 seconds of stability.

The `952477c1c` candidate restores 30 bounce rows and folds display rotation into IDF's
existing RGB framebuffer copy. LVGL can then omit its separate 57,600-byte PSRAM
rotation buffer and extra copy per redraw. Host checks execute the production
write methods against both an API model and the pinned IDF pixel-copy macros:
all four rotations, padded rectangles, already-rotated fallback buffers and
failed-transform retry pass under ASan/UBSan; the API model also passes Linux
GCC. Development and affected S3 factory builds pass, including selected driver
rotation, stock SDK/API, partition/ELF and 27 protected-source checks; CI is
green. The same uninstrumented maximum-layout visual test still flickers
according to the user. Its pre-animation free heap is 80,131 bytes with a
31,744-byte largest block and 1,644,160 bytes free PSRAM; exact original recovery
passes at generation 1171 (native/legacy HTTP 204, 78 protected preferences and
over 70 seconds of stability). The copy reduction is not a flicker fix. A private
IRAM GDMA-reset counter (`131045f77`, all 491 production component files
unchanged) records zero resets in 20.008 seconds idle, 756 in 30.476 seconds
of expanded GIF playback, then zero in 20.015 seconds after returning Home.
The caller is `0x40376ba8` inside the stock RGB VSYNC recovery ISR. The pinned
driver restarts when expected bounce-buffer EOF events are missed; unconditional
VSYNC restart is disabled and the application does not request resets. This
establishes scanout-service failures during animation; it does not measure cache
misses or establish visual correctness. Exact-original restoration passes at
generation 1173. A 14,400-byte internal LVGL draw buffer (`98462b13e`) still
records 697 resets in 30.021 seconds and the user still sees jitter. An
interrupted cleanup is resumed with exact ownership checks; original firmware,
data and all protected preferences are verified restored at generation 1175.
Other device configurations and
production writer/storage policies are unchanged. Earlier memory tables retain
their software-rotation source scope.


On November 5, verified private `705b3ae8a` isolates rendering from GIF work at
the same maximum layout/cache load. Each row below uses a 20-second interval;
scalar counter reads happen only at its boundaries.

| Phase | Display-DMA recovery resets |
| --- | ---: |
| Idle Home | 0 |
| Expanded GIF, rendering active | 480 |
| Rendering paused, GIF processing continues | 695 |
| Rendering resumed | 490 |
| Return to Home | 0 |

Existing DEBUG frame-completion notices collected outside the paused interval
confirm ongoing GIF processing: decode takes 29 ms and resizing 458–467 ms.
This excludes framebuffer rendering as the sole cause. Original firmware,
native/legacy data and 78 protected preferences are restored at generation 1177.
The current brightness-mode setting is captured read-only before this cohort;
the historical settings snapshot remains intact.

A rows-only internal-memory candidate (`2df8630dc`) reduces measured resize
work to 296–298 ms but still records 678 resets in 30 seconds. It is not a
stability pass; exact-original recovery passes at generation 1179. The next
private candidate (`f0a06fd66`) also keeps the optional lookup cache internal
(with exact arithmetic fallback if it cannot allocate) and installs a
7,680-byte internal LVGL draw buffer through the public API. Original ESPHome
buffer storage remains alive for paused snow. Host ASan/UBSan and Linux GCC
checks cover required-workspace refusal, optional-cache failure, correct pixels
and already-admitted playback with allocation disabled. Prior source fails the
new placement regressions. These checks do not establish physical stability.
The first device attempt reaches verified draw-buffer placement but ends before
expanded completion after a fixed three-second wait. A bounded-completion rerun
loads the 470 × 436 expansion in 3.999 seconds, measures 244 ms resize work and
still records 536 resets in 30.587 seconds; original recovery passes at generation
1183. This is a failed stability candidate.

Direct framebuffer DMA (`4ceec7012`) removes the CPU refill step. The stock
57,600-byte draw buffer fits internal RAM and expanded playback completes.
The user sees stable colours but a whole-layout shift of about 200 pixels: a
visual failure. Direct-mode reset counts do not detect FIFO underruns and cannot
establish pixel correctness. Original recovery passes at generation 1185.

Private `4f87813af` adds `CONFIG_LCD_RGB_RESTART_IN_VSYNC=y` to resynchronise the
pixel stream every vertical refresh. The measured 37.711 resets/second are
expected periodic resynchronisations, not underrun detections. Expanded GIF
playback completes; active standard readings are 48,279 internal free bytes,
31,744 largest internal block, 622,192 free PSRAM bytes and 40,748 internal boot
minimum bytes. These are instrumented maximum-load readings, not a matched
original-dashboard comparison. The initially unobserved visual window receives
late feedback reporting substantial sideways jitter. This variant fails physical
qualification; periodic resynchronisation is not an accepted fix. Exact current
original firmware/dashboard and all protected settings are restored at
generation 1187.

The corresponding public candidate (`ba1392483`) passes its uninstrumented S3
development and affected factory builds, including actual ELF identity checks;
GitHub CI also passes. A normal-firmware maximum-layout retest is cooperatively
cancelled after that negative reply, before its formal visual window. It is not
an additional pixel observation. Exact original firmware/dashboard and all 78
protected settings are verified restored at generation 1189. No memory readings
from an earlier source establish the before/after cost of this candidate. Direct
DMA with periodic VSYNC recovery remains unqualified and must not be presented
as a flicker fix.

Private `948ad815f` restores 30-row bounce buffering and conditional recovery,
then places the 5,503-byte optimized GIF resize function in actual linked IRAM
instead of external instruction storage. It still records 648 recoveries in
30.012 seconds and 89 during the transition; the user reports continued jitter.
Original recovery passes at generation 1191. Private `f6e725160` instead leaves
all 491 component sources unchanged and reduces bounce buffering to 10 rows.
The stock 57,600-byte LVGL buffer then fits internal RAM, but the user still
reports jitter; the experiment is cooperatively stopped and original recovery
passes at generation 1193. Neither is an accepted fix. The public S3 defaults
return to 30-row buffering and conditional recovery; the GIF scanout fault
remains unresolved.

The finite pipeline diagnostic (`de5be9908`) runs with LVGL paused and verifies
completed work counters for each isolated stage. Thirty-second recovery counts
are 1,064 for normal playback, 887 for decoding alone, 965 for resizing alone,
861 for copying completed frames alone and 1,065 for normal playback again.
The isolated stages use bounded waits and different cycle counts, so these
rates do not establish a single responsible stage. The user reports worse
jitter; original recovery passes at generation 1195.

The missing frozen-image control (`f698a52d5`) retains the image and buffers
while stopping all decoding, resizing and publication. At 10 MHz it still
records 601 recoveries in 20 seconds, with all three work counters unchanged.
This rules out active GIF processing as a sufficient explanation. With the
same paused renderer and maximum layout/cache, normal playback records 707,
542 and 258 recoveries at requested clocks of 10, 9 and 8 MHz respectively.
At 6 MHz, both normal playback (28 completed frames) and the frozen control
record zero recoveries over separate 20-second intervals. Returning to 10 MHz
reproduces 709 recoveries during playback and 589 while frozen. Observed VSYNC
callback rates change from about 37.7 to 22.6 per second; this lower refresh
rate needs a separate appearance check. Linked conditional-recovery ISR sizes
match the original binary, excluding the earlier unconditional-restart variant.
Exact original firmware/dashboard/settings are restored at generation 1197.
The uninstrumented 6 MHz candidate (`5fab98845`, all 491 component files
unchanged) passes source/SDK/partition/actual-ELF-identity checks. At the maximum
layout/cache, actual expanded-image completion and two completed-frame notices
precede a visual window without device polling. During that window the user
confirms both “there's no jitter now” and “Stable and correctly positioned” for
the changing full-screen Image 2, including flicker and breakup checks. This
qualifies that visible animation at the lower refresh rate. The public S3 clock
is changed to 6 MHz with 30-row buffering and conditional recovery retained.
The latest-results section records the subsequent production builds, matched
memory pair and 11-case runtime cohort; no depth-two writer is enabled.

The interrupted November 4 isolation build has an excluded artifact-recovery
attempt: source files survive the Docker shutdown, but capture reads the earlier
`98462b13e` ELF. The native identity fence stops before dashboard changes; the
known binary is restored to original firmware with current settings and no
native writes. That artifact is quarantined. The passing isolation cohort uses
a rebuilt image whose actual ELF API project version and build timestamp are
verified against the manifest. Wrong-label and wrong-timestamp negative checks
reject the quarantined image. No measurements from it are credited to `705b3ae8a`.

The durable-format follow-up adds a bounded lossless v1 fallback for flat
documents whose preferred v2 form exceeds the backend limit. Host checks cover
40 KiB and 16 KiB slots, retained/inactive records, opaque settings, raw invalid
escape spelling, graph IDs, import flags and discarded-dictionary rejection.
The exact historical 400-card fixture also selects v1 from either input format:
25,087 saved bytes instead of its 53,407-byte v2 form, within 40,932 bytes, with
an exact canonical v2 round trip. HTTP checks retain CAS and old-source fences,
allow a complete graph writer's deliberate flat replacement, and reload the
selected v1 document. Real legacy bindings also import, edit and reboot at both
slot sizes. Browser checks preserve graph transport/CAS and exact actual-format
backups. The 10-inch future-flat-writer cohort above qualifies one physical
migration path; production v2 and depth-two writes remain disabled.

All nine factory profiles pass at `a0a4988b2` on ESPHome 2026.10.2, including
the GIF admission/resource fix; browser-only follow-ups retain all component
and device-configuration bytes. Earlier `efc6dcf09` builds include bounded
canvases and the S3 hidden-workspace port; `b1847f0bb` retains its canvas-only scope and the earlier
`c05fde64f` matrix retains its own source scope. The `39a159354` and `43a914249` checkpoints also pass
all nine profiles with ESPHome 2026.10.1. Historical matrices also pass at `ac0fad1ff` with ESPHome
2026.10.1 and at private `c61cda45b` with ESPHome 2026.10.2 (base `c28c5b43f`
plus the SDK pin). The rebase onto main `6c10c3d5a` preserves the parent runtime
branch and every tested firmware/browser source; its three changed files only
add main's host flashing guard and guidance. CI also passes at `36b5b9cd0` and
`c28c5b43f`, including the audited SDK guard and the checks selected for each change.
Compilation does not establish physical layout parity or qualify the full
feature on ESPHome 2026.10.2. The latest-results section records the subsequent
S3 resource/callback cohort; historical matrices retain their exact source scope.

## Editor transfer follow-up (5 November)

Host regressions reproduce three review findings: a full-grid duplicate's local
planning failure blocks deletion, a snapshot or backup read revokes retry after
an explicit rejection, and a newly allocated high-ID card does not reserve its
span cells. Transfers now distinguish a pre-PUT planning error from an uncertain
acknowledgement and track safe retry against the write attempt rather than the
queue promise. Read-only operations retain the failed outcome without losing
eligibility; a later unrelated failed save cannot inherit that permission.
Lost acknowledgements remain blocked after reads.

Subpage grid construction now supplies its saved identity range separately from
visible cell capacity. Tall, wide and large cards above the visible ID count
reserve their covered cells on all four grids, so a subsequent paste or deletion
cannot hide an unrelated card. Home retains its renderable identity limit.

All 290 web unit tests, TypeScript, fresh web assets, generated-output checks,
web smoke, asset manifest and three backup contract/API checks pass. Two negative
controls remove local-failure retry eligibility and read-stable eligibility;
both fail the corresponding behavioral regression. These browser changes have
not been flashed for physical testing and do not enable production nested writes.
The memory and device results above remain scoped to their recorded sources.

## Current 10-inch recovery and cancellation (5 November)

Read-only discovery finds the 10-inch V3 back on official `v2.13.0`, rather
than the cached SDK2 development image. Its dedicated HTTP endpoint confirms
project/model/version, and direct ESPHome mDNS confirms native MAC
`80:F1:B2:D5:C9:16`, SDK `2026.9.1` and Noise encryption. All three published
OTA/factory/manifest files match GitHub's SHA-256 digests. The factory's app at
`0x20000` equals the OTA image; esptool validates its checksum/hash, P4 chip and
IDF `5.5.5` metadata. This verifies a pinned published-release recovery target;
it does not claim a native-API compilation timestamp or read the encryption key.

A fresh read-only snapshot retains the exact original 1,568-byte native-v1
dashboard at generation 375, reset/PIN state, 77 available protected preferences
and runtime backlight. The newer camera-dimming entity returns 404 on this
original, is excluded explicitly and receives no commands.

A separate controller uses those current records, not the obsolete SDK2 owner.
Twenty identity/source checks, 15 physical-evidence/reconciliation checks and
three update-intent/lost-acknowledgement checks pass on the host. It rejects wrong
MAC/model/version/SDK/encryption, does not repeat uncertain cleanup commands,
and never credits a network disconnect or software reset as a physical cut.
Its read-only preflight also passes. The prepared private storage image
`3f9ba3881` retains all six production storage files byte-identical to public
`b0f57fee0`; its earlier host results keep their recorded scope.

The actual firmware round trip then passes cancellation before and after
publication, using the initially erased isolated 90,112-byte scratch window.
Both sessions inspect the expected retained selection/generation, clean the
owned window and independently reproduce the complete 2 MiB image partition's
original SHA-256. The controller restores the verified official OTA, exact
native dashboard at generation 375, all 77 available preferences, PIN/reset and
both update settings. Direct discovery again confirms the original SDK/project/
MAC/encryption mode; over 76 seconds of original-firmware stability pass.
The controller neither reads nor writes the encryption key. These are cancellation and
recovery checks, not actual power-interruption evidence. No test is left armed.

## Completed GIF results across sleep and wake (5 November)

Private `0c3e308b1` adds three maximum-load display-off/wake cases. Its runtime
components, S3 clock/buffering and decoder remain those of public `d183c6898`;
current `cda1aea47` has the same component/device/SDK bytes. Each run audits all
1,406 routes and fills the complete 128 KiB cache for 81 cards, 70 climates,
two images and nine folders. Only bounded 64-byte descriptor samples are read;
these checks do not establish panel pixels, isolated heap peaks or a memory pair.

Two actual successful, non-cached 240 × 240 GIF completions are held at the
adapter/runtime boundary while their modal is current. Display-off retires all
image leases and modal contexts. Releasing one result while asleep does not
reactivate them; releasing the other after wake and a fresh PNG opening leaves
its request, pending/ready/failed state, asset, revision and sampled descriptor
unchanged. New images complete and the independent PNG remains available.

The third case refreshes a tile with a real GIF completion held, then retires
that tile through display-off and wakes onto a new PNG. Releasing the old result
again preserves the replacement's ownership/state and sampled descriptor. The
first cohort stops at an incorrect capture assertion: the shared modal validator
requires `ready == 0`, while the tile correctly retains completed asset 12 during
pending request 13. Production `request()` explicitly preserves that prior
picture. Its two preceding modal cases pass; the cohort remains recorded as
failed, followed by complete original recovery at generation 1205.

A tile-only retry requires the exact prior ready asset, the current pending
cookie, successful non-cached decode and an intact callback target. Ten host
checks use the actual recorded capture, reject invalid states and verify the
unchanged recovery finalizer. That retry passes without a firmware code change.
The fixed public `d183c6898` firmware, exact original dashboard at generation
1207, all 78 preferences, PIN/reset and update settings are verified restored,
with over 70 seconds of stability. No callback target is dropped instead of
replayed. The initial report's copied eleven-case label is corrected with its
original bytes/hash preserved; accepted evidence is three distinct cases across
two runs. No test is left running.

## 480-pixel S3 visual confirmation and interrupted-controller recovery

On 5 November, the normal public `d183c6898` firmware displays the 25,825-byte
480 × 480 GIF full-screen after loading the 81-card fixture, auditing 1,406
routes and prefilling the complete 128 KiB state cache. Two completed-frame
notices and an actual expanded decode precede the window without image or
memory polling. The user explicitly confirms that the colours change while
the whole screen remains stable and correctly positioned, without jitter,
breakup or flicker. No firmware upload is performed for this check.

A local ENOSPC error terminates the host controller before cleanup; its partial
report and frozen source are preserved. The human result is saved separately.
This is not recorded as an uninterrupted automated-cohort pass or proof that
live cache values remained frozen throughout the window. Disposable package
cache and generated CMake intermediates are removed; source, executables,
fixtures and verified recovery images are retained.

A restore-only supervisor verifies the exact current firmware identity, owned
fixture bytes/generation and protected settings before writing. It reuses the
controller's native/legacy CAS and preference recovery without rerunning the
visual test or uploading firmware. An HTTP 204 confirms complete native and
legacy restoration. The exact original dashboard at generation 1209, all 78
settings, PIN/reset and update preferences, and over 70 seconds of fixed-firmware
stability are verified. The manual appearance gate passes; the original host
run retains its interrupted status and separate successful recovery evidence.

## First manual 10-inch power attempt

The first operator attempt reaches the held before-publication boundary in
session `2126636821`. The user reports disconnecting and reconnecting power,
but the target does not restart: an independent read-only observer verifies
the expected native MAC and test firmware, a still-waiting phase, software-reset
reason 3 and continuous uptime of 606,962 ms, versus 70,986 ms at the ready
boundary. No transport disconnect or fresh power-on evidence is recorded, so
no physical power-cut pass is credited.

The controller times out, cancels the held write, cleans its owned region and
reproduces the complete partition checksum. Verified official `v2.13.0`, exact
native dashboard at generation 375, all 77 available preferences, PIN/reset and
both update settings are restored. The test is no longer armed. Identifying the
actual target power feed is required before a new attempt.

Two later before-publication attempts also record no controller restart. The
operator identifies the screen while it cycles between its existing Home and
Devices pages; navigation does not modify the dashboard. The second attempt
expires without a reset, and the later identification reply cannot establish a
cut within that checkpoint.

For the third attempt, the operator cue starts a fresh 15-minute observation
window after the storage boundary is ready. Nine host checks cover cue ownership,
cancellation and connection-loss handling; 15 existing physical-evidence guards
still pass. Storage, identity and recovery routines remain unchanged. The user
explicitly confirms a new five-second disconnection. Active API requests keep
succeeding, however, and an independent observer still finds the held worker,
software-reset reason 3 and 614,693 ms uptime, versus 70,844 ms at readiness.
The observation therefore does not establish loss of controller power. Another
power feed or disconnection of only the display remains a possibility, not a
verified diagnosis.

Cooperative cancellation cleans the owned region and independently reproduces
the full partition checksum. Official `v2.13.0`, the exact generation-375
dashboard, all 77 preferences, PIN/reset and update settings are restored; the
final restoration audit spans over 82 seconds. All three failed observations
and human receipts are retained separately. No physical cut is credited and no
test remains armed. Identify the external power connections before another
firmware replacement or cut request.

## Encrypted 10-inch qualification transport (6 November)

The fresh 15-minute before-publication attempt records no disconnect before its
deadline and restores official firmware and the current settings. The later
human confirmation arrives after cleanup. A fresh read then finds only 7.573
seconds of uptime on the correct controller: it restarted after the held
checkpoint had ended. This does not qualify that storage interruption. A new
30-minute attempt is cooperatively cancelled after the user reports repeated
Home Assistant encryption approvals.

The earlier qualifiers use a plaintext API while official `v2.13.0` uses
encryption. The replacement private qualifier retains `api: encryption: {}`
and configures no replacement key. Stock ESPHome 2026.9.1 and 2026.10.2 use
the same saved-key preference identifier. The test client uses a bounded HTTP
mailbox instead of retrieving the native API key. Commands execute on the main
loop, have monotonically reserved nonces and return their exact result once.
Encrypted API state, full MAC, SDK and actual build time are checked separately;
HTTP failures and changed boot identities remain subject to the existing
power-on, ownership, payload-selection and full-partition checksum guards.

Fifteen actual-client checks, nine encrypted-identity checks, 15 physical-evidence
guards and the actual C++ mailbox under ASan/UBSan pass. Six production storage
files and the raw storage core remain unchanged. Reversing only the private
worker's result-reporting edits reproduces the original worker byte-for-byte.

The first encrypted prototype boots with encryption active but its commands
are rejected by the webserver's epoch fence. Read-only diagnosis also exposes
an unsupported response-code mapping and a result-capture dependency on logging.
No storage session or START command is armed. Its failed report is retained;
a separate restore-only supervisor verifies official firmware, the exact
dashboard and all current settings. That recovery does not claim a scratch
cleanup checksum or power-interruption pass.

Corrected private source `66f2315df` passes pinned SDK, source, partition and ELF
checks. Its HTTP requests include the epoch fence, use explicit response codes
and receive bounded results directly from the worker, independently of logging.
Both real cancellation cases pass with API encryption active: before publication
selects generation 2 from the old slot; after publication selects generation 3
from the new slot. Each independently reproduces the complete original partition
checksum after cleanup. Official `v2.13.0`, the exact generation-375 dashboard,
all 77 current preferences, PIN/reset and both update settings are restored and
stable for over 76 seconds. The refreshed snapshot preserves the user's newer
screensaver settings. No API key is retrieved or exported, and no test is left
armed. Home Assistant's absence of further approval prompts awaits user feedback;
actual physical cuts are not credited by this cancellation cohort. The later
automated reset method below supersedes the manual-cut workflow.

## Automated abrupt-reset recovery (6 November)

At the user's request, the transaction recovery test now runs without
disconnecting power. Private source `f8bb317ccc63`, based on public `a9cddad09`,
and frozen controller `80d76b435d2b8af1` pass both checkpoints on the actual
10-inch P4 V3. The image builds with ESPHome 2026.10.2 and ESP-IDF 5.5.6;
source, partition, SDK, artifact and ELF checks pass. The six production storage
files and two private raw-partition/session cores retain their qualified bytes.
Test endpoints and the reset trigger remain private qualification code.

The worker holds mutation seven either immediately before or after publication
in a guarded, initially erased 90,112-byte scratch window within `card_images`.
A command fenced by nonce, session, boot identity, native generation, reset
epoch and checkpoint requests `esp_restart_noos()` through the existing P4
IRAM/cache-reset workaround. It bypasses application shutdown handlers. A
44-byte RTC-retained witness ties the request to the next boot and records any
shutdown-handler calls. The controller requires a fresh boot, software-reset
reason 3, a matching witness with zero shutdown calls and a newly initialized
worker before inspecting storage. It never infers a reset from lost transport
alone or retries a reset command blindly.

| Held checkpoint | Recovered test payload | Shutdown-handler calls | Result |
| --- | --- | --- | --- |
| Before publication | Old slot, generation 2 | 0 | Pass |
| After publication | New slot, generation 3 | 0 | Pass |

Both reset requests receive HTTP 202 and their exact accepted acknowledgements.
All boot identities exceed the signed 32-bit range, exercising the corrected
full-uint32 parser on hardware. Both cleanups independently reproduce the
original full 2 MiB partition SHA-256
`cdedcbd47d5c8199fb8669aaa2ecc8cd6dcecb3c59483f03697cc257ff2807ce`.
The final report, frozen source/controller/artifact digests, retained witnesses,
old/new payloads and recovery are independently audited. Scoped host coverage
passes 56 C++ ASan/UBSan checks with three weakened-policy negative controls,
68 controller checks, 17 actual HTTP-client checks and 11 identity checks.

Earlier cohorts remain separate. The first completes both resets and recovery,
but its client lacks the new reset-result decoder and reconciles the resulting
reporting error using the retained witness. The second refuses a boot identity
above the signed 32-bit range and times out without an accepted reset; it
cleans up and restores the original successfully. Neither is relabelled as a
clean final-controller run. The final cohort fixes the decoder, accepts the
full unsigned boot range and treats explicit HTTP rejection as a terminal
failure rather than waiting for or repeating a reset.

After the final cohort, verified official `v2.13.0`, the exact generation-375
native dashboard, all 77 protected preferences, PIN/reset and both automatic
update settings are restored. The restored identity and dashboard remain
verified for over 75 seconds, followed by another read-only identity/data/settings
preflight. Dynamic API encryption preserves the saved key without reading,
exporting or replacing it. No test is left armed.

This qualifies transaction crash recovery across abrupt low-level software
resets at both publication boundaries on the recorded device and storage
source. It does not qualify electrical power loss, brownouts or interruption
of a flash operation by collapsing supply voltage. RTC witness memory is
deliberately retained; the result does not claim that every RAM region loses
its contents. Electrical testing remains a separate scope, without blocking
this automated replacement on operator availability.

## Parent-review carry-forward audit (6 November)

An audit of eight earlier, non-ancestor PR heads finds omitted runtime fixes.
Runtime `7acf85afd` restores guards for controls enabled by delayed state,
prevents reentrant retirement from releasing an in-use layout, and clears
retired climate/light/cover titles during safe collection. Each restored
regression fails before its fix. Real-LVGL ASan/UBSan checks then pass 1,000
gesture cycles, pattern/navigation coverage on nine profiles, all 13 modal
families and the focused renderer/title cases. Feature `4829b77c3` integrates
these changes and passes its focused guard and three retirement checks.

The runtime's old hard-coded SDK stub also hides a 2026.10.0 transport guard
against the repository's 2026.10.2 pin. A direct current-pin compile reproduces
that mismatch. Host tests now derive the header from `.github/esphome.env`;
audited 2026.10.1/2 checks pass and unaudited 2026.10.3 fails compilation. The
feature retains its additional native-dispatch guard checks. These are host
and source checks, with no new device flash, factory matrix or memory result.

Runtime `c8317a8e6` restores configured/friendly captions for sensorless toggle
cards in text mode and translation-aware default local-action labels. Both
regressions fail before the fix. The final definition keeps its existing
264-byte host size; real-LVGL status/action coverage passes 1,000 cycles on
nine profiles under ASan/UBSan.

Runtime `c3aac55e4` sizes dependency storage from unique channels and card links,
including repeated declarations. Counting scratch is freed before the final
arena allocation. All 64 preparation allocation failures are exercised, four
weakened implementations fail their negative controls, and three state/consumer
checks pass. Matched synthetic host benchmarks retain the same requested arena
peaks for unique graphs; shared workloads require less storage. These are
allocator-request measurements, not physical heap results. Feature `712bcbb76`
integrates this change and passes four state/consumer checks.

Runtime `120155057` also preserves exhausted shared-channel state across a
same-connection replacement, while allowing fresh data to recover it and
reconnects to invalidate it. Its regression fails before the fix; three focused
checks pass afterward. Feature integration additionally requires a fresh global
change stamp: a regression fails with the runtime's local stamp and passes
with the feature's monotonic stamp. All four focused feature state/consumer
checks pass under ASan/UBSan.

Runtime `6a88bb6d4` reconnects only authenticated HA sessions after native
registration changes, allowing the peer to remove old listeners. Runtime
`49a6278b3` changes existing-value refresh to coalesced one-shot requests, paced
at one pair per loop tick. Six feature transport/index checks pass; three
weakened refresh implementations fail their negative controls. Actual pinned
upstream HA method bodies with event/state doubles confirm that one-shot reads
do not add listeners. This remains source/host evidence for HA, rather than a
claim about the user's installed HA integration.

Runtime `1406c2a4d` restores input-source dependencies for ordinary media
controls and speaker groups. Its restored declaration regression fails before
the fix; dependency, media-state and real-LVGL modal checks pass afterward.
The older fan-speed and alarm-name fixes are already represented by the current
implementation. The actual core binding test passes under GCC with the existing
224-byte slot; no host or firmware slot increase is needed.

Cumulative feature source `656fc078c` then passes all **289 Linux/GCC checks**
with real LVGL, including nested-folder and full port coverage. The first build
fails on ambiguous statement formatting, a missing `<cstddef>` include and a
C++-only flag applied to C; runtime `84ee476c1` corrects these without changing
firmware behaviour. Its focused ASan/UBSan checks and nine-profile, 1,000-cycle
status/action checks also pass. The failed first cohort remains separate from
the passing run, whose 1,004 tested files match the committed source.

All **nine factory profiles** compile sequentially with ESPHome 2026.10.2 on
that same source. Every profile passes compiled-source, stock API, target MCU,
ELF handler/wrapper and production depth-one writer checks. The later
`8478b46fd` CI-only correction replaces a stale source guard requiring live
subscription replay with a paced-refresh guard; scan and rejecting self-tests
pass. Integration leaves all tested firmware sources unchanged. These build
results do not extend earlier physical appearance or memory evidence to the
new source. Physical scopes beyond the S3 wire test below remain outstanding.

A fresh read-only S3 preflight finds the newer Card Images build from its
separate task, compiled 6 November at 18:30:40. Its exact retained OTA/ELF and
partition files are verified; current generation-1227 dashboard, 78 protected
settings, PIN/reset state and five uploaded JPEGs are preserved for recovery.
The older S3 recovery is historical. The current dashboard also passes actual
PageStore/dependency staging and fixture-to-original legacy mirroring/native
merge under ASan/UBSan, preserving all 14 original records and opaque image
options. Those recovery-preparation checks are read-only; the subsequent S3
wire test is recorded below. Stable reader/compatibility release gates remain
unchanged.

## Cumulative S3 HA wire regression (7 November)

The YAML-only `runtime-ha-656fc078` image, compiled 6 November at 23:50:58,
uses public `656fc078c` C++ with 412 compiled component files verified. SDK,
stock API, ELF identity, unchanged partition table, unencrypted API and normal
6 MHz display checks pass. It adds a project label and asset paths, with no
private actions, state injection or raw-partition hooks.

On the actual S3, a surrogate HA peer and a separate diagnostic client pass
50 cycles and 100 public v1 configuration saves. Four disjoint fixtures each
reserve 40 synthetic HA entity/attribute pairs. Changed registrations reconnect
HA; saving the same channels requests current values once without reconnecting
that session. Every cycle retains exactly one persistent listener per test pair,
with zero duplicate persistent requests and zero diagnostic disconnects. The
peer supplies 6,000 state replies across 51 sessions, including 50 intended
reconnects. Public climate-modal open/close commands also complete. The peer
never supplies invented values for the user's real entities.

The final report, all 50 cycle records, frozen controller/fixture hashes,
compiled artifacts and existing recovery files pass an independent audit.
Earlier private-controller attempts remain failed: one omits a protected-write
epoch and makes no accepted change; another misclassifies successful HTTP 204
saves. Their independent recovery checks pass. Corrected epoch/no-op/readback,
normal-204 handling, external-configuration fences and recovery fallbacks are
host-tested before the passing cohort. No firmware fix is inferred from those
controller failures.

Afterward, the current Card Images firmware compiled at 18:30:40, exact original
dashboard bytes, all 78 protected settings, PIN/reset state and five JPEGs are
verified restored. The update switch is restored too. Public transactional
writes advance the generation from 1230 to 1331; they do not change the restored
payload. A final diagnostic connection stays up for 65 seconds without a
reboot/disconnect. The older generation-1227 snapshot and failed-cohort records
remain separate.

This qualifies the recorded native API wire behaviour and configuration round
trip on S3. It does not qualify a full Home Assistant server instance, physical
appearance or touch actions, maximum-pressure memory, electrical interruption,
or another device. The optional visual response is still pending; earlier
appearance and memory evidence retains its original source scope.

## Matched S3 maximum-state preparation (7 November)

The earlier audit baseline `a51486f7f` and current firmware source `656fc078c`
pass the same real-LVGL S3-profile host workload: 81 cards, ten pages, 1,406
unique HA channels and exactly 131,072 UTF-8 value bytes. The A/A/B/B sequence
covers same-channel replay, disjoint replacement, rejection of retired
callbacks and 36 folder round trips. Navigation succeeds with the backing
arena disabled and makes no new arena allocation calls.

Both versions retain 286,144 requested arena bytes in seven allocations.
Requested peak arena memory is 351,680 bytes on first activation and 633,800
bytes after replacement. Metadata remains 107,782 bytes. The current unique
counting implementation makes more temporary allocation calls: 70 for first
activation and 69 per replacement, compared with 15 and 14 before. This
all-unique workload shows no retained or peak arena saving from the audit
fixes. It does not measure fragmentation or prove an S3 free-heap result.

These host counters exclude allocator overhead, LVGL objects, the enclosing
device, SDK connections, framebuffers and image decode/download buffers.
Matched YAML-only S3 observers are being prepared to report byte-addressable
internal RAM and PSRAM totals, free bytes and largest blocks separately, with
samples taken while pages are held still. This compares the audit fixes;
it is not a new whole-overhaul comparison with official v2.13.0. Physical
maximum-state and current-source artwork-pressure results remain pending.

The paced surrogate-HA sender passes a 1,406-channel/128 KiB check with native
round-trip barriers every 16 frames, exclusion of user entities, stale-epoch
rejection and distinct once/persistent handling. The real legacy adapter also
preserves all 14 original dashboard records, including opaque image options,
after either maximum fixture is mirrored and the original is restored.
These are host checks; no device has been flashed for this comparison yet.

## S3 maximum-state run and separate cleanup (7 November)

The before observer on `a51486f7f` completes all four A/A/B/B passes, 36 folder
round trips and 28 held-page samples at 1,406 channels and the full 128 KiB
value cache, with no allocation/accounting failures or diagnostic disconnects.
Its historical refresh path sends 5,624 persistent requests, with two requests
per pair after same-channel replacement and no one-shot refreshes.

The original Card Images firmware and dashboard are uploaded/restored, but the
controller then fails an immediate preference readback during settings cleanup.
This full controller run remains `recovery_failed`; completed measurements are
not relabelled as an uninterrupted pass. A separate settings-only recovery
waits for publication without repeating the accepted write, verifies all 78
protected settings, PIN/reset and five JPEGs, and holds a diagnostic connection
for 65.2 seconds. A fresh independent read-only preflight passes afterward.
The exact original dashboard bytes remain unchanged at generation 1336.

A regression reproduces the old controller's premature readback failure and
checks the corrected bounded polling for number and switch preferences,
including no-op writes and failure-path recovery. The next after observer uses
the same firmware probes, SDK, fixture and pressure case. Only the host
controller's preference acknowledgement handling changes. A matched physical
comparison and current-source artwork pressure remain pending.

## Matched physical S3 maximum-state comparison (7 November)

Earlier runtime `a51486f7f` and current firmware source `656fc078c` use the same
ESPHome 2026.10.2 configuration, partition table, 6 MHz display clock, YAML
memory probe and pressure case. Each completes A/A/B/B replacement, 36 folder
round trips and 28 held-page samples with 81 cards, 1,406 unique channels and
exactly 131,072 cached UTF-8 value bytes. The table gives median bytes from
three Home samples, three seconds apart, after each feed and a five-second
pause. These are matched operation phases, not long-term idle measurements.

| Phase | Internal free before | Internal free after | PSRAM free before | PSRAM free after | Combined free change |
| --- | ---: | ---: | ---: | ---: | ---: |
| First A | 80,899 | 80,867 | 1,642,832 | 1,642,576 | −288 |
| Same A again | 80,999 | 81,075 | 1,655,068 | 1,655,048 | +56 |
| Disjoint B | 80,351 | 80,323 | 1,732,980 | 1,732,636 | −372 |
| Same B again | 80,239 | 80,419 | 1,732,076 | 1,732,352 | +456 |

Combined free means byte-addressable internal RAM plus byte-addressable
PSRAM. All four changes are below 0.5 KiB; the device arena remains 275,724
requested bytes in seven allocations in both builds. Both report zero arena
allocation/accounting failures. The largest internal block stays 31,744 bytes;
the largest PSRAM block changes by +64, 0, −32 and +16 KiB respectively, so
this does not establish a uniform contiguous-block improvement.

Managed internal heap capacity is 290,163 → 290,155 bytes and managed PSRAM
capacity is 3,201,392 → 3,201,376 bytes. These heap-pool capacities exclude
memory reserved before heap registration, including code/read-only data copied
to PSRAM; they are not the panel's physical RAM specification. ELF accounting
separately shows +8 bytes of static DRAM, +1,908 bytes of flash text and +16
bytes of flash read-only data. It must not be added to live free memory.

The all-unique graph and full cache have no duplicate channels for the unique
counting fix to remove, explaining the unchanged requested arena. Both arms
already contain the earlier pooled runtime. This pair measures the later audit
fixes, not the full overhaul against official v2.13.0. The after firmware has
one persistent listener per pair, zero duplicate persistent requests, two
intended HA reconnects for changed lists and zero diagnostic disconnects.
Same-channel replacement uses one-shot refresh. Empty image URLs deliberately
exclude decoder/download pressure; no physical-pixel or touch result is claimed.

The before controller's cleanup/readback failure and separately verified
recovery remain recorded above. The after controller completes normally,
including the corrected accepted-write polling. Its final Auto Update restore
requires two readbacks, exercising that fix on hardware. Original Card Images
firmware, exact dashboard bytes, all 78 protected settings, PIN/reset and five
JPEGs are restored, with 65.2 seconds of connection stability and a fresh
independent read-only preflight. Only native generation advances to 1341.
No test firmware or background device process remains active.

## Current-source S3 image pressure investigation (7 November)

The current observer compiles unmodified production C++ from `656fc078c` with
YAML-only owner memory, IDF local minima and descriptor liveness probes. It
verifies 439 compiled C++ files, the stock API, native wrapper, unchanged
partition table, depth-one/v1 writer policy and 6 MHz clock. The sampler reads
at most 64 descriptor bytes, with a sanitizer check covering empty, short and
large surfaces. It does not read panel pixels or full frames. Forty-four
image/decoder/RGB/owner leaf files also match the earlier qualified `d183c6898`;
changed shared-state/renderer callers still need integration evidence.

The new public-API/native-HA case retains 81 cards, 1,406 channels and the full
128 KiB value cache. The initial run completes both PNG controls, the
369,953-byte 480-pixel GIF as a card, bounded expanded RESOURCE refusal and
retained-card resumption without download. It then stops because replacing the
playing GIF has no fresh successful decode. It is `failed_restored`, not a
completed image qualification. A separate revised controller records explicit
playback or RESOURCE admission and retains filtered decode events and views.

One controller retry reuses the initial image-state revision for its first
update and correctly causes no refresh. A regression against the actual
`PageImageRefreshState` confirms that repeated image states are ignored and a
new revision makes the tile dirty. The corrected controller seeds a finite
fixed-length revision counter from the actual initial HA value. Host tests also
cover result completeness, cleanup failures, source/success freshness and the
settings publication delay previously found during memory qualification.

With distinct revisions, the current binary completes six cases: both PNGs,
the larger GIF card, expanded refusal, retained-card resumption, direct
replacement admission and the smaller 25,825-byte 480-pixel GIF as a card.
Direct replacement explicitly reports RESOURCE while the previous animation
continues changing. At that point free PSRAM is 645,792 bytes but its largest
block is 286,720 bytes; another 480-pixel RGB565 canvas requires 460,800
contiguous bytes. A successful static PNG resets the old canvas, returning free
PSRAM to 1,549,456 bytes. The subsequent smaller GIF card plays with fresh
source/success logs and three changing descriptor samples.

Its expanded view repeatedly reports `GIF playback cannot reserve staging and
PSRAM headroom` for the 470 × 436 target. The strict fresh-success gate fails;
the mandatory 240-pixel, 640-pixel source, replay/replacement and unpolled visual
checks later in this sequence therefore remain unfinished. Do not substitute
an old descriptor or the earlier source's positive result for this failed gate.
The earlier pressure controller allowed a bounded 480-pixel expanded refusal,
although playback was actually observed; its mandatory 240-pixel control did
require playback. Fresh versus stressed capacity must be distinguished before
calling the current result a firmware regression or accepting a fitting control.

All saved display preferences match the earlier source. Relative to its
retained ELF, the current observer adds 16 static DRAM data bytes, 152 BSS bytes,
6,832 flash-text bytes and 2,288 read-only bytes. These section sizes are not
live heap measurements and do not prove the cause. A fresh-boot control is the
next check for baseline capacity versus heap fragmentation after the stress
sequence; no resource guard or image-quality requirement has been weakened.

All three stopped cohorts retain separate records and verified original
recovery. The latest restores current Card Images firmware, exact original
dashboard bytes at generation 1347, all 78 protected settings, PIN/reset and
five JPEGs, followed by 65.2 seconds of connection stability. No device test is
left running. Production firmware sources and the release policy are unchanged.

## Fresh 480-pixel control and fragmentation comparison (7 November)

A separate focused control boots the same verified `656fc078c` observer and
loads the same 81-card, 1,406-channel, full-128-KiB workload. It omits only the
preceding large-GIF/failed-replacement stress. Both PNG controls, the smaller
480-pixel GIF as a card, its expanded playback and retained-card resumption
pass, with fresh source/success logs and at least three changing descriptor
samples per animation. The expanded target remains 470 × 436, with a
409,840-byte surface. Closing it makes no additional image download.

Immediately before expansion, the paired card observations are:

| Observation | After stress | Fresh boot | Difference |
| --- | ---: | ---: | ---: |
| Internal heap free | 69,739 B | 69,751 B | +12 B |
| PSRAM free | 996,464 B | 996,784 B | +320 B |
| Largest PSRAM block | 655,360 B | 851,968 B | +196,608 B |
| Requested model arena | 275,724 B | 275,724 B | 0 B |

This supports heap fragmentation as the explanation for the stressed staging
refusal, rather than a loss of aggregate capacity for the 480-pixel image.
It does not isolate the allocator operation responsible or guarantee that
other heap histories will fit. No image-size, cache or admission guard changes
are made. The earlier full stress sequence remains a failed cohort; this
four-case diagnostic is not substituted for its remaining checks.

During expanded playback, sampled free internal RAM is 48,635 bytes and PSRAM
174,152 bytes. The local interval minima are 46,099 internal bytes and 117,528
PSRAM bytes; they are independent minima and must not be added. Requested
arena memory is 275,724 bytes in seven allocations when closed and 276,268 in
nine while its single expanded context is open, returning to the closed value.
All four observations retain the exact channel/cache totals with zero arena
allocation/accounting errors and zero diagnostic disconnects.

The test includes a 90-second window without descriptor or memory probes.
The optional user pixel response remains pending; changing descriptors do not
qualify the panel's physical appearance. The original Card Images firmware,
exact dashboard bytes, all 78 protected settings, PIN/reset and five JPEGs are
restored at generation 1349, verified stable for 65.2 seconds and checked again
read-only. No device test is left running.

Remaining current-source image work is the mandatory 240-pixel fitting control,
two larger-source refusal/recovery cases, same-configuration replay and in-flight
disjoint replacement after the expanded pressure outcomes are classified.
Retain the original distinction: the 480-pixel stress case permits explicit
RESOURCE refusal; a separate fitting control must actually play. Old physical
results and unchanged leaf-source proofs retain their recorded scope.

## Remaining S3 image integration checks completed (7 November)

The seven remaining current-source checks pass as a separate cohort on the
same `656fc078c` C++ and verified YAML observer. It retains 81 cards, 1,406
channels and all 131,072 cached value bytes throughout nine held-page memory
observations, with four independently stopped local-minimum intervals.
The already-recorded 480-pixel cases are not repeated or relabelled.

- The required 6,897-byte 240-pixel GIF plays as a card and expanded image,
  with fresh source/success logs and three changing descriptor samples.
- Both 2 MiB 640-pixel disposal sources report bounded RESOURCE refusal. The
  previous PNG and independent PNG retain their sampled descriptors; a fresh
  replacement PNG completes after each refusal.
- The 240-pixel card and expanded fitting control play again after both
  refusals. Both expanded closes resume the retained card without a download.
- A same-configuration save replays the full state cache and resumes animated
  playback without reconnecting HA or adding persistent listeners.
- A transfer is held after its first 2,048 bytes, then the disjoint B
  configuration commits before the gate is released. Its new PNG remains
  correct through three subsequent observations; the retired source does not
  replace it. This is an in-flight cancellation test, not a held completed
  callback.

The final native-HA peer has one persistent listener per pair, two intended
reconnects for changed lists, zero duplicate persistent requests and zero
unexpected diagnostic disconnects. Model memory returns to 275,724 requested
bytes in seven allocations with the expanded context closed; while open it is
276,268 bytes in nine allocations. There are zero arena allocation/accounting
errors, no retired modal contexts in the settled observations and no model
reservation growth through replay/replacement.

The two fitting-control intervals have internal minima of 46,235 and 46,387
bytes and PSRAM minima of 506,080 and 506,096 bytes. These are independent
per-heap observations, not simultaneous combined minima or isolated decoder
measurements. Both larger-source attempts stop cleanly and recover.

Original Card Images firmware, exact original dashboard bytes, all 78 protected
settings, PIN/reset and five JPEGs are restored at generation 1353. The final
diagnostic connection remains stable for 65.2 seconds, followed by a fresh
independent read-only verification. The source/artifact/result audit passes;
no test firmware, HTTP asset server or background device process remains active.

This completes the listed remaining automated image cases alongside the
separate successful fresh 480-pixel control and recorded 480-pixel pressure
outcomes. Earlier stopped full sequences retain their failed status. It does
not add physical-pixel/touch confirmation, a precise current completed-callback
hold, electrical recovery, another panel's qualification or a published reader
floor. The optional user response to the unpolled 480-pixel window remains
pending; production depth-two writes are still disabled by release policy.

## P4 source and recovery-baseline audit (7 November)

A fresh read-only HTTP/mDNS inventory finds all four P4s online with the
expected hardware profiles. The 4.3-inch and replacement 7-inch V2 run
development builds on ESPHome 2026.10.2. The 10-inch V3 and P4-86 run published
`v2.13.0` on ESPHome 2026.9.1 with API encryption advertised. No firmware,
configuration, preference or encryption changes are made by this inventory.

The 4.3-inch build time, native bytes and generation 147 match its retained
recovery baseline. The 7-inch V2 instead reports compilation
`2026-11-06 16:07:39 +0000` and generation 10: its saved generation-seven
recovery baseline is stale. Capture and verify the current 7-inch firmware,
dashboard and settings before another temporary test; do not restore the old
development image over that newer build. The P4-86 is identified at its expected
address, but this observation adds no candidate runtime qualification.

The 10-inch V3 passes the complete read-only recovery preflight again: exact
generation-375 native bytes, 77 protected settings, PIN/reset state and retained
published recovery-artifact hashes. Its existing encrypted API remains in use;
no key is read or changed. Runtime backlight output is reported separately from
the protected brightness configuration. Camera Dimming remains unobservable
on that original firmware and is untouched.

Historical P4 results retain their named source scopes. Comparing compiled
sources from the 20-slot mixed-image cohort against `656fc078c` finds 424 of
457 component files unchanged; the 6-slot native-lookup cohort retains 410 of
448. Both historical compiled-source maps match their stated public bases.
Changed shared-state, transport, renderer/modal and image code prevent treating
either cohort as a current-build physical pass. In particular, current decoder
resource classification, animation ownership and resize-workspace reuse are
separate from the older image measurements; S3-only allocation branches do not
describe P4 behaviour. The earlier passing device results are preserved rather
than replaced by source-equivalence claims.

The next physical work is a focused current-source 20-slot P4 integration
cohort covering preparation/replacement, navigation and image memory, using
the verified current recovery baseline. Keep its encryption key and retain
the completed S3 evidence. Remaining grid checks must use current recovery
baselines and target changed integration paths rather than repeat every
historical case. Production nesting and the published compatibility-release
sequence remain unfinished.

## Current P4 test image prepared (7 November)

The current-source 20-slot P4 qualification image is compiled and independently
verified, but **not flashed**. All 440 copied component source files match
`656fc078c`, including the selected repository MIPI driver. The API remains
byte-identical to the pinned SDK, with dynamic encryption enabled and no YAML
key. Its partition table matches the already qualified recovery image, and
its minimum silicon revision is the same 3.1 used by the earlier successful
encrypted P4 test. The actual RISC-V ELF contains the selected 20-slot scene
and no other grid implementation. API version/build identity checks reject
wrong labels and timestamps; the optimized API response's version stores are
checked against its actual instructions, rodata and DWARF field layout.

The private HTTP bridge schedules commands on the main loop, checks reset epoch
and scene generation, and rejects replayed requests. It feeds only the known
synthetic pairs through the production prepared lookup and subscriber callbacks;
it bypasses wire decoding and authentication and cannot establish transport
qualification. Separate page-position acknowledgements avoid heap scans and
descriptor sampling during navigation. Held-page snapshots and IDF local minima
provide the memory observations. Image liveness samples retain the tested
64-byte bound.

Sanitized host checks verify both 7,566-channel plans, exact 512 KiB value totals,
pair uniqueness, canonical names and overflow/boundary rejection. Four distinct
native-v1 fixtures retain the A/A/B/B dependency sequence while changing an
opaque revision field, so same-list cases request actual replacements. Device
admission, memory, latency and image results remain pending. The guarded
controller and automatic original restoration must be ready before flashing;
reuse the verified image rather than rebuild it without a source change.
Earlier checker failures are retained separately: they assumed a stock MIPI
driver, revision 3.0, or a named constant optimized away on RISC-V. They do not
represent device failures or qualify those prototype images.

## Current P4 pressure and navigation (7 November)

The verified `656fc078c` image now passes the physical 20-slot P4 pressure
cohort. Four marker-changing native-v1 publications exercise A/A/B/B dependency
sets: 400 cards, 21 pages, 7,566 channels and exactly 524,288 live value bytes.
Each publication receives HTTP 204 with its compatibility mirror acknowledged.
The private cooperative feed enters the production prepared lookup and state
callbacks; network framing/authentication and remote HA listener behaviour
retain their separate qualification scopes. Image URLs are empty throughout.

All 28 full-state snapshots retain 1,252,986 counted scene bytes in seven
allocations, 580,604 state-metadata bytes and 180 widgets. Allocation/accounting
errors and active/retired modal contexts remain zero. Native membership stays
at 7,581 registrations, including 15 foreign registrations; the lookup stays
at 60,648 bytes. Whole-device readings below are three held-Home sample medians,
in bytes, from this instrumented current build:

| Replacement | Internal free | PSRAM free | Combined free | Managed used |
| --- | ---: | ---: | ---: | ---: |
| A0 | 305,919 | 17,106,132 | 17,412,051 | 10,646,916 |
| A1 | 306,091 | 17,114,008 | 17,420,099 | 10,638,868 |
| B0 | 304,123 | 17,040,676 | 17,344,799 | 10,714,168 |
| B1 | 303,663 | 17,037,192 | 17,340,855 | 10,718,112 |

The managed pools total 28,058,967 bytes: 499,799 internal and 27,559,168 PSRAM.
Memory outside these allocator pools is excluded. Preparation/feed windows
reach independent per-region lows of 279,455 internal bytes and 10,561,300
PSRAM bytes; these minima must not be added. The model remains constant while
the whole-device free readings vary. This cohort measures steady state and
preparation pressure, without establishing a matched v2.13.0 memory difference
or an isolated image peak.

Remote navigation completes 80 folder round trips, with 160 actual page
transitions plus eight already-Home acknowledgements. Transition acknowledgement
time includes HTTP and controller work: median 781.8 ms, p95 943.3 ms, maximum
1,743.3 ms. Position acknowledgements avoid heap scans and image sampling while
pages change. These timings do not measure physical display-flush completion;
no new physical pixel or touch confirmation is claimed.

Cleanup makes two acknowledged exact-original native/legacy publications, then
restores published `v2.13.0`. Original native bytes, all 77 observable protected
settings, PIN/reset and update switches pass a fresh independent read-only
preflight at generation 381 after 74.97 seconds of stability. API encryption
stays enabled; the controller never reads or replaces the saved key. The earlier
HTTP-428 controller failure occurs before any test fixture publication and
remains a separate failed/restored cohort; the corrected epoch-header run
passes independently. Current P4 artwork decoding and remaining-grid checks
remain unfinished.

## Current P4 artwork completed (7 November)

The same verified `656fc078c` firmware passes a separate six-case artwork
cohort on the 10-inch V3, with 400 cards, 7,566 channels and the full 512 KiB
value cache. No firmware rebuild or production policy change is made. Thirteen
host checks reject stale/missing transfers, unchanged animation hashes, wrong
static pixels/geometry, unexpected covered-card changes and resumption downloads;
the confined asset server separately passes its four loopback/gating checks.

Both initial image owners publish distinct opaque 2:1 PNGs. Every GIF case
first establishes that static reset on the card and the larger expanded
surface. A successful GIF then needs square geometry, a complete exact-URI
response and at least three distinct bounded 64-byte descriptor fingerprints.
Static samples match the configured little-endian RGB565 values. This prevents
the retained picture from satisfying the playback gate. The evidence covers
sampled component descriptors, with no new physical panel-pixel confirmation.

| 640-pixel GIF | Disposal | Card | Expanded | Card after close |
| --- | --- | --- | --- | --- |
| 2 MiB | Keep | Playback | Playback | Playback, no download |
| 2 MiB | Restore previous | Playback | Playback | Playback, no download |
| 8 MiB | Keep | Playback | Explicit RESOURCE refusal | Playback, no download |
| 8 MiB | Restore previous | Playback | Explicit RESOURCE refusal | Playback, no download |

Every expanded attempt preserves the covered card's loaded descriptor, dimensions,
byte count and fingerprint. After close, card animation resumes for at least
five seconds without another source request. In the final case, a real 2 MiB
response is held after 2,048 bytes while a disjoint configuration is published;
after release, the old response is cancelled and both replacement PNG owners
retain the expected pixels. This exercises in-flight cancellation and generation
fencing, with precise post-decode callback holds retaining their separate scope.

All 14 memory snapshots retain the full channel/value quota and 180 widgets,
with zero allocation/accounting errors and retired modal contexts. The counted
scene is 1,252,986 bytes in seven allocations with the modal closed, and
1,253,530 bytes in nine allocations while expanded. Closing returns to the
baseline after every attempt. Six independent IDF local-minimum windows record:

| Window | Internal minimum, bytes | PSRAM minimum, bytes |
| --- | ---: | ---: |
| Initial layout/feed/PNGs | 277,991 | 10,588,940 |
| 2 MiB, keep | 273,503 | 9,194,212 |
| 2 MiB, restore previous | 270,999 | 7,556,784 |
| 8 MiB, keep | 290,455 | 7,261,808 |
| 8 MiB, restore previous | 289,059 | 6,442,628 |
| Transfer/configuration overlap | 276,875 | 11,610,316 |

Regional minima must not be added, and these windows include the surrounding
controller operations rather than isolating decoder allocations. Twenty-six
responses include 24 MiB of completely served GIF data; the refused and retired
transfers retain their cancelled outcomes.

Two acknowledged exact-original native/legacy publications precede restoration
of official `v2.13.0`. The original dashboard, 77 observable protected settings,
PIN/reset and update flags are stable for 75.29 seconds and pass another fresh,
independent read-only preflight at generation 385. Encryption remains enabled;
the controller never reads or replaces its saved key. No device, asset-server
or build process remains running. Remaining-grid qualification and published
compatibility release windows are still unfinished.

## Current six-slot P4 pressure and artwork completed (7 November)

The 4.3-inch P4 passes a combined current-source `656fc078c` cohort using
ESPHome 2026.10.2, the stock SDK API and MIPI driver, and the selected six-slot
ELF. Its deployed 16 KiB NVS document fallback remains in use: only the
application OTA image is uploaded, without repartitioning or raw flash writes.
The exact maximum flat layout has 36 cards, seven pages, 28 climates, two images,
566 channels, a full 512 KiB state cache and 54 borrowed widgets.

The exact fixtures pass a focused Linux/GCC real-LVGL scene check, including
retired callbacks, unchanged temperature/unit rendering and navigation with
arena allocation disabled. The controller passes 27 mutation/recovery checks;
the artwork gates and confined server pass 13 and four checks respectively.
Synthetic cache pressure pads names and list/enum fields; numeric temperature
values remain below the production parser's length limit. The private feed
uses production prepared lookup/callbacks and bypasses native wire/authentication.

Four distinct A/A/B/B publications and 24 folder round trips produce 48 real
page transitions. All 28 pressure snapshots retain 586,400 scene bytes in seven
allocations. Fourteen additional artwork snapshots return to that baseline
after each expanded view closes; an open modal uses 586,944 bytes in nine
allocations. All 42 full-state snapshots retain the exact channel/cache/widget
quotas, with zero allocation/accounting errors and retired modal contexts.

Both independent 2:1 PNG owners match expected RGB565 samples before animation.
All four 640-pixel GIF cases—2 MiB and 8 MiB, each with keep and restore-previous
disposal—play on cards and expanded views. Each expanded attempt preserves its
covered card; after close, animation resumes for at least five seconds without
another download. A real transfer held after 2,048 bytes is cancelled across a
disjoint configuration replacement, and both new PNG owners retain their
expected pixels. Twenty-six responses include 40 MiB of completely served GIF
data. Bounded descriptor fingerprints and exact-URI responses establish fresh
component output; panel pixels and a precise post-decode callback hold retain
separate scopes. The optional current physical appearance response is pending.

Across the four held-Home three-sample medians, internal free memory is
375,395–377,367 bytes and PSRAM free memory is 24,552,792–24,624,700 bytes.
Combined free memory is 24,929,963–25,001,559 bytes from 28,507,535 bytes of
managed allocator capacity. That capacity excludes memory outside these pools;
this is not a whole-overhaul before/after or global no-growth claim. Ten local
minimum windows include the surrounding controller operations: the lowest
internal result is 350,275 bytes, and the lowest PSRAM result is 2,532,372 bytes.
They occur in different windows and must not be added.

Two exact-original native/legacy publications finish at generation 155 before
the verified original SDK2 development image is restored. Its compilation
identity, exact native dashboard, all 78 protected settings, PIN/reset and
update flags pass a 75.48-second stability window and another independent
read-only preflight. The newly captured Brightness Mode is preserved; the API
remains unencrypted as originally configured. No test or asset server remains
running.

Earlier attempts remain separately failed/restored: one stops before OTA for a
missing per-device secrets symlink, and another stops before fixture publication
because the harness reads the aggregate capability response at the wrong level.
Both restore the original generation-147 dashboard and settings; neither is
relabelled as a successful qualification. The corrected final cohort checks the
candidate's configuration capability object and independently verifies recovery.

The P4-86's published profile-specific recovery bytes and current encrypted
identity/dashboard/77 settings are captured without reading its key. The
following cohort completes its current candidate check. The 7-inch V2 still
requires recovery bytes for its newer weather firmware before qualification.

## Current P4-86 pressure and artwork completed (7 November)

The 32 MB P4-86 passes a combined `656fc078c` cohort on ESPHome 2026.10.2.
The compiled image verifies the actual nine-slot/six-image templates, revision-zero
support, stock SDK API/MIPI bytes and the published profile's partition layout.
Dynamic API encryption retains the saved key without a YAML PSK, key export or
controller key read. Only application OTA images are uploaded. The candidate
reports a 40,932-byte durable document limit; its existing partition table is
preserved.

The exact maximum flat workload has 81 cards, ten pages, 70 climates, two images,
1,406 channels, a full 512 KiB value cache and 81 borrowed widgets. Its Linux/GCC
real-LVGL check recompiles the C++ objects for the actual P4-86 profile and passes
four A/A/B/B replacements, retired callbacks, temperature/unit rendering and
36 folder round trips with arena allocation disabled. Local controller coverage
passes 33 checks, with four additional bounded PIN-status policy checks, 13
artwork-gate checks and four confined-server checks.

On hardware, four distinct pressure replacements produce 72 real page changes.
All 42 full-state snapshots retain the exact channel/cache/widget quotas with
zero allocation/accounting errors and retired modal contexts. The closed scene
is 667,484 bytes in seven allocations; an expanded view uses 668,028 bytes in
nine allocations and returns to baseline after every close.

Both independent 2:1 PNG owners pass expected RGB565 sample checks. The four
640-pixel GIF cases have these outcomes for both keep and restore-previous
disposal:

| Source | Card | Expanded | Card after close |
| --- | --- | --- | --- |
| 2 MiB | Playback | Playback | Playback, no download |
| 8 MiB | Playback | Explicit RESOURCE refusal | Playback, no download |

Covered card descriptors remain unchanged during every expanded attempt, and
resumption is observed for at least five seconds. A real response held after
2,048 bytes is cancelled across disjoint configuration replacement; both new
PNG owners retain their expected pixels. Twenty-six responses include 24 MiB
of completely served GIF data. Exact-URI responses and bounded descriptor
fingerprints establish fresh component output. Private state feeding bypasses
native wire/authentication; panel pixels and a precise completed-callback hold
retain separate scopes.

Held-Home three-sample medians across the four pressure passes record
382,827–383,083 internal free bytes, 21,688,644–21,781,516 PSRAM free bytes and
22,071,483–22,164,255 combined free bytes from 26,907,743 bytes of managed
allocator capacity. Memory outside those pools is excluded. Ten local minimum
windows include surrounding controller operations; the lowest internal result
is 355,303 bytes and the lowest PSRAM result is 10,884,964 bytes. Independent
regional minima must not be added. These measurements do not isolate a
whole-overhaul before/after effect or prove global no-growth.

The first attempt remains failed/restored. Its strict PIN check stops before
fixture publication when the candidate recognises a disabled version-three
record that official `v2.13.0` ignores. A bounded, separately checked recovery
preserves that record and restores the original observable status. The final
controller freezes exact candidate-reader status after validating the unchanged
disabled policy, rejects subsequent changes, and requires exact original-reader
status after rollback. The controller neither retrieves nor writes PIN material,
and no PIN record byte comparison is claimed.

After two acknowledged original native/legacy publications, exact published
`v2.13.0` firmware and the original native dashboard are restored at generation
219. All 77 observable protected settings, original-reader PIN status, reset
state and both update flags pass a 78.99-second stability window plus another
independent read-only preflight. Encryption remains enabled. The original
Camera Dimming setting is unavailable and remains outside the observable
settings scope; no test command sets it. No test, asset server or build remains
running. Current 7-inch V2 qualification and the actual compatibility release
windows are still unfinished.

## Previous-application copy and return proof (7 November)

A private recovery helper passes a known-image hardware proof on the 10-inch
P4 V3. It reads only the identified previous application, requires the actual
pending-helper/valid-original rollback witnesses, and exports two sequential
copies into private local files. Both 5,784,832-byte copies match the retained
published `v2.13.0` OTA image byte for byte. The helper then returns to the
original application slot without another firmware upload or a power cut.

The helper has no API service, OTA uploader or safe-mode image acceptance.
Its compiled startup instructions and ELF verify that automatic acceptance is
absent; runtime checks also require the expected MAC, application descriptors,
image size and two application-slot boundaries. Local ASan/UBSan checks cover
the actual helper against SDK stubs and its bounded copy policy. Six copy
controller tests and four runner tests cover interrupted transfers, stale
requests, corruption, deadlines and settings cleanup. These checks do not
establish zero SDK-level NVS writes or electrical power-loss behaviour.

The original dashboard remains at generation 385. All 77 observable protected
settings, PIN status, reset state, update flags and encrypted API mode pass an
86.31-second stability window and another independent read-only preflight.
No API key is retrieved or set, and no dashboard or partition-table write is
performed. The first attempt remains failed/restored: the local uploader could
not find its executable and transferred no firmware; its held update flags
were restored before the successful retry.

Separately, the unchanged 7-inch V2 reports bootloader rollback support through
its existing API. Only that diagnostic is retained; other configuration log
payloads are discarded. The following cohort completes recovery of its current
weather image. The fifteen-slot runtime/artwork qualification remains separate;
this transport proof does not enable production nested-page writes.

## Current seven-inch V2 recovery retained (7 November)

The current weather application is now retained as two independent private
5,450,416-byte OTA copies with the same SHA-256 (`0549095353e6`). Both contain
the uniquely matched `2026-11-06 16:07:39 +0000` build timestamp. Image checksum,
SDK 2026.10.2, V2 project descriptor and embedded ELF identity checks pass.
The helper requires the actual pending/valid application witnesses and paired
slot boundaries before reading the previous application. Only that identified
application is exported; PIN material, API keys and arbitrary flash/NVS are
outside the copy interface.

The first attempt remains failed/restored: a revision-3.1-only helper is
rejected at final OTA validation. Its active original firmware, dashboard and
settings are verified afterward. The corrected helper uses revision-zero
support, matching the V2 profile's engineering-sample setting and the original
image's maximum revision of 1.99. Its compiled source/API absence/rollback/layout
audit passes before the successful hardware run. Seven ownership tests and
four runner tests cover foreign state, lost acknowledgements and cleanup; the
bounded helper and copy-controller checks retain the preceding scope.

The original application slot boots without another firmware upload. Its exact
weather dashboard remains at generation 10, with all 78 observable protected
settings, PIN status, reset state, update flags and original unencrypted API
mode verified through an 86.23-second stability window and a fresh independent
read-only preflight. No API key is retrieved or set, and no dashboard or
partition-table write is performed. Recovery identity uses the retained image
and original-slot witnesses plus fresh API/HTTP metadata; direct restored ELF
readback and zero SDK-level NVS writes are not claimed.

Separately, the exact current fifteen-slot V2 host workload passes 225 cards,
16 pages, 4,166 channels and the full 512 KiB state cache. All C++ objects use
the actual V2 profile with real LVGL. Four A/A/B/B replacements reject retired
callbacks and pass 60 folder round trips with arena allocation disabled and
135 borrowed widgets. The 64-bit host scene remains 959,586 bytes in seven
allocations; these are host results, not physical device memory measurements.
The compiled candidate also passes component-source, stock SDK API/MIPI,
actual fifteen-slot/six-image template, 16 MB/revision-zero and native API
identity checks against its ELF. Physical pressure and artwork qualification
are still required before the remaining grid gate is complete. Actual
compatibility releases remain required before production nested writes.

## Fifteen-slot V2 storage failure diagnosis (7 November)

The current-source fifteen-slot physical run is **not a pass**. On the
7-inch V2 at `192.168.6.102`, source `656fc078c93f` accepts the first
11,337-byte maximum flat dashboard, but rejects its second replacement with
HTTP 500 and leaves the active generation unchanged. The frozen failed run
and its independently verified original recovery are retained separately.
The 225-card/four-replacement/artwork gate remains open; the workload has not
been reduced to turn this failure into a pass.

A separate diagnostic image, `storage-diag-656fc078`, compiles against the same
437 production component sources and stock SDK API/MIPI code. Its bounded
32-entry observer calls the real `nvs_set_blob` unchanged and records only
configuration-slot lengths/error codes and aggregate NVS counts. It neither
reads protected key/value contents nor changes the partition table. Actual
header ASan/UBSan checks cover passthrough, errors, exclusion of other keys and
ring bounds; source, linker-wrapper, ELF API identity, actual 15/6 template,
16 MB/revision-zero and OTA artifact checks pass before installation.

The deployed fallback uses a **53,248-byte shared NVS partition**. The first
fixture publishes successfully, then shared native/compatibility/preferences
storage reaches 1,265 used entries out of 1,638. Before the second payload,
withdrawing the older target leaves 405 free entries. The real 11,365-byte
store-envelope write returns `ESP_ERR_NVS_NOT_ENOUGH_SPACE` (`4357`); HTTP
returns “Panel configuration could not be saved” and the first fixture stays
at generation 14. The nominal 16,356-byte document ceiling does not reserve
space for all shared data or NVS garbage collection. This is a persistent
storage failure, not evidence of runtime heap exhaustion.

The first fixture does fill all 4,166 state channels, the entire 524,288-byte
state cache and 135 borrowed widgets. Its one held diagnostic snapshot is:

| Reported memory region | Total bytes | Free bytes | Used bytes |
| --- | ---: | ---: | ---: |
| Internal heap | 519,455 | 336,107 | 183,348 |
| PSRAM | 27,919,488 | 20,647,440 | 7,272,048 |
| Combined reported regions | 28,438,943 | 20,983,547 | 7,455,396 |

The managed scene holds 931,070 bytes in seven allocations, with zero scene
allocation/accounting errors. This is one diagnostic snapshot, not a matched
before/after comparison, steady-state median, completed replacement cohort or
panel appearance result.

The diagnostic restores both native slots to the exact original weather
payload at generation 16, uploads the retained exact original application,
checks all 78 protected settings plus PIN/reset/update policy and preserves
plain API mode without reading or changing an API key. Restoration remains
verified for 78.37 seconds and passes a separate read-only preflight. The
independent diagnosis audit passes while leaving qualification failed. No test
is left armed. The shared NVS capacity handling must be addressed before
crediting repeated full-size saves on this deployed layout.

## Follow-up refresh requests during an active sweep (7 November)

Runtime PR #2400 commit `b713e1315` retains one coalesced follow-up Home
Assistant refresh when another request arrives after the current sweep has
visited channels. It finishes the current tail before restarting, preserving
one-shot backpressure without adding persistent listeners or disconnecting
clients. The actual transport regression fails with the old policy and passes
under ASan/UBSan with SDK stubs. Audited 2026.10.1/2 and unaudited-version
rejection checks pass. This fix is carried into the feature branch with its
native-lookup dispatch/maintenance guards preserved and the SDK checks rerun.
It has not yet been flashed; earlier physical evidence retains its original
source scope.

## Remaining qualification gates

The 480 × 480 S3 appearance gate has explicit user confirmation, with the host
interruption and separate recovery qualified above. Physical cases outside the
recorded P4/image/modal/takeover cohorts remain unqualified. The current
10-inch published-release recovery, cancellation and automated abrupt-reset
round trips are verified above. Manual power cuts are no longer required for
the replacement transaction crash-recovery workflow. Electrical power-loss
and brownout behaviour remain outside its qualified scope.

The historical SDK1 memory regression still lacks its original image/display
observations. Published-reader and mirror-retirement gates in
[the delivery plan](nested-pages-delivery.md) still apply before enabling
production depth-two writes. Keep PR #2424 and #1825 open.
