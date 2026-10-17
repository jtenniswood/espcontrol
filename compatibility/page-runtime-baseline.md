# Page runtime migration baseline

This is the first implementation slice of
[issue #1825](https://github.com/jtenniswood/espcontrol/issues/1825#issuecomment-5497549740).
It establishes shared limits and memory capture tools. The existing renderer,
live configuration pipeline, document v1 and persistence remain in use.
Nested subpages and capacity increases remain disabled.

## PageStore shadow migration

`PageStore` now stages native v1 documents into caller-owned arena storage,
using explicit page/card identifiers, interned normalized strings and
generation-scoped handles. Its adapter reuses the production card parsers.
The retained source document preserves settings, original encodings and
stored-but-inactive data exactly during this migration.

`PageStoreRuntime` is an opt-in, host-tested owner, not the production renderer.
It reserves a replacement arena before commit, publishes the committed
generation without allocation, and releases staged resources on failure.
Memory counters distinguish active, staged and peak reserved arena bytes.
Caller-selected arena capacities are test budgets, not qualified device limits.
These counters exclude temporary allocations made by the existing parsers;
those peaks still need to be measured and bounded before production cutover.

The configuration service now supports prepare/commit/activate/mirror ordering.
Existing text-backed adapters retain their apply-only compatibility path;
they do not yet promise allocation-free activation. Production wiring must not
select the PageStore owner until the entity-state, transport-reclamation and
pooled-renderer gates pass. No second full model is retained on live devices.

Host coverage includes empty/full existing configurations for all nine
profiles, supported card types, compact and legacy fields, UTF-8, exact v1
round trips, inactive slots, arena exhaustion, stale handles, 1,000 generation
replacements, allocation failures, write failures and generation conflicts.
These are host-model checks, not physical navigation or memory results.

## Capacity contract

`scripts/generate_device_slots.py` generates `device_ui_limits_generated.h`
from the device manifest and Product Model card contract, alongside each
device's profile selection flag. Firmware reads it through `device_ui_limits.h`;
the editor already reads the same product definitions through `web_config()`.
The firmware tests compile the selected limits and existing grid foundations
for every profile. A conflicting manual grid limit fails compilation.

| Display | Root slots / first-level subpages | Cards per subpage, excluding Back | Image slots | Legacy chunks per subpage |
| --- | ---: | ---: | ---: | ---: |
| 4.3-inch P4, including V3 | 6 | 5 | 6 | 8 |
| P4-86 | 9 | 8 | 6 | 8 |
| 4-inch S3 | 9 | 8 | 2 | 4 |
| 7-inch P4, including V2 | 15 | 14 | 6 | 8 |
| 10-inch P4, including V2/V3 | 20 | 19 | 6 | 8 |

Each legacy text chunk permits 255 UTF-8 bytes. A character cannot span chunks,
so multiplying chunk count by 255 is only an upper bound. Native v1 record
bounds remain defined by the document codec; this change does not restrict
native documents to legacy byte limits or resize the versioned storage format.
Card support and permission to appear on subpages come from the card contract.
Rotation changes visibility, not configuration capacity: the 10-inch V3 retains
20 stored root slots even when portrait mode displays 18.

The generated grid bound replaces handwritten 6/9-slot flags. It also sizes
7-inch and 10-inch runtime arrays for 15/20 slots instead of the historical
25-slot fallback. Subpage scratch arrays keep the conservative square of the
root capacity during this migration. The 25-slot fallback remains available
to standalone host consumers without a selected production profile.

## Capture a baseline

Start with the S3, then a 20-slot P4. Record the exact firmware commit, device
slug, configuration backup, orientation, HA connection state, enabled image
features, sampling interval and phase alongside every capture. Compare the
pre-change `main` commit with this branch using identical configurations.
The added sensors require this branch's firmware; older firmware leaves those
optional columns empty.

```sh
python3 scripts/monitor_display_memory.py \
  --target s3=192.168.6.105 --samples 120 --interval 1 \
  --csv /tmp/s3-navigation.csv
```

Use a separate CSV per phase: idle after boot, repeated root/subpage navigation,
modal open/close, HA reconnect, save, backup restore and live configuration
replacement. Run at least 1,000 navigation cycles, with empty, representative
and maximum configurations. For maximum configurations use every first-level
subpage, longest accepted text, distinct HA dependencies and the profile's
image capacity. Verify Back position/label, spans, summaries and live state.
Save and restore while rotated, including the V3's stored-but-inactive slots.

The monitor reports first/last values, sampled minima/maxima and trends for
free internal heap, largest internal block, free PSRAM and largest PSRAM block.
New optional `heap_min_free` and `psram_min_free` values report the allocator's
minimum free bytes since boot, capturing brief allocation peaks between polls.
Reboot before comparing runs: these minima do not reset between phases.
Largest-block minima are sampled and can miss short fragmentation peaks.
Historical minima are excluded from leak classification because a temporary
allocation peak can lower them without retaining any memory.

Whole-device figures include framebuffer and LVGL draw buffers. They do not
separate configuration, entity state, widgets, modals, transport registrations
or persistence ownership, and they do not measure navigation latency. Record
those separately when the corresponding runtime instrumentation is added.
No physical measurements or memory-saving claims are established by host tests.

## Next delivery gates

Before PageStore or renderer cutover, finish the per-profile golden fixture
corpus, owner-level memory/registration counters and navigation timing, and
record the physical baseline above. Measure both active and staged
configuration peaks; do not treat the proposed 250 KiB saving or 0.70 MiB
ceiling as verified thresholds.

Then deliver the issue's remaining stages as focused PRs: staged PageStore and
v1/legacy parity; replaceable HA state and transport reclamation; pooled card
drivers with generation-safe unbind; negotiated document v2; streaming storage
with prepare/commit/activate/mirror semantics. Keep the existing depth and
capacity contract through all of them. Require S3 and 20-slot P4 physical
parity and bounded resource growth before shared renderer cutover; retain the
issue's upgrade/downgrade and compatibility-mirror release gates.
