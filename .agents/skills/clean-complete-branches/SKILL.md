---
name: clean-complete-branches
description: >-
  Clean up completed Git branches and worktrees for this repository both
  locally and on GitHub. Use when the user asks to remove complete, completed,
  merged, stale, finished, or done branches, delete merged GitHub branches,
  tidy local worktrees, prune completed PR branches, or clean up branches and
  worktrees after pull requests have merged.
---

# Clean Complete Branches

## Overview

Clean up branch work safely after pull requests are complete. This skill covers:

- local worktree folders;
- local branch refs;
- matching `origin/*` remote branches on GitHub.

Use the bundled script for detection and cleanup. It dry-runs by default and
requires `--apply` before removing anything.

## Workflow

### 1. Check the Repository

Start from any worktree for this repository:

```bash
git status --short --branch
git worktree list
git fetch origin main --prune
```

Do not overwrite, revert, or remove local changes. If a worktree has
uncommitted changes, leave it alone and mention it in the final update.

### 2. Preview Complete Branches

Run the script in dry-run mode first:

```bash
python3 .agents/skills/clean-complete-branches/scripts/clean_complete_branches.py
```

The script treats a branch as complete only when:

- the branch is not protected, such as `main`, `master`, `develop`, `dev`,
  `release`, or `gh-pages`;
- the branch has no open GitHub PR;
- Git says the branch is already contained in `origin/main`, or GitHub reports
  a merged PR for that branch into `main`;
- any checked-out worktree for that branch has no uncommitted changes.

If GitHub CLI is unavailable or unauthenticated, the script still uses Git's
merge check. Squash-merged PR branches usually need GitHub CLI evidence because
their exact commits may not be ancestors of `main`.

### 3. Apply Cleanup

After reviewing the dry-run output, remove only the listed safe candidates:

```bash
python3 .agents/skills/clean-complete-branches/scripts/clean_complete_branches.py --apply
```

By default, `--apply` removes safe worktree folders and local branch refs. To
also delete matching GitHub remote branches, add `--delete-remotes`:

```bash
python3 .agents/skills/clean-complete-branches/scripts/clean_complete_branches.py --apply --delete-remotes
```

The script does not merge PRs, close issues, or delete protected branches.

### 4. Validate the Result

Confirm the cleanup:

```bash
git worktree list
git branch
git branch -r
git status --short --branch
```

Keep the final update practical: list what was removed, what was skipped, and
whether GitHub remote branches were deleted.

## Safety Rules

- Never remove `main`, the current worktree, dirty worktrees, detached
  worktrees, or branches with open PRs.
- Never use `rm -rf` for registered Git worktrees. Use `git worktree remove`.
- Do not delete GitHub remote branches unless the user asked to clean GitHub too
  and the script has shown them as complete in dry-run output.
- Do not close GitHub issues or merge pull requests as part of cleanup.
