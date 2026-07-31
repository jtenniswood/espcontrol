# ADR 0009: Atomic Draft Releases

## Status

Accepted.

## Context

The firmware workflow previously started after a GitHub release was already
public. Device builds then replaced assets individually. During a slow or
failed build, users could see an empty release, a partial device set, or files
from different attempts.

Build caches, per-device outputs, generated source files, and public release
assets also used overlapping generic directory names, making their ownership
harder to distinguish.

## Decision

Use an immutable tag and a private GitHub draft as the input to the release
workflow. Each device job places only its three publishable files in
`dist/firmware/`:

- factory image;
- OTA image;
- firmware manifest.

The workflow merges every device artifact into one distribution, generates
release notes, verifies the full local bundle, uploads it to the draft, and
compares the remote names and byte sizes with the local files. Only the tested
publisher may change the release from draft to public.

The root `dist/` directory is ignored and reserved for publishable output.
Tracked generated code remains beside its consumers because it is a build
input rather than a release artifact.

## Consequences

- Users cannot discover a release until its complete firmware set is present.
- Failed builds and partial uploads leave a recoverable draft.
- Reruns may replace expected draft assets, but unexpected stale assets block
  publication.
- Release creation now requires an explicit workflow dispatch after the tag
  and draft exist.
- GitHub Pages deployment still starts from the successful `Build Release`
  workflow and therefore sees only published, verified assets.
