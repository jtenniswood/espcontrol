# Native modal bank

This #1825 stage assembles all fifteen demanded panel kinds under one
`PageNativeModalBank`. Production still selects the legacy renderer until the
complete scene, widget bank and configuration ownership are connected and
qualified. It does not enable nested pages or establish physical memory savings.

The bank owns fourteen native family ports (alarm control and PIN share one),
one modal runtime, and the confirmation, alarm and guest Wi-Fi command owners.
It borrows the scene's graph, forecast/alarm state, acknowledgements, QR encoder,
expanded image view, network source and source-bound style resolvers. No family
owner or routing record is created per configured card. Install happens after
all members are constructed and after the eager renderer is retired. Missing
resolvers, missing validators, invalid slot counts and unsafe dispatch reject
initialization. A failed family installation can be retried before activation;
configuration preparation is blocked until every family has installed.

All panel kinds use the same modal request/retirement boundary. A retiring
context continues to occupy that owner until its family is safe to collect.
Commands run before collection, with recursive bank maintenance blocked.
Configuration replacement retires the panel before cancelling its borrowed
command state; shared graph/source storage must follow the bank in the scene's
resource group. The scene must retain original grid bindings under ordinary
panels for accepted feedback and reconcile cover/retirement before re-enabling
input. Full display takeovers remain the scene's responsibility.

Native command adapters use the existing Home Assistant send helpers, defer
sending until outside LVGL/native/HA callbacks, reject locked/disconnected
states, and propagate actual send acceptance. Confirmed scripts retain the
canonical script fields/templates. The canonical action helper now returns the
HA send result; legacy callers may ignore it. Its local-action return means a
nonempty key was dispatched to the existing local registry, whose callback has
no result. Alarm PIN selection delegates to the exact demanded alarm opening;
no PIN or entity view is retained by the adapter. Guest controls retain the
existing restriction to valid HA switch entities.

The native sanitizer harness uses the actual command adapters, canonical action
helper definitions, configuration parser/store, graph and modal runtime. It
covers script payloads, rejected sends, reconnect/lock cancellation, safe callback
admission, exact opening cancellation and PIN selection, and 1,000 command cycles.
A composition check extracts the complete production bank unchanged and doubles
only its UI families and external display/forecast sources. It exercises all
fifteen routes for 1,000 cycles each, one-context ownership, deferred collection,
partial-install retry, preparation/discard and offline local settings. Individual
native family tests remain responsible for their real LVGL widgets. An S3 SDK
syntax check uses the actual generated firmware translation unit and current
ESPHome/ESP-IDF headers; this is separate from a full build or physical testing.

Next connect the full scene's model/state owners, source styles, shared card
command queue, covered input, artwork/QR pools and device widgets. Retire the
legacy eager contexts and raw YAML sends before selecting the scene. Preserve
matched S3 and working 20-slot P4 baselines for physical heap/PSRAM comparison;
storage/compatibility qualification still precedes depth-two nesting.
