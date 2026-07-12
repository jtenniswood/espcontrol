# Source-text check migration ledger

This ledger records why source-text checks exist before any of them are retired.
The migration status is deliberately conservative: every current rule remains in
place until a behavioural replacement and a deliberately broken mutation prove
equivalent protection.

| Rule family | Defect protected against | Replacement layer | Mutation case | Status |
|---|---|---|---|---|
| `check_firmware_ha_bindings.py` Home Assistant read, callback and subscription shapes | Lost updates, unsafe callback lifetime, duplicated reads, stale reconnect work | Host C++ `HaReadCoordinator` executable tests | Disconnect, low heap, re-entry, duplicate read, reconnect, cancel and stale generation | Planned in stage 2; old rule retained |
| `check_firmware_parser.py` extracted parser source | Saved configurations becoming unreadable or normalising differently | Host C++ tests compiling the real parser headers with shared JSON fixtures | Legacy input rejected or canonical bytes changed | Planned in stage 3; old rule retained |
| `check_firmware_card_runtime.py` runtime source patterns | Card actions or rendering paths silently disappearing | Focused host C++ domain tests | Remove the protected action or state transition | Awaiting executable domain migration (#1007); old rule retained |
| `check_firmware_modals.py` modal source patterns | Broken navigation, modal lifecycle, controls or generated wiring | Host C++ state tests and browser interaction tests | Remove lifecycle transition or generated binding | Awaiting executable domain migration (#1007); old rule retained |
| `check_firmware_display_tokens.py` display token scanning | Unknown or misplaced display substitutions reaching firmware | Structural schema/token validator | Add an invalid token at a guarded location | Genuine structural rule; retain |
| `check_cover_art_contract.py` implementation/source checks | Cover-art downloads or cleanup violating memory and lifecycle contracts | Host C++ lifecycle tests | Remove cleanup or accept an invalid transfer | Awaiting executable domain migration (#1007); old rule retained |
| `check_timezones.py` `sun_calc.h` source checks | Timezone lookup or solar calculations losing required boundaries | Pure C++ timezone/solar executable tests | Break a boundary or known location result | Awaiting executable domain migration (#1007); old rule retained |
| `check_device_profiles.py` generated/source assertions | Device capability data and generated packages drifting apart | Schema and generated-output comparison | Change a profile without regenerating outputs | Mixed structural/output contract; retain structural rules |
| `check_card_contract_outputs.py` generated text assertions | Firmware, web and docs consuming different card contracts | Generator snapshot equality | Change one generated consumer only | Genuine generated-output rule; retain |
| Web smoke scripts using source transformation or pattern assertions | Setup behaviour diverging without browser coverage | `node:test` module tests and Playwright interactions | Remove validation, import, event or persistence behaviour | Awaiting web migration (#1007); old rule retained |

Assertions that protect only a symbol name, formatting choice, log wording, or one
valid implementation shape will be marked **intentionally retired** when their
domain migration reaches the mutation-overlap step. None are removed in stage 1.
