---
name: resolve-pr-conflicts
description: >-
  Resolve merge conflicts in an open pull request for this repository. Use when
  the user says "/resolve-pr-conflicts", asks to fix PR conflicts, says a pull
  request cannot merge because it is behind main, asks to update a PR branch
  from main, or asks to make a conflicted PR testable again.
---

# /resolve-pr-conflicts

## Overview

Resolve pull request merge conflicts without merging into `main`. Work on the
PR branch or a maintainer-owned conflict-fix branch, commit the resolved files,
push the branch, and leave the PR open for user testing.

## Workflow

### 1. Identify the PR

Use the PR number or URL if the user provides one. Otherwise inspect open PRs
and choose the newest conflicted PR, clearly saying that assumption.

Prefer GitHub connector tools when available. Otherwise use `gh`:

```bash
gh pr list --state open --limit 20 \
  --json number,title,author,headRefName,headRepositoryOwner,baseRefName,mergeStateStatus,isCrossRepository,url,updatedAt
gh pr view <number> \
  --json number,title,body,author,headRefName,headRepositoryOwner,baseRefName,mergeStateStatus,isCrossRepository,maintainerCanModify,url,files,statusCheckRollup
```

Record the PR URL, base branch, source branch, whether the PR is from a fork,
whether maintainers can modify it, and the current check state.

### 2. Protect Local Work

Check the current checkout before changing branches:

```bash
git status --short --branch
git worktree list
```

Do not overwrite or revert unrelated local changes. Prefer a new worktree for
conflict work, especially when the main checkout has local files.

Use a worktree from the PR branch when it can be pushed:

```bash
git fetch origin <base-branch>
gh pr checkout <number> --branch <local-pr-branch>
git worktree add ../<repo>-pr-<number>-conflicts <local-pr-branch>
```

If the PR comes from a fork and cannot be pushed safely, create a same-repo
branch from the PR head instead and tell the user the original PR may need a
replacement PR:

```bash
gh pr checkout <number> --detach
git switch -c <maintainer-conflict-branch>
```

### 3. Bring in the Base Branch

Fetch the latest base branch and merge it into the PR branch. Do not merge the
PR into `main`.

```bash
git fetch origin <base-branch>
git merge origin/<base-branch>
```

If the repository convention prefers rebase and the PR has not been shared or
reviewed, rebase is acceptable. For existing shared PRs, prefer merge because it
does not rewrite the contributor's history.

### 4. Resolve Conflicts Deliberately

List conflicted files:

```bash
git status --short
git diff --name-only --diff-filter=U
```

For each file:

- Read both sides of the conflict and the surrounding current code.
- Keep the intended PR behavior while preserving newer changes from the base
  branch.
- Remove all conflict markers: `<<<<<<<`, `=======`, `>>>>>>>`.
- Avoid broad formatting or unrelated cleanup while resolving conflicts.
- Regenerate derived files only when the repository has an established generator
  for the affected output.

Use structured tools for structured formats where practical:

- YAML: preserve indentation, anchors, substitutions, and comments.
- JSON/package lock files: prefer the package manager or lockfile-aware tooling.
- Generated firmware, manifests, icons, or docs output: run the matching
  generator/check command instead of hand-editing large generated sections.

When the right resolution is unclear, inspect the original PR diff and recent
base-branch changes:

```bash
gh pr diff <number> --patch
git log --oneline --decorate -- <file>
git diff ORIG_HEAD...HEAD -- <file>
```

### 5. Run Focused Checks

Run checks that match the files touched by the conflict. Common commands in this
repository include:

```bash
npm run check:config
python3 scripts/build.py --check
python3 scripts/generate_device_slots.py --check
python3 scripts/check_icon_groups.py
python3 scripts/check_timezones.py
npm run docs:build
```

Use the repo-local `compile` skill when firmware YAML, common YAML, device YAML,
or C++ component changes need compile-level confidence.

If a check cannot be run locally, say why and name what remains unverified.

### 6. Commit and Push

Confirm only intended files changed:

```bash
git status --short
git diff --check
git diff --stat
```

Commit the conflict resolution with a plain message:

```bash
git add <resolved-files>
git commit -m "Resolve PR <number> merge conflicts"
git push
```

If working on a newly created same-repo branch, push with upstream and open a
draft PR into the original base branch, explaining that it carries the original
PR plus conflict resolutions.

Do not merge the PR, close linked issues, or mark device testing complete unless
the user explicitly confirms the tested result.

## Output Format

Keep the final update approachable and practical:

```text
Resolved conflicts for PR #<number>: <title>

What changed:
- <plain-English summary of the conflict resolution>

Checks run:
- <command>: <pass/fail/skipped and why>

Pushed:
- <branch or PR URL>

What to test:
- <device/display or workflow the user should test before merge>
```
