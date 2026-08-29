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

The core store depends only on a narrow `StorageBackend`. The first live
adapter uses an isolated `espcontrol_cfg` namespace in ESPHome's NVS
partition. On the 7-inch P4, it reserves its fixed-size blobs from PSRAM at
startup and writes only each slot's used bytes at the store's explicit
durability boundaries. This preserves the two-slot protocol without a
partition-table migration or reserving the full in-memory slot capacity in
NVS.

Place `ConfigurationService` above that store. The service wraps each payload
with an independently versioned document header, imports the existing entity
configuration through `LegacyConfigurationAdapter` when the new store is
empty, and persists that imported document immediately. While the legacy
entities remain authoritative, each boot refreshes the native shadow if the
editor has changed those entities. New native saves commit the atomic document
first and then mirror the same version and content through the legacy adapter.
A legacy mirror failure is reported separately while the new document remains
durable.

Document schema validation and the HTTP API remain separate layers.

## Migration

The 7-inch P4 profile now imports its restored button configuration, subpage
chunks, and grid order at boot. Its legacy text entities remain active. The
compatibility sequence is:

1. extend the same generated bindings to the remaining device profiles;
2. expose guarded native reads and writes while dual-writing the old format;
3. retain legacy import for the defined upgrade window; and
4. switch the web configurator only when a device advertises the new API.

## Consequences

- The last valid configuration survives interrupted payload and header writes.
- Storage format changes require a new envelope version or an explicit reader.
- Payload size is bounded by the platform adapter's fixed slot capacity.
- Firmware integrations must check every load and commit result explicitly.
- The durable document is never hidden by a failed compatibility mirror.
- The native document is a durable shadow copy for the 7-inch P4 until the
  guarded configuration API is enabled; the legacy entities remain the live
  runtime source during that period.
