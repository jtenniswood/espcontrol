# Shared command-policy routing

`PageCardControlRouter` composes the slider, timer and media state policies
behind the existing native command adapter. An application-owned immutable
table maps each typed command kind to one borrowed policy. The current eight
routes still feed the single eight-entry `PageCardCommands` FIFO; no new queues,
card records, subscriptions, timers or visible-shell fields are introduced.

Check `ready()` before publishing the scene and register every command kind that
the enabled card families can produce. It checks table structure, not complete
feature coverage. Null policies, duplicate kinds and basic/non-control kinds
make the table unusable. Missing routes reject that intent. A selected policy
can reject a wrong card/family; there is no permissive fallback to another
policy. Discard and accepted-send notifications follow the same exact route.

The router and every borrowed policy live for the application lifetime. Pass
its address as the native adapter's control policy; the scene's accepted-control
hook separately validates visible seek feedback. Resource ordering stays:
command owner, existing histories/media, then graph/store. Routing itself has
no prepare/activate state, UI callbacks, string ownership or allocation.
Storage is fixed by enabled command kinds rather than configured card count.

The host fixture uses the actual graph, all five history owners, three policies
and one FIFO on each of the nine profiles. It runs 1,000 mixed six-action cycles
per profile and checks order, allocation-free admission, timer acceptance history,
stable arena/registrations, one shared overflow limit, wrong-family/table
rejection, failed/cancelled rollback, reconnect, mid-queue service loss and
reentrant same-generation replacement after poisoning the retired model arena.
These are host tests with a fake sender; they are not a physical device test or
an assembled production scene.

Scene composition, remaining device-command routes, production selection,
factory builds and S3/working 20-slot P4 memory/interaction qualification remain
required before nested pages. Documents, backups and depth are unchanged.
P4-86 remains reserved for separate USB diagnosis.
