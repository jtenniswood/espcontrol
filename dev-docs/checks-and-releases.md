# Checks and Releases

This page describes how verification and release safety work. Use the generated
[Check Matrix](check-matrix.md) or a [Task Playbook](playbooks/README.md) for the
exact commands required by a change. Tool installation belongs in
[Development Environment](development-environment.md).

## Dependency-Aware Check Graph

The public npm check commands enter the dependency-aware task graph. A focused
task automatically includes its declared prerequisites; the product, fast, CI,
all, and release profiles run broader assurance sets. The release profile covers
firmware host-service tests, cross-language saved-configuration parity, browser
journeys, device-matrix validation, generated outputs, documentation, and
release manifests.

`scripts/check_tasks_data.py` is the maintained source for task commands,
dependencies, profiles, domains, input paths, cache inputs, tool requirements,
and parallel safety. `scripts/check_tasks.py` plans and executes that registry.
It can list registered tasks or explain a profile before running it.

## Parallel Execution

Normal npm aliases and CI use one worker. The explicit `check:parallel` entry
allows no more than four workers, and only dependency-independent tasks marked
parallel-safe may overlap. Browser, release, Git-state, and shared-output checks
always run alone; release profiles remain single-worker.

After the first failure, no new tasks start. Already-running tasks finish and
dependent tasks are reported as blocked. The decision to keep parallel mode
opt-in is supported by the dated
[parallel-check benchmark](history/parallel-check-benchmark.md).

## Deterministic Result Cache

Successful deterministic local checks are cached by content in the repository's
shared Git directory, so linked worktrees can reuse results. A key includes the
task and command, dependency keys, declared authored and generated inputs,
runner and registry code, lockfiles, platform, tool versions, and declared
environment variables. Any change to those values causes a fresh check.

Only successful results are stored, corrupt entries are misses, and checks that
depend on Git history, release state, external state, or shared output are never
cached. Browser smoke is cacheable only when Playwright, Node, Chromium, layouts,
and web inputs are all fingerprinted. `CI=true` disables the result cache so CI
always runs from scratch.

The task runner supports cache status, cache clearing, and a `--no-cache` option
for deliberate fresh execution.

## Changed-Path Planning

The changed-path planner considers committed, staged, unstaged, renamed,
deleted, and untracked paths relative to `main`. Unknown paths and changes to
shared helpers, generators, validators, the task runner, registry, lockfile, or
workflow definitions select the complete fast profile. Domain filters can
narrow a deliberately selected CI profile, but changed-path planning never
reduces CI coverage.

## Confidence Levels

Playbooks use three consistent levels:

- Minimum proves the narrow contract touched by a change.
- Recommended runs the normal product-level safety net for that workflow.
- Release-grade adds broad checks, browser journeys, documentation builds, or
  firmware compiles when release-facing behavior is involved.

A successful compile is automated evidence, not physical-device confirmation.
Record flashing and device behavior separately in the pull request.

## Release Boundary

Firmware releases start as private GitHub drafts. The manually dispatched build
workflow checks out one immutable tag in every job, builds every supported
target, and writes only publishable files to the distribution. Generated source
and build caches are not assets.

The workflow verifies every manifest, embedded version, expected filename,
checksum, byte size, and the final remote asset inventory while the release is
still private. Publication occurs only after the complete remote inventory
matches the verified local distribution. Any build, upload, or verification
failure leaves the release as a draft.

Current generated-output ownership is listed in
[Source of Truth Contract](source-of-truth.md); upgrade-sensitive public names
and formats are listed in [Compatibility Contract](compatibility-contract.md).
Use [Change the Release Workflow](playbooks/change-release-workflow.md) for the
exact edit and verification procedure.
