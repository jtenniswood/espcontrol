# Image refresh handoff

This #1825 prerequisite moves fixed refresh state between an image tile and one
expanded view. [Typed modal request ownership](pooled-modal-image-ownership.md)
remains separate from tile downloader ownership. Production still uses the
existing renderer; the demanded expanded source/UI, cache, application scene,
compiled legacy retirement and physical qualification remain unfinished.

`PageImageRefreshState` owns bounded activity/revision bytes, periodic/activity
schedule, HA connection, primary/source flags, tile completion time and camera
failure cooldown. It has no entity, URL, widget or configuration pointers.
Primary/token/picture/trigger updates and completion helpers allocate nothing.
Tile and modal revision acknowledgements remain distinct, and a modal success
does not rewrite the tile's 30-second duplicate guard.

`PageImageTileRuntime` permits one expanded transfer across its two S3/six P4
slots. The global transfer records the exact tile binding/serial, full modal
opening, monotonic transfer nonce and stable modal origin. It does not add a
modal record to every configured or visible card. `has_tile` lets the demanded
source distinguish a hidden-card request, which must seed independent state,
from a failed/exhausted transfer of an existing tile.

The demanded observer must already own its state before `begin_expanded`.
The tile copies state and delegates observation before cancelling its current
request, so a fresh report delivered reentrantly during cancellation updates the
new owner. Completed tile pixels remain owned for a guarded modal preview;
transfer itself neither downloads nor releases pixels. The tile skips delegated
policy work until state returns, and cancellation rechecks exact binding/serial
and modal identity before reporting success.

`end_expanded` requires the exact opening/transfer nonce and original tile
binding/serial. It intentionally permits return after the modal token has been
revoked: real modal retirement invalidates the token before calling its port.
A stale close cannot overwrite another opening or rebound card. Lost/expired
transfers resume from current graph state without replaying hidden activity;
configuration/unbind retirement invalidates them before borrowed metadata dies.
The modal origin must outlive an active transfer.

The handoff preserves periodic completion cadence and motion baselines/windows,
including sustained-on activation when entering expanded view. Ordinary camera
failure counts and 2/4/8/16/30-second cooldowns cross the boundary. Missing or
preempted tile work resumes with the existing one-second deferral, while source
updates keep their event-relative delay and memory admission cooldown. Closing
an unchanged healthy camera with refresh off does not create another download.
Image revisions received during modal work still leave the tile's older revision
pending after the modal has acknowledged its own result. A shared invalidation
marker hides tile pixels after unavailable/recovery or a modal failure until that
tile receives its own fresh successful image; modal success alone cannot revive
the old tile buffer.

Tests combine the actual entity graph, renderer and modal lifecycle with transfer
I/O replaced. They cover cancellation-time fresh reports, a close during modal
initialization, return after token invalidation, reconnect, configuration
publication from private UINT32_MAX to generation zero, overwritten old arena
bytes, hidden-card admission, stale transfer fields, final nonce exhaustion,
clock wrap, error/recovery timing and revision coalescing. Each of nine profiles
passes 1,000 navigation cycles plus 1,000 allocation-free handoffs without new
channels or transfer starts. Existing image/modal/cover-art sanitizer and GCC
suites and actual S3/P4 SDK checks supplement the models. Native S3 and twenty-slot
P4 grid image regressions remain required; none of these are physical image/RAM
qualification or current factory builds.

Expanded source/UI integration must use the returned state as its sole observer
owner, call `expanded_finished`, return state before releasing its own metadata,
handle direct hidden sources, defer other tile transfers behind an overlay and
preserve existing modal headroom, preview and cache policy. The current raw modal
lease releases retired pixels in maintenance; bounded reuse of the legacy modal
cache still needs an explicit safe ownership path. No format, backup or nesting
behavior is enabled by this handoff stage.
