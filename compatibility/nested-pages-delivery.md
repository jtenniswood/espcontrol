# Two-level subpage delivery

This follows the architecture programme in issue #1825. The feature is
Home → Subpage → Nested subpage, with the root at depth zero and a maximum
depth of two. Production writes and editor creation remain at depth one until
the compatibility and delivery prerequisites pass.

The runtime prerequisite in PR #2400 has user-confirmed layout coverage on the
6-, 9-, 15- and 20-slot grids. The follow-up adds depth-bounded direct readers,
prepared navigation, descendant indicators and remote lookup, with an editable
browser graph codec and subtree-transfer planning. Generated reader limits are
depth two; the advertised writable depth remains one. The web editor now loads
and navigates the graph and offers creation only at negotiated writable depths.
Browser fixtures cover creation, rejection rollback, reload and reconnect on all
four grid sizes. Clipboard operations save owner/order/subtree changes together;
cut removes its source only when paste commits. Version-two card codes retain
raw descendant pages, while flat codes remain version one. Graph backups keep
exact native-v2 bytes in outer format three so older editors reject them;
flat exports retain outer format two/native v1. Restore checks writer support
and publishes the whole graph before other settings. Physical nested testing and
durable-v2 release enablement remain separate gates.
A private, labelled S3 qualification image has now exercised the actual
embedded editor: creation inside an existing subpage, nested card save/reload,
immediate-parent Back, creation limited to two subpage levels, subtree code paste/delete
and exact native backup export/import. An old-format replacement is rejected
without changing the nested document or generation. These checks use an
unpublished test-only v2 writer; the production release contract is unchanged.
The S3 subsequently completed 1,000 navigation cycles, three reconnects and
bounded-resource checks, followed by an initial before/after idle memory cohort
and verified restoration of its original firmware, dashboard and settings.
Its initial synthetic Sensor fixtures were found to have unbound Sensor fields.
A corrected final-build pair now verifies all expected registrations and the
original firmware/data restoration. Later SDK2 checks populate the complete S3
128 KiB and 10-inch P4 512 KiB HA caches through same/disjoint replacements and
API reconnects. The original dashboard's historical memory regression, image
peaks and the remaining physical runtime gates still require investigation.
The [qualification record](nested-pages-qualification.md) gives the source scopes,
memory results, required reader bridge for old-firmware recovery and remaining
physical/visual gates. Release enablement remains unfinished.

The release contract records `panelConfigReadNavigationDepth` separately from
`panelConfigWriteNavigationDepth`. A flat native-v2 writer does not enable
nested placement. Depth-two writes require legacy-mirror retirement and a
published reader floor whose entire supported downgrade range explicitly
declares depth-two reading; format-two support alone is insufficient proof.
Reader depth is generated from that contract. This branch still declares
reader depth two, writer depth one, durable format one and no reader floor.
The service applies separate write validation to imports, shadow writes and
both save entry points. The declared write depth also constrains a future
flat-v2 writer: it may restore an existing nested document for reading, but
cannot publish a nested replacement while writable depth is one. Host tests
cover this distinction and reject writes before runtime or storage changes.

### Production release sequence

The latest published `v2.13.0` contract does not declare a v2 or depth-two reader.
The tested branch therefore needs the following stable-release sequence before
ordinary builds can create a subpage inside an existing subpage. Release names
are assigned when publishing; test fixtures and private images cannot satisfy
the published-history gate.

| Stage | Durable default / writable depth | Legacy mode | Required published evidence |
| --- | --- | --- | --- |
| Reader bridge | v1 / one | Dual-write | Publish this branch's v1/v2, depth-two readers with its runtime dependencies |
| First flat-v2 release | v2, with lossless v1 capacity fallback / one | Dual-write | Set the bridge as the minimum reader; verify every supported downgrade through that floor |
| Second flat-v2 release | Same / one | Dual-write | Retain the first flat-v2 release in the consecutive stable history |
| Nested-page release | v2, with lossless flat-v1 capacity fallback / two | Read/import-only | Verify both consecutive flat-v2 dual-write releases and depth-two readers throughout the supported downgrade range |

Keep the import adapter for at least the declared stable import-only window;
enabling nesting does not remove legacy import. Later import-only patches must
retain proof of the original dual-write window. An intervening release using
an older durable format or missing its manifest breaks that window rather than
being filtered out. Do not advance production policy using hypothetical tags
or edit historical release manifests to supply missing reader proof.

Legacy first-boot import and flat compatibility edits now request the selected
durable format from the adapter. Test-only v2-writer coverage exercises the real
service, text bindings and validator without enabling the production writer.
Conversion failures leave storage unchanged; unchanged native documents keep
their original bytes and generation. Flat edits retain inactive records and
unknown settings, and still carry v1 source provenance so they cannot replace
a nested graph. The v1 import path adds no conversion allocation.

For a graph without a complete legacy representation, native publication
validates and updates only the live root order and shared active colour. It
preserves per-card/subpage text, writes no preference mirror, and reports
`NATIVE_ONLY` separately from a complete legacy view. Saves, boot restore and
publication repair accept that outcome only after mirror retirement. Appearance
refreshes restore those globals from the native document without importing
stale text or committing another generation. Releases still promising a
downgrade mirror retain their full-mirror admission and replacement fences.

`GET /api/v1/backup` now preflights both native formats before sending JSON.
Graph exports use outer format three with exact native-v2 bytes and structured
pages, including inactive subtrees; native-v1 exports keep outer format two
and retain inactive records outside the readable grid. The request owns its
validation index, releases it on failure or disconnect, and streams escaped
fields/base64 in bounded chunks. Host HTTP tests and actual C++ output passed
through the browser importer cover these paths; physical backup qualification
remains part of the device gate.

Browser backup export also works on a depth-two reader whose writer still uses
v1: when the device rejects a lossy v1 export with HTTP 422, the browser requests
the complete v2 graph. Authentication, storage and memory failures still stop
export instead of falling back to a partial snapshot.

The generated contract now describes the logical two-level shape separately
from memory admission. The 6-, 9-, 15- and 20-slot grids allow at most
37/186, 82/657, 226/3,165 and 401/7,620 pages/cards respectively, including Home.
These are shape ceilings, not guaranteed full layouts. `/api/v1/capabilities`
reports them as `max_runtime_pages` and `max_runtime_cards`. Inactive records
remain preserved within the document budget rather than counting as runtime cards.
`max_durable_document_bytes` reports the selected storage backend's actual
saved-document limit, separately from the larger `max_v2_document_bytes`
transfer/conversion ceiling. The existing 40 KiB slots (or 16 KiB NVS fallback)
and storage headers are unchanged. This is a per-document ceiling, not a
reservation of free shared NVS space: existing preferences, compatibility text,
two native slots and garbage collection also consume that backend. A rejected
NVS blob write reports HTTP 413 and preserves the active document; an
unconfirmed commit remains an I/O error. A layout can fit the runtime arenas
and still exceed its saved-document allowance.
The same response advertises `max_page_store_bytes` (128 KiB S3, 256 KiB P4),
`max_state_metadata_bytes` (512 KiB S3, 2 MiB P4) and `max_ha_state_bytes`
(128 KiB S3, 512 KiB P4). State metadata covers the semantic models, graph
edges/names/indexes and local producer bindings; it excludes value caches,
prepared layouts, fixed owners, transport storage, widgets and image buffers.
Local values retain their separate 16 KiB ceiling. Key-byte and HA dependency
ceilings stay at the first-level runtime budgets. A prepared replacement must
fit all ceilings before publication. Local producer bindings allocate per
active/prepared graph, and failed preparation releases its own bindings.
Host tests cover larger-than-first-level layouts on all nine profiles, exact
metadata admission and rollback, and unchanged allocation for an unchanged
dashboard. These budgets still require physical qualification; writable depth
remains one and durable document version remains one.

Writer retirement must also preserve editing and restoring existing flat
dashboards that fit v1 but exceed the saved-document limit after v2 conversion.
The 400-card P4 climate fixture is 25,087 bytes in v1 and 53,407 bytes in v2,
against a 40,932-byte durable limit. A bounded format selector now retains v1
when the preferred v2 representation exceeds the actual saved-document limit.
A v2 request can use that fallback only if projection and reimport preserve
page/card IDs, values, raw text, import flags and record counts. Nested or lossy
projections are rejected; allocation failure never silently changes format.
Legacy import and edits use the same backend limit, excluding both headers.
Backups follow the response's actual durable version and retain exact stored
bytes, independently of the preferred writer. A v1 source remains fenced
against nested replacement; a v1-only writer keeps that fence for converted v2
requests too. Production still writes v1. Host capacity, HTTP, generation,
allocation-failure, legacy edit/reboot and browser checks pass. The private
10-inch future-flat-writer cohort also passes actual client edit/backup, old/new
v1 restores, restart and full-cache navigation, with exact original recovery;
see [its qualification scope](nested-pages-qualification.md#flat-storage-migration-on-the-10-inch-v3).
Reader-floor and mirror-retirement gates remain. Transaction crash recovery now
has automated abrupt-reset coverage on the 10-inch V3; electrical power-loss
behaviour remains separately unqualified.

PageStore preparation now starts with a smaller private parsing reservation and
grows within the existing profile ceiling, releasing an unsuccessful attempt
before growing. It compacts before resources borrow model data. This avoids
requiring a full 128/256 KiB contiguous block for a small edit; it does not
increase document admission or remove other preparation/image peak costs.

## Agreed behaviour

- Keep current grid sizes and one Back slot on every non-root page.
- Increase total page/card capacity within advertised document-byte and
  prepared-runtime resource budgets; do not promise a completely full tree.
- Back returns to the immediate parent. Keep the editor's existing label and
  Back-only appearance, without breadcrumbs.
- Automatic folder icon state includes eligible descendant controls.
  Explicit entity, numeric and text state keep their existing behaviour.
- Copy/move a folder with its subtree. Validate the entire transfer before
  applying it; prevent creation/paste of a third subpage level.
- Preserve `navigate` as home-card targeting and `close_subpage` as return
  Home. Extend `open_subpage(label)` to nested pages, preferring first-level
  matches and then nested matches in display order.
- Nested documents require a capable firmware/editor. Never silently flatten,
  truncate or accept an old-format replacement over nested configuration.

## Delivery order and gates

1. **Limits and baselines:** generated device contracts and memory capture are
   implemented. Complete owner counters, physical workload baselines and
   navigation timing before renderer cutover.
2. **PageStore shadow:** native-v1 arena model, interned strings, explicit
   targets and prepare/activate/discard lifecycle are implemented for host
   validation. Existing native/legacy configuration still drives production.
3. **Entity-state ownership:** move HA dependencies/state out of widgets,
   deduplicate registrations, and reclaim EspControl transport registrations
   without removing those owned by other components. Verify disjoint entity
   replacements and reconnect invalidation. The
   [transport reclamation prerequisite](ha-subscription-reclamation.md) now has
   host and S3 replacement coverage. The
   [configuration-owned state owner](page-entity-state-runtime.md) now has staged
   host coverage. The [card dependency declarations](page-card-dependencies.md)
   cover registered drivers in host tests. The branch now wires these owners
   into the selected native scene; full physical parity remains a release gate.
4. **Reusable renderer:** the [prepared binding/navigation owner](pooled-renderer-runtime.md)
   has host cycle/transaction coverage. Give every real card driver bind/update/unbind operations;
   bound widgets, contexts and modals by the visible profile, and guard delayed
   work with configuration/binding generations. These paths are implemented;
   qualify their physical steady/peak memory, delayed work and takeovers in
   addition to the existing navigation/layout results.
5. **Document v2:** negotiate explicit page/card/navigation records on the
   existing endpoint paths. Ship dual readers before durable v2; preserve the
   downgrade floor and compatibility-mirror release gates from #1825.
6. **Streaming persistence:** transient storage removes five full-slot buffers
   while preserving storage layout and atomic publication. Host coverage
   injects power cuts/read/write failures for partition and NVS paths. S3
   hardware coverage also injects NVS failures with a 16 KiB transient scratch
   reservation and verifies fresh-handle recovery/cleanup. A later P4 cohort
   passes 78 injected raw-driver cases at the deployed 40 KiB slot geometry in
   isolated erased sectors, verifying the entire partition after each cleanup.
   At the user's request, automated abrupt low-level software resets now
   replace manual power cuts for transaction crash-recovery qualification.
   Both before/after-publication checkpoints pass on the 10-inch P4 V3 with
   fresh boot witnesses, zero application shutdown calls, expected old/new
   payload selection and full-partition cleanup checksums. Original firmware,
   dashboard and settings are restored. See the
   [qualification scope](nested-pages-qualification.md#automated-abrupt-reset-recovery-6-november).
   The later [seven-inch V2 storage diagnosis](nested-pages-qualification.md#fifteen-slot-v2-storage-failure-diagnosis-7-november)
   leaves repeated maximum-size NVS saves unqualified: the real backend reports
   insufficient shared persistent space. Preserve that failure and address
   capacity handling before crediting the remaining physical grid gate.
   Electrical power loss, brownouts and supply collapse during flash operations
   remain separate, unqualified behaviours.
7. **Nested-page feature:** only after the preceding memory and compatibility
   gates, enable depth two through the generated contract, editor, validators,
   navigation, subtree operations, backups and remote page lookup.

Before enabling nesting, test at least 1,000 physical navigation cycles,
repeated reconnects and configuration replacements, delayed events, modal and
display takeovers, storage failures, matched memory workloads and supported
upgrades/downgrades. Image download/decode peaks must be recorded separately
from idle memory. Compile all device variants. Keep hardware-dependent PRs
open until physical testing is confirmed; keep issue #1825 open until the
feature works on devices.
