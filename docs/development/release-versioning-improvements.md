---
title: Release Versioning Improvements
description: Portable maintainer notes for release tag versioning, firmware manifests, checks, and release notes.
---

# Release Versioning Improvements

This page documents the release hardening added around the `v1.12.x` releases. It is written as a reusable brief, so the same pattern can be requested for other ESPHome firmware projects.

## Copyable Request

Use this when asking for the same work elsewhere:

```text
Please implement the release hardening from EspControl in this repository.

Use long semantic version tags like v1.2.3 for stable releases, and treat the GitHub release tag as the source of truth for the public firmware version. Inject that version into ESPHome at build time, generate firmware manifests after the firmware is built, verify that the manifest and firmware binaries contain the correct version, publish detailed release notes from git history, and verify the uploaded release assets and public firmware files after deployment.

Keep the project-specific names, device slugs, chip families, GitHub Pages URL, and release asset names correct for this repository. Add local smoke tests and package scripts so the versioning and release helpers can be checked before a release.
```

## What Changed

The important change is that the release tag is now the source of truth. The firmware build files keep placeholder versions for day-to-day development, and the release workflow replaces those placeholders with the real release tag only when it builds release firmware.

Stable releases use full tags such as `v1.12.3`. Short tags such as `v1.12` should be normalized to `v1.12.0` before publishing. Pre-releases can still use a suffix such as `v1.13.0-beta.1`, but they should not be used as the base when choosing the next stable version.

The release workflow now creates the firmware manifest after compilation, using the files that were actually built. This matters because the OTA `md5`, asset paths, and `release_url` are generated from the real output instead of being hand-maintained.

The workflow verifies the result before publishing it. It checks that the manifest version matches the release tag, that the OTA checksum is correct, that the expected factory and OTA files exist, and that the firmware binary contains the release version in ESPHome project metadata.

The workflow also rejects common placeholder leaks. A release build should not still advertise `dev` or `0.0.0` as the ESPHome project version.

After the firmware assets are uploaded to GitHub Releases, the workflow downloads them again and verifies the downloaded copies. This catches upload mistakes, renamed files, bad manifests, and checksum mismatches before users receive the release.

The docs deployment now downloads firmware from the latest GitHub release, places it under the public `firmware/<device>/` paths, and verifies the public GitHub Pages URLs after deployment. That final check confirms the files users install from the website are the same version as the release.

Release notes are generated automatically from git history. When a release is published, the workflow compares it with the previous stable tag, groups commits by practical area, and replaces the GitHub Release body with detailed notes.

## Files That Matter

These are the main pieces to copy or adapt in another similar project:

| File | Purpose |
|---|---|
| `.github/workflows/release.yml` | Builds release firmware from the release tag, creates manifests, uploads assets, verifies published assets, and updates release notes. |
| `.github/workflows/pages.yml` | Downloads release firmware into the public website and verifies the deployed firmware URLs. |
| `scripts/firmware_release.py` | Shared helper for version injection, manifest generation, local asset verification, release asset verification, and public Pages verification. |
| `scripts/check_firmware_release.py` | Smoke tests for the firmware release helper. |
| `scripts/release_changelog.py` | Builds detailed GitHub Release notes from commits since the previous stable tag. |
| `scripts/check_release_changelog.py` | Smoke tests for the changelog generator. |
| `package.json` | Adds easy commands such as `check:firmware-release`, `check:release-changelog`, and `changelog:release`. |
| `.agents/skills/release/SKILL.md` | Optional Codex release instructions, including version bump rules and release verification steps. |

## ESPHome Version Pattern

The factory build YAML uses a placeholder:

```yaml
substitutions:
  firmware_version: "0.0.0"

esphome:
  project:
    name: owner.project
    version: "${firmware_version}"
```

The normal package can keep a development value such as `dev`, but release builds should not rely on that default. The release workflow should inject the tag into the factory YAML before compiling:

```sh
python3 scripts/firmware_release.py inject \
  --slug "$DEVICE_SLUG" \
  --version "$VERSION"
```

That makes the device's own project metadata, diagnostic version sensor, update manifest, and release asset version agree with each other.

## Release Workflow Pattern

A hardened firmware release should do these steps in order:

1. Check out the repository at the release tag.
2. Set `VERSION` from the GitHub release tag.
3. Inject `VERSION` into the ESPHome factory build file.
4. Compile the firmware.
5. Copy the factory binary and OTA binary into a clean output folder.
6. Generate the manifest from the compiled files.
7. Verify the manifest, binaries, checksum, paths, and embedded firmware version.
8. Upload one artifact per device build.
9. On the release event, download all build artifacts and upload them to the GitHub Release.
10. Download the uploaded release assets back from GitHub and verify them again.

For this project, the helper verifies these details:

- `manifest.version` equals the release tag.
- `home_assistant_domain` is `esphome`.
- `ota.path` is the expected `<device>.ota.bin` file.
- `ota.md5` matches the actual OTA binary.
- `ota.release_url` points to the GitHub Release for the tag.
- factory parts point to the expected `<device>.factory.bin` file at offset `0`.
- the factory and OTA binaries contain the release version.
- the binaries contain ESPHome project metadata for the release version.
- the binaries do not still contain placeholder project metadata for `dev` or `0.0.0`.

## Pages Deployment Pattern

If the project serves firmware through GitHub Pages, the docs deployment should not use stale committed firmware files. It should download the latest release assets during the Pages workflow and publish those files into the static site output.

The Pages workflow should then verify the deployed public URLs, not just the local build folder. EspControl does that with retries because GitHub Pages can take a short time to serve the newly deployed files.

## Release Notes Pattern

The release notes generator should:

- use only stable tags matching `vX.Y.Z` when finding the previous release;
- compare a new release with the latest reachable stable tag;
- compare an existing tag with the stable tag before it;
- include a full comparison link;
- group commits into useful sections;
- link commits and pull requests where possible;
- be run automatically by the release workflow after the release is published.

You can preview the release notes locally:

```sh
npm run changelog:release -- v1.12.3
```

## Checks To Keep

Run these before changing the release process:

```sh
npm run check:firmware-release
npm run check:release-changelog
```

They are also included in:

```sh
npm run check:all
```

For another project, add equivalent smoke tests before relying on the release automation. The tests do not need to compile real firmware; they should create small fake binaries containing the version strings and prove that the helper catches wrong versions, placeholder leaks, bad checksums, missing assets, and bad changelog ranges.
