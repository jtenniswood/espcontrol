# Demand-created light and cover panels

This extends the unselected pooled renderer for #1825. It does not enable
nested pages, change document limits, or select the renderer in the application.

`PageLightModalPort` and `PageCoverModalPort` use the existing modal contexts,
constructors and subscription/UI helpers. Each opening allocates one context
record and one bounded gesture bank through the page arena; no card retains a
modal context or pointers into a reusable grid. The context owns its configuration
strings until its callback/event has returned and maintenance destroys it.
All family ports must share one `PageModalRuntime` through a dispatcher.

Installation requires an explicit assertion that the legacy renderer has been
retired. These two families have no legacy context-reference registry, so this
is a caller contract, not automatic detection. A mandatory style resolver supplies
the correct device fonts and palette. Existing eager callers keep their original
allocation/ownership behaviour when the optional observer is uninstalled.

Modal subscriptions belong to the opening's context owner. They reuse graph
channels and seed missing values from the matching configuration generation;
subsequent maintenance does not replay over user edits. Retirement invalidates
input and releases callbacks immediately. Configuration replacement hides the
panel and detaches the shared modal service, then defers widget/context destruction
until outside HA and LVGL dispatch. A reconnect cannot reactivate a stale opening.
An unavailable entity closes its panel during maintenance.

The light palette previously allocated one click payload per swatch without a
delete callback. It now points to immutable preset data. Rebuilt swatches receive
the same bounded gesture tracking as the original panel; setup closes if that
reservation is insufficient. Cover preset failure rollback checks the original
context is still active after the service call, which may close the panel.
Climate shares the observer implementation while retaining its existing facade.

## Checks and remaining gates

The native LVGL fixture exercises production contexts, constructors, subscriptions,
close/can-open functions and the complete light colour-grid rebuild. The visual
panel layout, selected transport/parser helpers and service sending are doubles;
this is not a physical panel or full input/service integration test. It runs 1,000
open/close cycles per family, four palette rebuilds per light opening, seeded
state/edit preservation, guarded press/release/clicks, closure during an event,
configuration replacement in a real coordinator callback, reconnect, unavailable
state, arena exhaustion, style failure and gesture-budget exhaustion. Retained
LVGL bytes, arena reservations and graph callback/channel counts return to baseline.
Palette rebuilding adds no C++ click-payload allocations; LVGL widget allocation
is expected during modal setup/rebuild, unlike ordinary pooled page navigation.

Actual SDK compilation explicitly instantiates both adapters. Existing climate
option-value and lifecycle regressions must pass against the latest main.
The factory matrix and full repository checks are recorded in the PR.

Application selection, family dispatcher, light/cover grid presentation, the
remaining media/weather/local/image ownership, and physical memory/navigation
qualification remain required before a production cutover. No device was flashed
for this adapter stage. Preserve the S3's installed speaker/reliability fixes in
later integration; P4-86 hardware remains untouched.
