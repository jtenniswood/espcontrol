# ADR 0001: Generated Web Bundles

## Status

Accepted.

## Context

The setup page is authored under `src/webserver/`, but devices load a single
`www.js` bundle for their supported screen profile. The production bundles live
under `docs/public/webserver/<slug>/www.js`.

## Decision

Keep generated per-device web bundles as committed release artifacts. Do not
replace them with a runtime build step on the device.

## Why

- ESP32 devices should serve a simple, predictable setup page.
- GitHub Pages can host the release bundle reliably.
- Generated bundles make release diffs visible.
- Device-specific profile data can be compiled into the bundle without asking
  the device to transform source files.

## Consequences

- Changes under `src/webserver/` must regenerate `docs/public/webserver/*/www.js`.
- Reviewers should expect large generated diffs after meaningful web changes.
- `npm run check:web-smoke` and `npm run check:generated` are required guards.
