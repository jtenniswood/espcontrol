---
name: clean-merged-branch-folders
description: >-
  Clean up local branch worktree folders for this repository after their
  branches or pull requests have merged to main. Use when the user asks to
  remove merged branch folders, tidy local worktrees, clean old feature branch
  directories, prune merged PR worktrees, or clean up local development folders
  without impacting main.
---

# Clean Merged Branch Folders

## Overview

Remove only local Git worktree folders whose branch is already merged to
`main`. Keep `main`, the current worktree, dirty worktrees, detached worktrees,
and unmerged branches untouched.

Use the bundled script for the candidate detection and removal. It dry-runs by
default and requires `--apply` before deleting any folder.

## Workflow

### 1. Check the Repository

Start from any worktree for this repository:

```bash
git status --short --branch
git worktree list
git fetch origin main --prune
```

Do not overwrite, revert, or remove local changes. If a worktree has uncommitted
changes, leave it alone and mention it in the final update.

### 2. Preview Cleanup Candidates

Run the script in dry-run mode first:

```bash
python3 .agents/skills/clean-merged-branch-folders/scripts/clean_merged_worktrees.py
```

The script treats a worktree as safe to remove when:

- the worktree is on a local branch, not `main` or another protected branch;
- the worktree is not the current working directory;
- the worktree has no uncommitted changes;
- the branch commit is already contained in `origin/main`, or GitHub reports a
  merged PR for that branch into `main`.

If GitHub CLI is unavailable or unauthenticated, the script still uses Git's
merge check. Squash-merged PR branches may require GitHub CLI because their
exact branch commits are often not ancestors of `main`.

### 3. Remove Merged Worktree Folders

After reviewing the dry-run output, remove only the listed safe candidates:

```bash
python3 .agents/skills/clean-merged-branch-folders/scripts/clean_merged_worktrees.py --apply
```

Leave local branch refs in place unless the user explicitly asks to delete them.
If branch deletion is requested, use:

```bash
python3 .agents/skills/clean-merged-branch-folders/scripts/clean_merged_worktrees.py --apply --delete-branches
```

The script uses `git branch -d`, not forced deletion, so branches that Git does
not consider merged are preserved.

### 4. Validate the Result

Confirm the cleanup:

```bash
git worktree list
git status --short --branch
```

Keep the final update plain and practical: list the folders removed, any folders
skipped, and whether local branch refs were kept or deleted.

## Safety Rules

- Never remove `main`, the current worktree, dirty worktrees, detached
  worktrees, or unmerged branches.
- Never use `rm -rf` for registered Git worktrees. Use `git worktree remove`.
- Never force-delete branches unless the user explicitly asks and the branch has
  been checked carefully.
- Do not close GitHub issues or merge pull requests as part of this cleanup.
