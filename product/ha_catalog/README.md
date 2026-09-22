# Home Assistant catalog contract

The EspControl integration owns this contract. `contract-lock.json` records the
exact upstream commit and SHA-256 of the JSON contract, protocol fixture,
TypeScript types/constants, and C++ constants vendored into this repository.

After the upstream contract is committed, synchronize from that checkout:

```sh
python3 scripts/sync_ha_catalog_contract.py --source-checkout /path/to/espcontrol-integration
```

Commit the lock and all copied artifacts together. Ordinary verification is
offline: `python3 scripts/sync_ha_catalog_contract.py`. It runs in fast, CI and
release checks. Never edit the vendored files to work around a mismatch.

`tests/web/unit/entity_catalog.test.js` consumes the same pagination fixture
that the integration runs through its native action and HTTP adapter. The JSON
provenance describes a feature-branch baseline, not an assertion that a released
firmware or installed display passed a hardware test.

This change depends on the native catalog implementation in PR #2023. Wire v1
remains compatible with the preceding integration; no pairing removal is needed
for contract adoption.
