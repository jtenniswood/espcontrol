# Transactional live configuration

This document records the migration boundary for applying card configuration
without restarting the display. The user-facing saved-card schema remains
compatible throughout the migration.

## Invariants

- A save is one versioned configuration revision, not a sequence of unrelated
  text-sensor updates.
- A revision is validated and planned before it changes the active screen.
- The active card graph remains usable until the replacement revision can be
  committed.
- Card resources have one owner and one deterministic release path. A late
  callback must be harmless after that owner is released.
- Repeated edits are bounded by the compiled grid and subpage limits. Runtime
  state must not retain every card or Home Assistant entity seen since boot.
- Appearance and layout changes do not recycle Home Assistant state channels.
- Entity binding changes can settle asynchronously while the display and web
  server stay running.
- The configuration event stream carries revision notifications. The complete
  document is obtained from the snapshot endpoint, so reconnecting never
  depends on a partially delivered series of entity events.

## Target flow

1. The configurator reads a snapshot and its revision.
2. It writes a complete candidate document with the expected revision.
3. The firmware rejects stale or invalid candidates before applying them.
4. The candidate is converted to an immutable, fixed-capacity `CardGraph`.
5. The reconciler produces typed add, remove, replace, rebind, visual and layout
   operations.
6. Owned resources are prepared, then the revision is committed. If preparation
   cannot safely complete, the prior graph remains active and the newest pending
   revision is retried.
7. Legacy text entities are mirrored during the compatibility period rather
   than acting as the transaction boundary.

## Migration slices

The implementation is deliberately staged so each pull-request commit can be
tested independently:

1. Add the fixed-capacity graph and pure change planner.
2. Expose the existing durable `ConfigurationService` through revisioned
   snapshot/save operations and mirror legacy entities.
3. Adapt parsed main-grid and subpage cards into the graph and apply plans using
   owned `CardInstance` lifetimes, starting with media and cover art.
4. Move Home Assistant state reads behind reference-counted broker leases.
5. Replace configuration enumeration over general SSE with revision events and
   snapshot recovery.
6. Generate lifecycle metadata and runtime inventory from the card contract.
7. Remove the compatibility path only after saved configurations and hosted web
   bundles no longer require it.

## Verification gates

Planner tests are pure host-side tests. Later slices add lifecycle trace and
fault-injection tests, stale revision and reconnect tests, one hundred-edit
memory tests, browser compatibility tests, and compilation of every supported
display. Physical S3 testing remains separate from automated verification and
is required before the pull request is merged.
