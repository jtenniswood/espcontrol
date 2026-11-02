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
No nested-page candidate has been flashed to a physical device yet.

The generated contract now describes the logical two-level shape separately
from memory admission. The 6-, 9-, 15- and 20-slot grids allow at most
37/186, 82/657, 226/3,165 and 401/7,620 pages/cards respectively, including Home.
These are shape ceilings, not guaranteed full layouts. `/api/v1/capabilities`
reports them as `max_runtime_pages` and `max_runtime_cards`. Inactive records
remain preserved within the document budget rather than counting as runtime cards.
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
   cover registered drivers in host tests; production ownership wiring, full
   parity and renderer cutover remain.
4. **Reusable renderer:** the [prepared binding/navigation owner](pooled-renderer-runtime.md)
   has host cycle/transaction coverage. Give every real card driver bind/update/unbind operations;
   bound widgets, contexts and modals by the visible profile, and guard delayed
   work with configuration/binding generations. Physically qualify the S3 and
   a 20-slot P4, then check all remaining grid sizes.
5. **Document v2:** negotiate explicit page/card/navigation records on the
   existing endpoint paths. Ship dual readers before durable v2; preserve the
   downgrade floor and compatibility-mirror release gates from #1825.
6. **Streaming persistence:** remove five full-slot buffers while preserving
   storage layout, atomic publication and power-loss recovery for partition
   storage and NVS fallback.
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
