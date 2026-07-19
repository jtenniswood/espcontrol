# ADR 0008: Typed Web State Boundary

## Status

Accepted.

## Context

The web configurator was split from one large browser script into TypeScript
modules, but those modules still accessed mutable application state through a
`state` property installed on `globalThis`. Its ambient declaration used
`any`, so TypeScript could not check subpages, clipboard data, settings drafts,
or state property access across the application.

## Decision

Application modules, card modules, and test-hook adapters import the live
`state` binding directly from `state/app_instance.ts`. The entry point
initializes that binding before installing application modules but does not
publish either the state object or its initializer as browser globals.

Transient state structures used across modules have explicit contracts:

- runtime subpages;
- copied-card clipboard entries;
- the card-settings draft.

The browser smoke test executes the built bundle and rejects any regression
that exposes `state` on the global object. Other legacy globals remain as
compatibility seams and should be removed in similarly testable vertical
slices.

## Consequences

- Shared state property access is checked against `AppState` in every consumer.
- Internal mutable state is no longer writable by unrelated page scripts.
- The generated bundle remains a normal non-module browser script and keeps
  its existing URLs and test-hook interface.
- New state consumers must use the module import; adding an ambient `state`
  declaration is not supported.
