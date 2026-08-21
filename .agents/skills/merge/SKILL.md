---
name: merge
description: >-
  Prepare and merge a specific EspControl pull request by addressing remaining
  review feedback, resolving conflicts with main, and merging only when required
  reviews and checks are clear. Use when the user says "/merge" or asks to get a
  pull request ready and merge it.
---

# /merge

## Goal

Take one identified pull request through the repository's final review and
conflict-resolution workflows, then merge it when GitHub reports that it is
ready. Invoking this skill authorizes the merge attempt for that pull request;
it does not authorize bypassing protections, merging unrelated work, closing
issues, or claiming that physical device testing occurred.

## Workflow

### 1. Identify One Pull Request

Use the PR number or URL supplied by the user or the single PR clearly
identified by the current chat. If there is not exactly one PR with confidence,
stop and ask for its number or URL. Do not select one by recency, branch, or the
open PR list.

Record the PR's URL, title, base and head branches, draft state, review decision,
merge state, and checks. Keep the same PR number through every stage below.

### 2. Clear Review Feedback

Read and follow [the `pr-review` skill](../pr-review/SKILL.md) completely for the
identified PR. Address and push all actionable unresolved feedback and resolve
only the threads that were actually addressed.

Do not continue toward merge if actionable feedback remains, a requested change
is intentionally left open, or the review workflow cannot safely complete.
Report what blocks the merge instead.

### 3. Update the Branch and Resolve Conflicts

After review feedback is clear, read and follow [the `resolve-pr-conflicts`
skill](../resolve-pr-conflicts/SKILL.md) completely for the same PR. Bring the
latest base branch into the PR branch and resolve any conflicts, preserving both
the PR's intent and newer base-branch changes. Push the result.

The nested skill normally leaves a PR open; for this `/merge` workflow, continue
to the readiness gate below because the user explicitly requested a merge.

If the source branch cannot be updated, the conflict resolution is uncertain,
or a replacement PR would be required, stop without merging and explain the
next action needed.

### 4. Recheck Merge Readiness

After every review fix, conflict resolution, base-branch update, and other push
is complete, refresh GitHub state and record the current head as the candidate
head SHA. Wait for checks triggered by that exact commit to finish. Before
merging, require all of the following for the candidate head:

- the PR is open and not a draft;
- GitHub reports it mergeable with no conflicts;
- no actionable unresolved review threads or change requests remain;
- all required reviews and required status checks have passed;
- after the last head-changing push, the user has explicitly confirmed they
  tested the candidate head's relevant behavior;
- branch protection does not report another unmet requirement.

Run any focused local check required by the two nested skills. Do not treat a
local build as physical device testing, and do not waive a required check
because a similar local command passed.

User confirmation is mandatory even when automated checks pass or the current
request says to merge. If the user has not confirmed testing, prepare the PR as
far as possible, leave it open, and ask them to test it. Never infer physical
device testing from a build or compile result. Any later head-changing push,
including another review fix or base-branch update, invalidates both the
readiness result and the testing confirmation; repeat this step for the new
candidate head.

If GitHub is still calculating mergeability, refresh briefly. If checks are
pending, wait for them rather than enabling auto-merge. If any gate fails, leave
the PR open and report the exact blocker with links where possible. Never use an
administrator override or otherwise bypass branch protection.

### 5. Merge

Refresh the PR once more and compare its current head SHA with the candidate head
recorded and validated in step 4. If they differ, do not merge; return to step 4
and validate the new head. Merge the identified PR using the repository's
established merge method, preferring squash when no clear convention is
available. Bind the merge to the unchanged candidate head SHA so it fails safely
if another commit arrives. Prefer GitHub connector tools when available and pass
their expected-head equivalent; otherwise use
`gh pr merge <number> --squash --match-head-commit <candidate-head-sha>` (or the
established method's equivalent).

Do not delete the source branch or close linked issues as part of this skill.
After the command returns, refresh the PR and verify that GitHub reports it as
merged. A successful command alone is not sufficient confirmation.

## Final Update

Keep the result concise and include:

- the PR number, title, and URL;
- review feedback addressed or confirmation that none remained;
- conflict/base-branch update performed;
- final checks and review state;
- the verified merge result, or the precise blocker if it stayed open.
