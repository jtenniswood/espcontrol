# ADR 0010: Central Firmware Owner

## Status

Accepted. Amends ADR 0006.

## Context

EspControl's long-lived runtime state was created through separate YAML globals
and reached directly from many lambdas. That makes ownership and lifecycle
implicit, and it prevents compiled services from sharing one stable application
boundary. ADR 0006 intentionally deferred a central component while behaviour
families were separated; the required foundations now exist.

## Decision

Introduce one compiled `EspControlApp` ESPHome component. It owns the
framework-independent `EspControlAppCore`, which in turn owns long-lived
firmware services. ESPHome calls the application boundary for setup, loop, and
shutdown. Device YAML keeps the stable `espcontrol_app` ID and acts as a wiring
and compatibility layer rather than owning service state.

The first migrated service is `DisplayModeController`. Existing display
behaviour and method names are unchanged; YAML reaches the controller through
`id(espcontrol_app).display()`. Later service migrations extend this owner one
focused pull request at a time and must not introduce generated `id(...)`
references into compiled modules.

The card-background feature subsequently adds `CardAssetService` to the same
owner. Image persistence and the card-background runtime are created and
released with `EspControlApp`; rendering and HTTP adapters reach that owner
through the service boundary instead of maintaining independent singletons.

The application core remains independent of ESPHome so ownership and lifecycle
are covered by executable host tests.

## Consequences

- There is one explicit lifetime for EspControl-owned services.
- Device-specific objects and callbacks still enter through narrow wiring
  adapters.
- YAML remains compatible during migration but no longer creates the display
  controller as an unrelated global.
- Adding a service requires an application accessor and executable lifecycle or
  behaviour coverage.
