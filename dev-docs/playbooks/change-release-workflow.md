# Change the Release Workflow

Use this for draft publication, firmware asset assembly, release manifests,
public firmware URLs, release checks, or GitHub workflow behavior.

## Edit First

- `.github/workflows/release.yml` for job orchestration.
- `scripts/firmware_release.py` for firmware asset and manifest behavior.
- `scripts/check_release_contract.py` and focused release validators for
  assurance rules.
- `.github/esphome.env` for the single pinned ESPHome image/CLI version.

Keep release tags immutable and preserve existing device slugs, public URLs,
asset names, and manifest compatibility unless the change includes an explicit
migration.

## Protect the Publication Boundary

- Build and validate an existing private draft from its tagged revision.
- Keep generated source and build caches out of release assets.
- Verify filenames, versions, checksums, sizes, and remote inventory before
  publication.
- Leave the release as a draft after any build, upload, or verification failure.
- Do not publish a draft manually to bypass the workflow barrier.

## Stop If

- Different jobs can build different source revisions for one release.
- An installed panel could request an asset or manifest that no longer exists.
- Partial assets could become public before complete verification.
- A test replaces, rather than supplements, the private-draft safety barrier.

## Verify

```bash
python3 scripts/check_release_contract.py --self-test
npm run check:release-preflight
```

Run the manual `Firmware Compile` workflow for firmware-visible changes before
publishing. A full release still requires every supported firmware target to
compile in Docker from the tagged revision.
