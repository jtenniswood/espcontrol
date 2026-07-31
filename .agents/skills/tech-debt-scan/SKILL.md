---
name: tech-debt-scan
description: Research-only technical debt scan for this EspControl repository. Use when the user asks for a tech debt scan, cleanup backlog, maintainability review, messy or rushed implementation review, duplicated logic audit, fragile assumption scan, dead code review, architecture cleanup plan, or asks to identify code that works now but will be painful to maintain later.
---

# Tech Debt Scan

Perform a research-only review of the repository and turn findings into a
practical cleanup backlog. Do not implement fixes during the scan unless the
user separately selects a backlog item and asks for it to be fixed.

The user is technical but not development-oriented. Explain findings in plain
language, with enough file detail for a future implementation task.

## Guardrails

- Treat the first pass as research only.
- Do not change product code, firmware, docs, generated files, or tests while
  scanning.
- The only allowed file change during the scan is creating or updating the
  cleanup backlog file.
- If the user asks to fix an item later, use the repository branching and PR
  workflow from `AGENTS.md`.
- Do not close issues or merge PRs as part of this skill.

## Backlog File

Create or update:

```text
dev-docs/tech-debt-cleanup-backlog.md
```

Use this file as the durable task list. Keep it concise enough to revisit after
each cleanup task. If the file already exists, update it instead of replacing it
blindly: preserve unresolved useful items, remove or mark completed items only
when there is direct evidence, and add new findings with dates or context where
helpful.

## Scan Workflow

1. Inspect repository state.
   - Check the branch and local changes.
   - Do not revert or overwrite user work.
   - Note whether the scan was run from `main`, a feature branch, or a PR
     branch.

2. Map the project structure.
   - Identify the main firmware, device, web, docs, scripts, and generated
     output areas.
   - Read representative files before drawing conclusions.
   - Prefer targeted searches with `rg` and focused file reads.

3. Look for these debt patterns:
   - Rushed or messy implementation.
   - Duplicated logic.
   - Inconsistent naming or folder structure.
   - Fragile assumptions, hidden coupling, or order-dependent behavior.
   - Missing error handling or weak failure messages.
   - Security, validation, or unsafe input handling issues.
   - Dead code, unused files, stale generated output, or obsolete scripts.
   - Architecture that is becoming confusing.
   - Code that works now but will be painful to maintain later.

4. Validate each meaningful finding.
   - Name the files or folders involved.
   - Explain the observed pattern, not just the conclusion.
   - Check whether there is an existing helper, component, script, or pattern
     that the code should probably use instead.
   - Separate confirmed problems from areas that only need deeper inspection.

5. Prioritize by practical impact.
   - Critical: likely to cause incorrect builds, broken firmware, unsafe input,
     release risk, upgrade risk, data loss, or confusing architecture around a
     core workflow.
   - Medium: clear maintainability cost, duplication, inconsistency, fragile
     assumptions, or missing validation that is not immediately dangerous.
   - Nice-to-have: polish, readability, organization, small consistency issues,
     or optional follow-up checks.

## Backlog Format

Write the backlog file with this structure:

```markdown
# Tech Debt Cleanup Backlog

Last reviewed: YYYY-MM-DD
Review scope: <branches, folders, or areas inspected>

## Critical Issues

### 1. <short action-oriented title>
- Where: `<file-or-folder>` and any related areas
- Why it matters: <plain-English risk>
- Recommended change: <practical fix direction>
- Safe to fix now?: Yes / Wait, with reason
- Suggested first step: <smallest useful next action>

## Medium Cleanup Items

### 1. <short action-oriented title>
- Where: `<file-or-folder>` and any related areas
- Why it matters: <plain-English risk>
- Recommended change: <practical fix direction>
- Safe to fix now?: Yes / Wait, with reason
- Suggested first step: <smallest useful next action>

## Nice-to-Have Polish

### 1. <short action-oriented title>
- Where: `<file-or-folder>` and any related areas
- Why it matters: <plain-English risk>
- Recommended change: <practical fix direction>
- Safe to fix now?: Yes / Wait, with reason
- Suggested first step: <smallest useful next action>

## Completed Items

- YYYY-MM-DD: <item completed and PR/branch if known>

## Scan Notes

- Checks run: <commands or review activities>
- Areas not deeply reviewed: <known gaps>
```

Use numbering inside each priority section so the user can choose a cleanup task
by section and number, such as "Medium item 2".

## Response Workflow

After creating or updating the backlog file:

1. Show the prioritized cleanup backlog in the chat.
2. Keep the wording approachable and focused on practical next steps.
3. Ask the user to select the next task to work on.

After completing a selected cleanup task in a later turn:

1. Update the backlog file to move or mark the completed item.
2. Re-display the remaining cleanup backlog.
3. Ask the user to select the next task.

## Output Style

Use this chat format after a scan:

```text
Created/updated: dev-docs/tech-debt-cleanup-backlog.md

Critical issues:
1. <title> - <one-line practical impact>

Medium cleanup items:
1. <title> - <one-line practical impact>

Nice-to-have polish:
1. <title> - <one-line practical impact>

Pick the next item by saying something like "Medium item 1".
```

If no critical issues are found, say that clearly and still list medium and
nice-to-have items.
