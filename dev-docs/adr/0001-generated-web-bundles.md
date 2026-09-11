# ADR 0001: Generated Web Bundles

## Status

Accepted.

## Context

The setup page is authored under `src/webserver/`, but devices need a generated
`www.js` application that works without compiling source in the browser. Current
firmware includes an offline editor, while older firmware uses stable per-device
GitHub Pages URLs.

## Decision

Keep the complete generated web asset set as committed release artifacts. This
includes the embedded fallback, hosted bridge and manifest, immutable
content-addressed bundle, and per-device compatibility loaders. Do not replace
them with a runtime build step on the device.

## Why

- ESP32 devices should serve a simple, predictable setup page.
- GitHub Pages can host the release bundle reliably.
- Generated bundles make release diffs visible.
- Device profile data can be compiled into the shared application without
  asking the device to transform source files.

## Consequences

- Changes under `src/webserver/` must regenerate the asset set under
  `docs/public/webserver/`.
- Reviewers should expect large generated diffs after meaningful web changes.
- `npm run check:web-smoke`, `npm run check:web-asset-manifest`, and
  `npm run check:generated` are required guards.
