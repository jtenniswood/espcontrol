---
title: Release Changelogs
description: Maintainer notes for building detailed release changelogs from git history.
---

# Release Changelogs

Use the release changelog script before publishing a GitHub Release. It looks at the commits since the previous stable release tag, groups them by area, and creates Markdown that can be used as the release description.

## Create Notes for the Next Release

For a new release that has not been tagged yet, pass the version you plan to publish:

```sh
npm run changelog:release -- v1.12.0
```

The script compares `HEAD` with the latest stable tag, such as `v1.11.1`. This is the normal workflow when preparing release notes from `main`.

## Create Notes for an Existing Tag

If the release tag already exists, use the same command:

```sh
npm run changelog:release -- v1.12.0
```

When `v1.12.0` already exists as a tag, the script compares that tag with the previous stable release tag.

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
