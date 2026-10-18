# Demand-created media controls

The unselected page renderer can demand-create the existing All Controls,
Speaker Group and Cover Art control panels. Each opening owns one context and
gesture reservation. It reads the configuration-owned entity graph and shared
media clock/freshness owner, without registering another `MediaPlaybackState`
or per-opening Home Assistant subscriptions. Cover panels follow the same
primary/secondary route as their card.

The shared snapshot presentation policy retains volume limits, delayed volume
and seek acknowledgements, shuffle/repeat capability checks, grouping discovery
and separate power control. An unavailable main player can retain its usable
Power tab. The S3 reads progress availability from the shared clock rather than
searching the legacy media-state pool.

Inputs consume fresh state before calculating a command. Widget rebuilding
waits until input dispatch returns, so a capability change cannot delete the
button or row currently handling a tap. Group action responses carry the exact
64-bit opening identity, including when another opening reuses its address.
Replies also validate the player entity, so a cover-source switch cancels
requests for its previous player. Input waits for the replacement player view
to finish painting. A callback dispatch scope defers context and widget
destruction during an in-flight action response. Closing cancels outstanding row responses; late
responses cannot update replacement panels.

The active record caches channel revisions and local acknowledgement state.
A projection guard rejects nested input while a snapshot is being painted.
Unchanged frames and unrelated media-clock ticks skip parsing and painting.
Confirmed group feedback remains in the demanded context until the membership
channel's revision changes; clock and unrelated state updates cannot replay an
older membership value over it. Response callbacks repaint existing rows and
attach their newly enabled controls; candidate construction waits for the next
runtime budget check. All projection and context storage is
released with the opening. No fields are added to configured legacy contexts.

The gesture reservation also bounds speaker discovery before parsing and row
construction. It reserves 32 entries for fixed controls and four per speaker;
the default 128-entry reservation admits 24 input entries and distinct candidate
rows. Oversized lists fail admission without displaying or commanding a prefix.
The reservation and physical peak still need profile qualification before
production selection. A 64-channel per-card revision reservation fails closed
if a future schema exceeds it. Extreme finite discovery volumes are clamped
before conversion, non-finite readings remain unknown, and progress display and
seek validation share the bounded playback helpers.

Native LVGL tests extract the production contexts, snapshot policy, tab/input
builders and group action handlers. Platform shell geometry and speaker-row
construction use doubles; these tests do not establish pixel or physical layout
parity. They check retained C++/LVGL/arena/subscription resources across 1,000
open/close cycles, quiet frames with another player ticking, capabilities before
a tap, separate offline power, volume/seek grace, exact-address context reuse,
late responses, callback-time retirement, configuration replacement, reconnects,
resource admission and shared cover-source routing.

Production still selects the existing renderer. These adapters require a single
scene/modal dispatcher, original grid-binding validation for numeric previews,
complete renderer parity and physical S3/P4 memory and workload qualification.
The configuration-activation fresh-snapshot gate remains in effect for channels
already owned by another component. No nesting, persistence/backup format or
physical memory-saving claim changes in this stage.
