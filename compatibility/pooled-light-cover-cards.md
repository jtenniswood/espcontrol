# Pooled light and cover cards

This extends the unselected renderer for #1825 with light-control cards and the
cover All Controls variant. Slider/position/tilt/command variants are still
rejected until ported. The application still selects the legacy renderer;
document limits, backups and nesting are unchanged.

Light cards preserve their on/off icons, colours, label/friendly-name fallback
and push feedback. Cover cards remain unchecked and use a proportional closed-
amount fill from the top, with open/closed icons and the existing split-glyph
text-contrast helper. One optional plain fill object per visible slot replaces
an interactive slider and its context for this read-only card background. The
owner supplies that fixed bank before initialization. All slots must have fills
if the configuration contains an All Controls cover. Navigation never creates
widgets or event descriptors. Resize events update fill geometry, and unbind
hides and zeroes the fill before the shell can be reused by another card.

The control action hook is mandatory for these families. It receives the
validated original press/binding token and must forward to the shared modal
runtime. This branch does not install that application dispatcher.

## Position history without hidden widgets

Legacy cover callbacks ignore malformed position updates. Keeping only the
latest raw state would lose the previous valid position while a page is hidden.
The shared state channel now retains a bounded percentage for current_position
and current_tilt_position. It occupies existing padding: a compile-time assertion
keeps Channel at 32 bytes on host and firmware builds. There is no extra per-card
context, channel or arena reservation. Disconnect/generation changes clear it.

Packed strings are parsed through a terminated 65-byte stack buffer. Numeric
prefixes retain the legacy policy; non-finite/oversized values are ignored and
finite values are clamped before integer conversion. A new modal can replay the
last valid percentage after malformed or overflowing raw updates. The replay
snapshot remains owned and revision-checked through the complete callback.

## Qualification

Pure tests cover every profile, defaults/friendly names, partial/full fill,
open/opening/closed/closing fallback, exact-length strings, fractional rounding,
malformed updates while hidden, finite extremes and reconnect invalidation.
Replay tests cover canonical position/tilt history, raw-buffer exhaustion and
connection changes.

Native LVGL tests run the original basic/status fixtures plus 1,000 control-page
navigation cycles on each profile, with a real software-rendered frame, grid
resizing, fill reuse and stale input. They assert stable retained LVGL bytes,
C++/arena reservations, subscriptions and retained label-descriptor counts.
LVGL drawing and temporary style/grid scratch allocations are separate from
retained resources; these tests do not claim zero total allocation or device
memory savings. Modal lifecycle regressions and actual S3 SDK instantiation are
also required. Final check/build results belong in the PR.

Production selection, remaining card/async owners, storage/version work and
physical S3/20-slot-P4/other-grid qualification are still unfinished. No display
has been flashed for this stage. Preserve installed S3 speaker/reliability fixes
in later integration; P4-86 hardware remains untouched.
