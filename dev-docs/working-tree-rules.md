# Working Tree Rules

Use these rules before editing, staging, committing, or pushing changes from this
repository.

## Main Rule

Do not stage, revert, clean, reset, or rewrite unrelated local changes.

Local changes may be work in progress from another task. Treat them as someone
else's work unless the current request explicitly says to modify them.

## Start Every Change by Checking Scope

Before editing files, inspect the current state:

```bash
git status --short --branch
```

If the worktree is dirty:

- identify which files are already changed
- decide whether each changed file is part of the current task
- leave unrelated files untouched
- do not use broad cleanup commands to make the tree look clean

## Staging Rules

Stage only files that belong to the current change.

Prefer explicit paths:

```bash
git add dev-docs/working-tree-rules.md
git add dev-docs/README.md
```

Avoid broad staging commands when unrelated changes exist:

```bash
git add .
git add -A
```

If a file contains both current-task edits and unrelated edits, stop and inspect
the diff before staging. Either split the change carefully or leave that file out
of the commit.

## Commands to Avoid

Do not run these to manage unrelated local work:

```bash
git reset --hard
git checkout -- <path>
git restore <path>
git clean -fd
git clean -fdx
```

Use them only when the current request explicitly asks for that exact cleanup and
the affected paths are understood.

## Generated Files

Generated files should only be committed when the current source change caused
the generator to update them.

If generated files change unexpectedly:

- inspect the generator input that caused the change
- compare against [Source of Truth Contract](source-of-truth.md)
- do not stage the generated output just because it exists
- rerun the relevant `--check` command after deciding whether the change belongs

## Before Committing

Run this checklist:

- [ ] `git status --short` shows only intended files staged.
- [ ] Unrelated modified or untracked files remain unstaged.
- [ ] No generated file is staged without its authored source change.
- [ ] No cleanup/reset command was used on unrelated work.
- [ ] The verification command for the current change has passed.
- [ ] The commit message describes only the staged change.

Useful inspection commands:

```bash
git diff --staged --stat
git diff --staged
git status --short
```

## Before Pushing

Confirm the branch and outgoing commits:

```bash
git status --short --branch
git log --oneline origin/main..HEAD
```

If unrelated commits are already ahead of `origin/main`, do not rewrite them.
Push only when those commits are intentionally ready to publish, or ask for a
separate decision.
