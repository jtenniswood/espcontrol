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

Use docs/development/release-versioning-improvements.md in EspControl as the acceptance spec. Keep the project-specific names, device slugs, chip families, GitHub Pages URL, ESPHome project name, Docker image version, and release asset names correct for this repository. Add local smoke tests and package scripts so the versioning and release helpers can be checked before a release.
```

## Required Outcome

The other project should end up with the same release guarantees, not just similar wording:

- a stable release is always a full tag like `v1.2.3`;
- the GitHub release tag is the only public firmware version source during release builds;
- development builds can still show `dev` or `0.0.0`, but release builds cannot;
- ESPHome `project.version`, the diagnostic version sensor, firmware manifests, release URLs, and web update information all agree on the release tag;
- release manifests are generated from the compiled binaries, not manually maintained;
- release assets are verified before upload and verified again after upload;
- the public website publishes firmware from the latest GitHub Release, not from stale committed binaries;
- the public website firmware URLs are verified after deployment;
- GitHub Release notes are generated from git history and published automatically;
- local smoke tests prove the helper scripts catch wrong versions, placeholder leaks, missing assets, bad checksums, and bad changelog ranges.

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

## Values To Replace

Do not copy the EspControl values blindly. Replace these for the target project:

| EspControl value | Replace with |
|---|---|
| `jtenniswood/espcontrol` | target GitHub owner and repository |
| `jtenniswood.espcontrol` | target ESPHome project name |
| `https://jtenniswood.github.io/espcontrol` | target GitHub Pages or public firmware base URL |
| `devices/manifest.json` | target project's device list source, or an equivalent generated slug list |
| each device slug | target release asset slug, used in `<slug>.factory.bin`, `<slug>.ota.bin`, and `<slug>.manifest.json` |
| each chip family, such as `ESP32-P4` or `ESP32-S3` | target chip family for each manifest build |
| `builds/<slug>.factory.yaml` | target ESPHome factory build file path |
| `ghcr.io/esphome/esphome:2026.4.5` | ESPHome build image the target project already uses or wants to standardize on |
| `dev` and `0.0.0` placeholders | target project's development placeholder versions |

## Migration Checklist

Use this order when porting the change to another project:

1. Identify every firmware build target and choose the release asset slug for each one.
2. Add or confirm an ESPHome `firmware_version` substitution in each factory build.
3. Set factory builds to a placeholder such as `0.0.0`.
4. Keep normal development packages on a clear development value such as `dev`.
5. Make ESPHome `project.version` use the `firmware_version` substitution.
6. Make any visible version sensor or web UI version source use the same substitution or ESPHome project metadata.
7. Add a release helper script with commands for `inject`, `manifest`, `verify-files`, `verify-directory`, and `verify-pages`.
8. Move manifest creation into the release workflow after firmware compilation.
9. Make release asset names predictable: `<slug>.factory.bin`, `<slug>.ota.bin`, and `<slug>.manifest.json`.
10. Add release workflow verification before uploading artifacts.
11. Add release workflow verification after uploading assets to the GitHub Release.
12. Update the Pages workflow to download firmware from the latest release and publish it under public firmware paths.
13. Add public URL verification after Pages deployment.
14. Add the release changelog generator and have the release workflow update the GitHub Release body.
15. Add package scripts and smoke tests for both helper scripts.
16. Update any local agent or maintainer release instructions so future releases use `vMAJOR.MINOR.PATCH` and verify the release action.

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

## Helper Command Contract

The exact filenames and paths can change per project, but the helper should provide the same behavior:

| Command | Required behavior |
|---|---|
| `inject` | Replace the factory YAML placeholder with the release tag before compiling. Fail if the expected placeholder is not present. |
| `manifest` | Read the compiled factory and OTA binaries, calculate the OTA checksum, and write the manifest for that specific build. |
| `verify-files` | Verify one device's manifest, factory binary, OTA binary, version strings, release URL, and checksum. |
| `verify-directory` | Verify all expected device assets in a downloaded or assembled release folder. |
| `verify-pages` | Download the public manifest and binaries from the deployed website and verify them the same way. |

The binary version check should look for the release tag in both general firmware strings and ESPHome project metadata. It should fail if the release binary still has placeholder project metadata such as `Project owner.project version dev` or `Project owner.project version 0.0.0`.

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

## Definition Of Done

The port is complete only when all of these are true:

- creating a published GitHub Release with tag `vX.Y.Z` starts the release workflow automatically;
- every compiled firmware binary contains `vX.Y.Z` as its ESPHome project version;
- no release binary reports `dev`, `main`, or `0.0.0` as its ESPHome project version;
- every GitHub Release asset has the expected slug-based filename;
- every manifest points to the matching OTA binary and release URL;
- downloading the assets from GitHub Releases and running the verifier succeeds;
- the Pages deployment publishes the same release firmware and public URL verification succeeds;
- GitHub Release notes are replaced with generated notes from the correct previous stable tag;
- local checks for firmware release helpers and release changelogs pass.
