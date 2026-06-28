---
name: Expert Review
description: >-
  Provide an expert codebase review for this repository. Use when the user says
  "Expert Review" or "/expert-review", asks for an expert review, wants
  refactoring or codebase improvement recommendations, wants consistency and
  quality assessed, or asks how to reduce duplication, improve component use,
  preserve the end-user experience, and keep upgrades cleanly migratable.
---

# Expert Review

## Overview

Review the codebase as a senior product-minded engineer. Produce a ranked list
of recommendations to improve maintainability, consistency, component reuse,
quality, and upgrade safety without changing the current end-user experience.
Recommendations should be direct and appropriately bold: if the structure of
the project needs a large change to reduce long-term risk, say so clearly
instead of only suggesting small local fixes.

Default to review only. Do not implement code changes, alter behavior, close
issues, or start a large refactor unless the user explicitly asks.

The audience is technical but not development-oriented, so explain findings in
plain language while still being precise about risk, impact, and effort.

## Review Principles

- Preserve the current user experience unless recommending an explicitly
  optional consistency improvement.
- Favor clean upgrade paths: existing users should keep working after upgrading,
  with defaults, migrations, compatibility layers, or documented transition
  steps where needed.
- Prefer reusable components, shared helpers, clear boundaries, and established
  project patterns over one-off implementations.
- Reduce duplication when it lowers future maintenance risk. Do not recommend
  abstraction for its own sake.
- Be willing to recommend large structural changes when the evidence shows that
  incremental fixes would leave the same maintenance or upgrade risk in place.
- Do not soften important recommendations just because they are disruptive.
  Label them clearly, explain why they are worth doing, and separate the
  recommendation from the rollout plan.
- Always pair large or high-risk recommendations with a practical mitigation
  plan so the user can see how the work can be controlled.
- Separate user-facing polish suggestions from internal quality improvements.
- Rank work by practical importance, not by what is easiest to spot.

## Workflow

### 1. Establish Project State

Inspect the current branch, local changes, and recent structure.

```bash
git status --short --branch
find . -maxdepth 3 -type f | sort
```

If there are local changes, work with them. Do not revert or overwrite user
work.

### 2. Map the Product and Architecture

Identify the main user experiences and the code paths that support them. Read
enough of the repository to understand:

- Web or configuration UI surfaces
- Firmware, device, or generated configuration paths
- Shared scripts, validators, and build or release workflows
- Repeated patterns, duplicated logic, or parallel implementations
- Data formats and compatibility boundaries users depend on

Prefer targeted searches with `rg` and file reads over broad assumptions.

### 3. Evaluate Consistency and Quality

Look for improvement opportunities in these areas:

- Component reuse: repeated UI structures, controls, layout patterns, scripts,
  configuration fragments, validation logic, or firmware/device definitions
- Interaction consistency: similar actions behaving differently, inconsistent
  labels, validation feedback, defaults, save flows, navigation, or presentation
- Data and migration safety: changed schemas, generated files, saved settings,
  firmware configuration, API contracts, version handling, and upgrade fallback
  behavior
- Code organization: unclear ownership, mixed responsibilities, duplicated
  business rules, fragile coupling, and scattered constants
- Test and validation coverage: missing checks for shared behavior, generated
  output, compatibility, or high-risk workflows
- Developer workflow: unreliable scripts, hard-to-repeat checks, inconsistent
  formatting, stale generated assets, and unclear release confidence

### 4. Validate Before Recommending

For each significant recommendation, gather concrete evidence:

- Name the affected files or areas.
- Explain what pattern or risk was observed.
- Confirm whether the recommendation preserves the current user experience.
- Describe how existing users would migrate cleanly.
- Identify the main risks of making the change and how to reduce them.
- Estimate the work in small, medium, or large terms.

Run lightweight checks only when they help validate the review. Avoid expensive
or invasive checks unless the user asks for deeper confidence.

### 5. Add Risk Controls for Large Changes

For every medium, large, or structurally significant recommendation, include
steps to mitigate and manage the risk of doing the work. The risk controls
should be practical and staged, such as:

- Split the work into reviewable phases with a clear stopping point after each
  phase.
- Preserve old behavior behind compatibility layers, defaults, feature flags, or
  adapter functions while the new structure is introduced.
- Add characterization tests or generated-output comparisons before moving code,
  so current behavior is captured before refactoring starts.
- Migrate one representative path first, verify it, then repeat the pattern for
  the remaining paths.
- Keep user-facing output, saved settings, generated firmware/configuration, and
  public interfaces stable until a deliberate migration step is ready.
- Define rollback points, manual test steps, and release checks before changing
  high-impact areas.
- Document any user-visible migration path in plain language.

Do not use risk as a reason to avoid recommending necessary structural work.
Instead, explain how to make the work safer.

### 6. Prioritize

Rank recommendations using this order:

1. Upgrade safety, data compatibility, or release risk
2. User-facing consistency problems that can confuse users
3. Shared duplication that increases bug risk across multiple experiences
4. Missing validation or test coverage for important workflows
5. Code organization improvements that unlock future work
6. Nice-to-have polish or documentation improvements

When two items are close, put the one with broader user or maintenance impact
first.

## Output Format

Start with a short plain-English summary of the overall health and the biggest
theme.

Then provide ranked recommendations:

```text
1. <Recommendation title>
   Importance: Critical / High / Medium / Low
   Recommendation strength: Strong / Moderate / Optional
   Why it matters: <plain-English reason>
   Evidence: <files, patterns, or examples reviewed>
   User experience impact: <no change / optional consistency improvement / needs careful handling>
   Migration approach: <how existing users upgrade cleanly>
   Risk management: <specific steps to reduce rollout, regression, and migration risk>
   Work required: Small / Medium / Large, with the main tasks
   Suggested first step: <practical next action>
```

Finish with:

```text
Quick wins:
- <small, low-risk improvements>

Not recommended right now:
- <tempting refactors or behavior changes that should wait, with why>

Checks run:
- <command or review activity>: <result, or skipped with reason>
```

If the review finds no meaningful issues, say so clearly and list any remaining
areas that were not inspected deeply.
