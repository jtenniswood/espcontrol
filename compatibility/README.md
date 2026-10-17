# Compatibility Fixtures

These fixtures protect user upgrades. They describe saved panel configuration
shapes that must keep importing and round-tripping as EspControl changes.

- `current` covers the current button, subpage, layout, generated contract, and
  device-profile expectations.
- `legacy-v1` covers older saved button strings and backup imports that should
  continue to migrate safely.

Use these fixtures for saved button config strings, subpage config strings,
backup import/export behavior, cross-device layout adaptation, and legacy card
migrations. Do not change fixture expectations unless the user-facing migration
behavior is intentionally changing.

The [page runtime baseline](page-runtime-baseline.md) records the device capacity
contract and measurement procedure for the staged architecture migration.
