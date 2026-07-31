# ADR 0006: Hybrid Compiled Firmware Modules

## Status

Accepted. Supersedes ADR 0003.

## Context

The firmware UI grew as a set of focused headers included through
`button_grid.h`. That kept ESPHome integration simple, but behaviour-heavy
headers increase every generated source file's compile workload and make
private state look globally available. The firmware must keep its existing
YAML helpers, saved configuration, fixed-capacity storage, and device behaviour
while implementations move gradually.

## Decision

Adopt a hybrid structure with this dependency direction:

1. foundation headers for shared limits, strings, and narrow data contracts;
2. behaviour-family interfaces and compiled `.cpp` implementations;
3. the grid dispatcher and visual adapters;
4. `button_grid.h` as the YAML compatibility facade.

`button_grid.h` remains the supported include and may contain includes only,
not behaviour implementations. YAML-facing helpers keep their current global
signatures through forwarding wrappers. Internal helpers move into the owning
behaviour namespace without compatibility wrappers. Compiled modules receive
required objects or callbacks and do not reference generated `id(...)` values.

The migration is incremental: one behaviour family per pull request, beginning
with date/time. Headers remain appropriate for templates, small adapters, and
shared inline foundations. A central ESPHome component class is not introduced.

## Consequences

- Existing YAML and saved configuration remain compatible throughout the
  migration.
- Each compiled unit must receive the same feature flags and conditional build
  settings as the compatibility facade.
- Private registries can move into `.cpp` files without changing capacity,
  allocation strategy, or lifetime.
- Host checks must compile all participating translation units and source
  inspection must include both `.h` and `.cpp` files.
- Each family migration requires clean display builds, regression comparison,
  automated review, and physical testing before the next family begins.
