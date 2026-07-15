---
name: pr-review
description: >-
  Address GitHub pull request feedback for this repository. Use when the user
  says "/pr-review" and wants review comments, requested changes, or unresolved
  feedback on an open PR inspected, incorporated into the branch, pushed, and
  marked resolved after the fix is made.
---

# /pr-review

## Goal

Turn actionable PR feedback into a tested branch update. Find unresolved review
comments, understand what they are asking for, make the matching code or docs
changes, push the branch, and resolve only the feedback threads that were truly
addressed.

This is different from `/pr`, which reviews a PR and recommends whether to
merge. Use `/pr-review` when feedback already exists and needs action.

## Workflow

### 1. Identify the PR

Use the PR number or URL if the user provides one. Otherwise use the single PR
that the current chat is already active in, based on the existing chat context
or linked PR context. If the chat does not identify exactly one PR with
confidence, stop and ask the user for the PR number or URL before continuing.

Never choose a PR from the open PR list, by recency, or from an unrelated
checked-out branch. Do not guess or silently substitute another PR.

Prefer GitHub connector tools when available. Otherwise use `gh`:

```bash
gh pr view <number> \
  --json number,title,body,headRefName,baseRefName,reviewDecision,statusCheckRollup,url
```

### 2. Work in the PR Branch

Protect `main`. Use the PR branch in a separate worktree when possible. If the
branch is not local yet:

```bash
git fetch origin <head-branch>
git worktree add ../espcontrol-<short-topic> -b <head-branch> origin/<head-branch>
```

Before editing, check for local changes:

```bash
git status --short --branch
```

Do not overwrite or revert unrelated user changes.

### 3. Collect Unresolved Feedback

Collect unresolved review threads, requested changes, and ordinary PR comments.
Use thread-level data whenever possible so resolved feedback is not repeated.

With `gh`, use GraphQL for review threads:

```bash
gh api graphql -f owner='{owner}' -f name='{repo}' -F number=<pr-number> -f query='
query($owner:String!, $name:String!, $number:Int!) {
  repository(owner:$owner, name:$name) {
    pullRequest(number:$number) {
      reviewDecision
      reviewThreads(first:100) {
        nodes {
          id
          isResolved
          path
          line
          startLine
          comments(first:20) {
            nodes {
              author { login }
              body
              createdAt
              url
            }
          }
        }
      }
      comments(first:50) {
        nodes {
          author { login }
          body
          createdAt
          url
        }
      }
    }
  }
}'
```

Summarize each actionable item in plain language before editing. Ignore already
resolved threads unless later comments clearly reopen the same issue.

### 4. Make the Requested Changes

For each actionable item:

- Read the relevant file and surrounding code before editing.
- Make the smallest change that fully addresses the feedback.
- Preserve existing project patterns and user-facing behavior unless the
  feedback explicitly asks for a behavior change.
- If feedback is unclear, infer cautiously from the code and comment context.
  Ask the user only when the requested change could go in materially different
  directions.
- Do not make unrelated cleanups while addressing review feedback.

If a comment is not actionable or should not be implemented, leave it
unresolved and explain why in the final update.

### 5. Check the Change

Run focused checks based on touched files. Common checks:

```bash
npm run check:config
python3 scripts/build.py --check
python3 scripts/generate_device_slots.py --check
python3 scripts/check_icon_groups.py
python3 scripts/check_timezones.py
npm run docs:build
```

Use the `compile` skill when firmware, common YAML, device YAML, or C++ changes
could affect firmware builds and the user wants build-level confidence.

### 6. Commit and Push

This repo expects branch changes to be committed and pushed.

```bash
git status --short
git add <changed-files>
git commit -m "<short, human branch update summary>"
git push
```

Do not include "Codex" in branch names, commit subjects, or PR titles.

### 7. Resolve Addressed Feedback

After the fix is committed and pushed, resolve only the review thread IDs that
were actually addressed. Do not resolve broad comments, unresolved decisions,
or feedback you intentionally did not implement.

With `gh` GraphQL:

```bash
gh api graphql -f threadId='<review-thread-id>' -f query='
mutation($threadId:ID!) {
  resolveReviewThread(input:{threadId:$threadId}) {
    thread { id isResolved }
  }
}'
```

If a thread cannot be resolved because of permissions or API limits, say so and
include the comment URL so the user can resolve it manually.

### 8. Final Update

Keep the final message approachable and concise:

```text
Addressed PR #<number>: <title>

Changed:
- <plain-English summary>

Resolved feedback:
- <comment/topic>

Left open:
- <only if any, with reason>

Checks:
- <command>: <pass/fail/skipped and why>

Pushed branch: <branch>
PR: <url>
```
