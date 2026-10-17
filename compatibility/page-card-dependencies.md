# Declaring card dependencies before rendering

`PageCardDependencies` supplies the configuration-owned state owner with the
static Home Assistant state/attribute pairs used by each registered card driver.
It walks immutable PageStore records without creating LVGL widgets. Production
still uses the existing renderer; these declarations do not enable nesting or
select the new state owner on a display.

The source distinguishes sensor inputs from action targets, local sensors from
HA values, command-only cards from stateful controls, conditional friendly names,
and time sensors with automatic units. It includes configured secondary media
and power entities, speaker-discovery inputs, camera activity triggers, inherited
alarm targets and inputs needed by automatic folder summaries. Media policy
honours the S3's existing low-heap control variant. Shared pure speaker-discovery
helpers are used by the old binding code and the new source.

All configured cards on navigable pages are considered, including cards hidden
by the current layout/orientation. Stored-but-inactive subpages are preserved in
PageStore but do not own subscriptions. Unsupported active types and nested
folder cards fail preparation rather than being silently omitted.

Climate declarations reserve the optional mode attributes that changing HA
capabilities can expose. The final renderer must consume these prepared inputs
instead of allocating subscriptions when a page or menu opens. This can reserve
more channels than the old lazy climate binder; measure complete preparation
peaks and choose qualified device budgets before cutover.

Forecast responses, local ESPHome values, artwork/image bytes, ambient panel
consumers and temporary modal-discovery bindings require their own existing or
migrated owners. This declaration table covers static card HA inputs; it is not
a claim that all remaining mutable card/forecast/media state has moved out of
widgets or that pooled rendering is complete.

The host tests check concrete pairs for simple sensors, actions, selection,
numeric/light/cover controls, climate, media, camera triggers, guest Wi-Fi,
weather, timers and folder/alarm behaviour. They run the normalization corpus
for all nine profiles, reject unknown active types and propagate resource-sink
failures. Maximum sensor layouts use correct uppercase compact card codes and
populate the actual sensor field. They prepare 30, 72, 210 or 380 distinct live
sensor channels for the respective grid profiles, and reject a channel budget
one below the requirement without leaving subscriptions behind. Cache budgets
in these fixtures are test inputs, not device memory limits.

A read-only comparison against the S3's saved layout checks declarations against
the live native list without changing firmware or configuration. It supplements
host coverage; physical layout, touch, complete global ownership and resource
peak qualification remain necessary before production wiring.
