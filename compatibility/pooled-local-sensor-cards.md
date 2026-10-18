# Shared local sensor state and card views

The unselected renderer supports numeric and text Local Sensor cards through
`PageLocalSensorRuntime`. Device-local data has its own packed configuration
cache and connection generation: an HA reconnect cannot invalidate it. Values
are shared by object-id/type across root and child cards, without view callbacks,
per-card contexts, timers or HA registrations.

The ESPHome provider uses the generated numeric/text entity counts for a fixed
producer table. A producer gets one lazy callback, capturing only its stable
index. ESPHome has no public callback-removal API, so those bounded records have
firmware lifetime. They contain no configuration strings or widget pointers.
The fixed table and callback count are reported separately from configuration
bindings and cache arenas. All provider/feed/cache operations run on the
application loop. Registering additional ESPHome entities after setup is outside
this provider's contract.

Configuration bindings can be discarded or retired independently. The manual
`send_local_sensor_update` path forwards through a fixed feed only after the
caller declares the legacy renderer retired. Computed keys need no physical
producer. Exact key/type matches update active/staged caches; retired keys cannot
reach reused labels. Manual overrides hold until the next physical report.
An installed feed belongs to one runtime and clears on destruction.

Local numeric samples use four internal float bytes, preserving the existing
printf rounding, non-finite `--`, configured units and standard font. This is
volatile cache data, never persisted or exposed as a document format. Numeric
labels use only their configured label; no old widget caption can carry over.
Text keeps exact case/punctuation, its icon and wrapped bottom label. Initial
empty text leaves `--`; a subsequent empty report clears the label. The existing
C-string boundary is preserved. No local sensor contributes a folder indicator.

Staged local caches read physical producers directly instead of inheriting old
view state. They refresh existing handles again before activation, without
allocating or dispatching actions. Failed preparation/discard preserves the old
active cache. A configuration without local values releases both arenas and its
binding table. Producer callbacks remain bounded by the physical device.

The cache has aggregate and per-channel byte limits. Local text is capped at the
shared 256-byte limit (or a lower requested budget); exhausted data displays
`--` and increments overflow diagnostics, rather than publishing a prefix.
`PageEntityBudget::channel_bytes` defaults to unlimited for existing HA owners,
so their current policy is unchanged. The local provider opts out of inherited
shared values to preserve fresh/initial-empty semantics; other subscriptions
retain the existing default.

Memory diagnostics separate wrapper storage, active/staged/peak packed cache,
current/peak binding storage, the shared feed, producer storage and producer
callback count. They do not measure ESPHome's callback-container overhead or
prove device heap/PSRAM savings. Include those and image/modal peaks in physical
qualification.

Host tests cover duplicate consumers, all four producer-feature combinations,
missing/computed keys, empty reports, fresh activation, precise float formatting,
HA reconnect independence, discard/rollback, exhaustion/recovery, allocation and
watch failures, and 1,000 disjoint replacements. Native LVGL tests cover all nine
profiles, local text/number slot reuse, manual values, invalid numeric recovery,
1,000 navigation cycles and stable retained C++/LVGL/arena/subscription resources.
Use `ESPCONTROL_TEST_FAMILY=local_sensors` for that focused suite; it is included
in the default native suite too.

Production selection and the remaining drivers are unfinished. Physical S3 and
working 20-slot P4 qualification, remaining profiles, the complete factory build
matrix and the persistence/compatibility gates still precede nested-page support.
No device was flashed by this stage, and saved/backup formats are unchanged.
