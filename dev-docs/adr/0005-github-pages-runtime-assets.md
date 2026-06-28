# ADR 0005: GitHub Pages Runtime Assets

## Status

Accepted.

## Context

Installed panels need public firmware manifests, OTA binaries, web setup
bundles, docs, and device profile metadata. The project publishes these through
GitHub Pages and GitHub releases.

## Decision

Keep GitHub Pages as the public runtime asset host for web bundles, install
metadata, generated docs, and firmware update metadata.

## Why

- It keeps the install and update path simple for users.
- Public URLs are stable and inspectable.
- Release artifacts can be validated before users update panels.

## Consequences

- Device slugs and public paths are compatibility-sensitive.
- Release checks must protect firmware manifests and public web bundle paths.
- Changing a public path requires a migration plan, not just a docs update.
