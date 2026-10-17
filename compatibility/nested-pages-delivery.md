# Two-level subpage delivery

This follows the architecture programme in issue #1825. The feature is
Home → Subpage → Nested subpage, with the root at depth zero and a maximum
depth of two. Production remains at depth one until the prerequisites pass.

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
   replacements and reconnect invalidation.
4. **Reusable renderer:** give every card driver bind/update/unbind operations;
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
