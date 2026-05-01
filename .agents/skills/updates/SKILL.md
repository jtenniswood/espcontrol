---
name: updates
description: >-
  Summarize user-facing project changes from the last 7 days. Use when the user
  says "/updates", asks for recent updates, wants a changelog-style summary, or
  wants the most important feature changes from the past week.
---

# /updates

Summarize what changed in this project over the last 7 days, focusing on
user-facing features and behavior rather than implementation details.

## Workflow

### 1. Collect Recent Changes

Use the current date as the endpoint and inspect commits from the last 7 days.

```bash
git status --short --branch
git fetch origin --prune
git log --since="7 days ago" --date=short --pretty=format:'%h %ad %s' --decorate
git log --since="7 days ago" --name-only --pretty=format:'commit %h %ad %s' --date=short
```

If the local branch is behind or stale, note that the summary is based on the
available local history unless a fetch succeeded.

### 2. Understand User Impact

For each meaningful change, inspect enough context to explain what a user can
now do, see, configure, or rely on.

Useful commands:

```bash
git show --stat <commit>
git show --name-only <commit>
git show -- <path>
```

Prioritize these as user-facing:

- New visible UI behavior, controls, screens, or settings
- Firmware capabilities, device support, setup, update, or reliability changes
- Documentation that helps users operate, configure, or troubleshoot the project
- Release, install, or build changes that affect how users receive the project

Usually skip these unless they clearly affect users:

- Internal refactors
- Formatting-only changes
- Test-only changes
- CI-only maintenance
- Dependency bumps without a visible change

### 3. Order by Importance

Sort the summary by practical user impact:

1. New or expanded capabilities
2. Fixes for broken or confusing user workflows
3. Reliability, compatibility, or update improvements
4. Documentation or setup improvements
5. Minor polish

When importance is close, put broader changes before narrow device- or
page-specific changes.

### 4. Write the Update

Use one sentence per item in this format:

```text
Feature: description of the user-facing change.
```

Rules:

- Keep each item to one sentence.
- Use approachable, non-developer language.
- Mention the feature or area first, then the benefit.
- Avoid commit hashes, file names, and developer terms unless the user asks.
- Combine related commits into one item when they deliver the same user-facing
  outcome.
- If there were no user-facing changes, say that clearly and mention that recent
  work appears to be internal maintenance.

## Output Format

```text
Updates from <start date> to <end date>:

- Feature: <one-sentence description>
- Feature: <one-sentence description>
```

If the 7-day window is ambiguous, use exact dates in the heading.
