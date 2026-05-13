---
title: Release Changelogs
description: Maintainer notes for building detailed release changelogs from git history.
---

# Release Changelogs

The GitHub release workflow updates release notes automatically when a release is published. It looks at the commits since the previous stable release tag, groups them by area, and saves the generated Markdown as the GitHub Release description.

## Automatic Release Notes

When a release is published, `.github/workflows/release.yml` runs the changelog script with the release tag:

```sh
python3 scripts/release_changelog.py "$VERSION" --output "$RUNNER_TEMP/release-notes.md"
```

The workflow checks out full git history so the script can find the previous release tag. It then edits the GitHub Release body with the generated notes.

## Preview Notes Locally

You can still preview notes before publishing a release:

```sh
npm run changelog:release -- v1.12.1
```

For a version that has not been tagged yet, the script compares `HEAD` with the latest stable tag, such as `v1.12.0`.

## Create Notes for an Existing Tag

If the release tag already exists, use the same command:

```sh
npm run changelog:release -- v1.12.0
```

When `v1.12.0` already exists as a tag, the script compares that tag with the previous stable release tag. This is the same mode the release workflow uses.

## Save the Changelog to a File

Use `--output` when you want a file you can review before publishing:

```sh
npm run changelog:release -- v1.12.0 --output release-notes/v1.12.0.md
```

`release-notes/` is only an example path. It is useful for drafting, but release notes do not need to be committed unless there is a reason to keep a copy in the repository.

## Override the Range

Use an explicit range if you need to rebuild notes for an unusual release:

```sh
npm run changelog:release -- v1.12.0 --from v1.11.0 --to main
```

This is helpful if a release was rebuilt, skipped, or based on a branch rather than the current `main`.

## Check the Tool

The changelog smoke test is part of the full local check:

```sh
npm run check:all
```

To run only the changelog test:

```sh
npm run check:release-changelog
```
