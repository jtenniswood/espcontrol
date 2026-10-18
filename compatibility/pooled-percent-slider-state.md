# Ordered percent-slider state

`PagePercentSliderRuntime` prepares configuration-owned state for light
brightness, fan speed, cover position and cover tilt sliders. It is a prerequisite
for their reusable views, which are not wired by this stage. Production keeps
the existing renderer. Numeric and light-temperature sliders need separate
policies and are deliberately not classified by this owner.

Fan Speed now declares the actual slider dependencies for its entity domain,
including light/cover aliases, instead of unrelated fan-modal attributes.
A source is shared by entity and level attribute. Each eligible card has a small
preview record: dragging one duplicate card must not change the other card's
optimistic value. These are semantic records in exact active/staged arenas,
never widgets or legacy slider contexts. Defaults permit 32 sources, 512 cards
and 2,048 name bytes including terminators. Preparation validates every budget
before allocating; discard/failure preserves the active configuration, and an
empty configuration frees the arena. Diagnostics separate fixed storage,
active/staged/peak bytes, source/card counts and preparation failures.

Prepare the owner before the HA graph and include it in the fixed
`PageEntityObserverSet`. Live reports retain their original order. A valid level
report changes the fill even when primary power is off. A later Off/unknown/
unavailable report clears it; an On report changes availability/icon without
restoring a previous percentage. Invalid level reports leave the fill intact.
Cover attributes also change the icon; light/fan attributes do not. Copied graph
snapshots seed attribute-before-primary semantics independently of the graph's
deduplicated channel order. Live fill/icon writes take precedence over later
copied fields during preparation.

Card previews survive hidden-page reads until a valid level report or a primary
Off report reconciles every consumer. Identical reports still reconcile them;
cache byte deduplication cannot suppress that acknowledgement. An On report or
invalid level report does not erase the preview. New configurations do not
inherit previews. Reconnect clears old values/previews and availability until
fresh primary state arrives. Before the first primary report, nonnumeric sliders
retain the existing initial available state. Actions still require current
binding and transport checks in the eventual view/scene.

Percent values use bounded numeric-prefix parsing, reject non-finite input and
clamp before integer conversion. Brightness retains the legacy 0–255 conversion,
including a minimum 1% for a positive value. Input is limited to 64 bytes. Invalid
or oversized reports preserve the last level; the eventual view must separately
fail closed for exhausted graph data. The owner supplies raw position; cover
fill inversion, orientation, geometry, touch dispatch and draw contrast belong
to the view port and remain unfinished.

The reference test runs 10,000 report sequences for each of the four source
kinds against the actual production subscription callbacks. It also checks
independent previews, repeated confirmations, copied/live snapshot ordering,
shared-graph staging, stale generations, reconnect, limits/allocation failures,
all nine profiles, allocation-free reads/updates and 1,000 replacements.

There are no new HA registrations, widgets, callbacks or timers in this owner.
Host retained-resource checks and SDK compilation do not establish device RAM
savings, physical layout or the full factory build matrix. Visible slider ports,
production scene selection and physical S3/P4 qualification are still required.
No device was flashed; saved/backup formats and nested-page support are unchanged.
