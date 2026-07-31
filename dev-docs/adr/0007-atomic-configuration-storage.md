# ADR 0007: Atomic Versioned Configuration Service

## Status

Accepted.

## Context

EspControl currently persists cards and subpages through restored ESPHome text
entities. This remains the production compatibility path, but the versioned
device configuration service needs storage that cannot lose the last
working configuration when power is interrupted during a save.

The storage mechanism must work on constrained panels, avoid requiring a JSON
library, and allow the document format to evolve independently of its durable
envelope.

## Decision

Use `ConfigurationStore` as the durable storage boundary. It stores an opaque
payload in two fixed-capacity slots. Each slot has
a versioned header containing a generation, payload length, and CRC32 checksum.
The checksum covers the version, header size, generation, payload length, and
payload bytes so damaged selection metadata cannot promote a stale slot.

A commit invalidates the target first, then writes and syncs its payload and
header metadata. The envelope magic is written and synced separately as the
final publication marker, so a torn metadata write cannot combine a new
generation with stale size or checksum bytes. The other valid slot is not
modified. Reads validate both slots and select the newest valid generation; if
the newest slot is incomplete or corrupt, the previous valid generation is
returned.

The core store depends only on a narrow `StorageBackend`. The ESPHome adapter
will own flash preferences or partition access.

Place `ConfigurationService` above that store. The service wraps each payload
with an independently versioned document header, imports the existing entity
configuration through `LegacyConfigurationAdapter` when the new store is
empty, and persists that imported document immediately. New saves commit the
atomic document first and then mirror the same version and content through the
legacy adapter. A legacy mirror failure is reported separately while the new
document remains durable.

Document schema validation and the HTTP API remain separate layers.

## Migration

This service does not yet change production persistence or any Home Assistant
entity. The compatibility sequence is:

1. integrate the ESPHome storage and legacy-entity adapters;
2. import current text entities into the versioned document on first boot;
3. dual-write the old and new formats while rollback is supported; and
4. switch the web configurator only when a device advertises the new API.

## Consequences

- The last valid configuration survives interrupted payload and header writes.
- Storage format changes require a new envelope version or an explicit reader.
- Payload size is bounded by the platform adapter's fixed slot capacity.
- Firmware integrations must check every load and commit result explicitly.
- The durable document is never hidden by a failed compatibility mirror.
- The existing text-entity configuration remains authoritative until the
  migration service is integrated and physically tested.
