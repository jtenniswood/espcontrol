---
name: review-all
description: Review every open pull request authored by the user in this repository and address actionable unresolved review feedback. Use when the user says "/review-all", "review all", "review all my PRs", asks to iterate on all of their PR review comments, or wants all open PR feedback inspected, fixed, tested, committed, pushed, and resolved.
---

# Review All

## Goal

Run the `/pr-review` workflow across all open pull requests authored by the user, not just one PR. Find unresolved review threads, make focused fixes on the matching PR branches, push each update, and resolve only the threads that were truly addressed.

This skill is for feedback iteration. It is not for reviewing outside contributions for merge safety; use `/pr` for that.

## Rules

- Protect `main`. Do not edit the main worktree for PR fixes.
- Work in each PR branch's existing worktree when available; otherwise create a separate worktree.
- Do not overwrite unrelated local changes. If a PR worktree has unrelated changes, inspect them and work around them.
- Keep each PR fix scoped to its own branch.
- Commit and push every branch where changes are made.
- Resolve only the review thread IDs that the pushed changes actually address.
- Leave unclear, non-actionable, or intentionally rejected feedback unresolved and explain why.
- Do not include "Codex" in branch names, commit subjects, or PR titles.

## Workflow

### 1. List the user's open PRs

Use `gh` from the repository root:

```bash
gh pr list --state open --author @me --limit 100 \
  --json number,title,author,headRefName,baseRefName,reviewDecision,updatedAt,url
```

If the user supplied a different owner or author, use that explicit value. Otherwise assume `@me`.

### 2. Collect unresolved review feedback

For each open PR, use thread-level GraphQL data so resolved feedback is not repeated. Review threads must be paginated before filtering for unresolved items; do not assume `first:100` is the complete result set.

```bash
gh api graphql -f owner='{owner}' -f name='{repo}' -F number=<pr-number> -f cursor=null -f query='
query($owner:String!, $name:String!, $number:Int!, $cursor:String) {
  repository(owner:$owner, name:$name) {
    pullRequest(number:$number) {
      reviewDecision
      reviewThreads(first:100, after:$cursor) {
        pageInfo {
          hasNextPage
          endCursor
        }
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

If `reviewThreads.pageInfo.hasNextPage` is true, repeat the query with `cursor` set to `endCursor` until every page has been collected, then filter for unresolved threads. Summarize the actionable unresolved threads before editing. Ignore already-resolved threads. Treat ordinary PR comments as context unless they clearly describe a current failing check or requested change.

### 3. Prepare each PR branch

Check current worktrees:

```bash
git worktree list --porcelain
```

Use an existing worktree for the PR branch if one exists. Otherwise fetch and add one:

```bash
git fetch origin <head-branch>
git worktree add ../espcontrol-<short-topic> <head-branch>
```

If the local branch does not exist, create it tracking origin:

```bash
git worktree add ../espcontrol-<short-topic> -b <head-branch> origin/<head-branch>
```

Before edits in every PR worktree:

```bash
git status --short --branch
```

### 4. Fix feedback branch by branch

For each actionable thread:

- Read the commented file and surrounding code before editing.
- Prefer the smallest change that fully preserves intended behavior.
- Follow existing project patterns and helper APIs.
- If generated web assets become stale, regenerate them with `python3 scripts/build.py www`.
- Do not make unrelated cleanup changes.

When multiple threads are on the same PR, address them together if the fixes are related, then commit once.

### 5. Run focused checks

Choose checks based on touched files:

```bash
npm run check:config
python3 scripts/build.py --check
npm run check:web-smoke
npm run check:firmware-parser
npm run check:firmware-card-runtime
npm run check:firmware-modals
git diff --check
```

Use the `compile` skill only when the requested confidence level or file changes require full firmware build validation.

### 6. Commit and push each fixed branch

```bash
git status --short
git add <changed-files>
git commit -m "<short human summary>"
git push
```

If a branch has no actionable changes, do not create an empty commit.

### 7. Resolve addressed review threads

After the commit has pushed, resolve only the thread IDs fixed by that push:

```bash
gh api graphql -f threadId='<review-thread-id>' -f query='
mutation($threadId:ID!) {
  resolveReviewThread(input:{threadId:$threadId}) {
    thread { id isResolved }
  }
}'
```

If resolving fails, keep going and report the thread URL so the user can resolve it manually.

### 8. Verify completion

Run one final paginated unresolved-thread pass across the user's open PRs. The goal is a concrete result such as:

```text
No unresolved review threads found across <n> open PRs authored by @me.
```

If unresolved threads remain, list the PR number, thread URL, and reason.

## Final Update

Keep the final message concise and approachable:

```text
Reviewed <n> open PRs authored by you.

Changed and pushed:
- #<number>: <plain-English fix>

Resolved feedback:
- <count> review thread(s)

Left open:
- <only if any, with reason>

Checks:
- <commands run and pass/fail/skipped>
```
