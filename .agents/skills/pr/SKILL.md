---
name: pr
description: >-
  Review a new or open pull request for this repository with an adversarial
  security and quality lens. Use when the user says "/pr", "/pr-review",
  "check the new PR", "is this PR safe to merge", "look for malicious code",
  "review outside contribution", "should I merge this PR", or asks for
  reasons not to merge a pull request.
---

# /pr or /pr-review

## Overview

Assess a pull request before merge. Focus on bugs, suspicious behavior,
malicious code, supply-chain risk, broken project conventions, missing tests,
and any reason the repo owner should wait before merging.

Default to review only. Do not merge, push fixes, approve the PR, or close
linked issues unless the user explicitly asks. The repo owner wants to test
changes before issues are closed.

## Workflow

### 1. Identify the PR

Use the PR number or URL if the user provides one. Otherwise inspect open PRs
and review the newest one, clearly saying that assumption.

Prefer GitHub connector tools when available. Otherwise use `gh`:

```bash
gh pr list --state open --limit 10 \
  --json number,title,author,createdAt,updatedAt,isCrossRepository,headRefName,baseRefName,url
gh pr view <number> \
  --json number,title,author,body,baseRefName,headRefName,isCrossRepository,files,commits,mergeStateStatus,statusCheckRollup,reviews,url
gh pr diff <number> --name-only
gh pr diff <number> --patch
```

Record the PR title, author, URL, base branch, source branch, whether it comes
from a fork, changed files, diff size, existing CI state, and whether it touches
release, firmware, workflow, dependency, generated, or web delivery paths.

### 2. Protect Local Work

Check the working tree before checkout or tests:

```bash
git status --short --branch
```

If there are local changes, do not overwrite or revert them. Review with
`gh pr diff` when possible. Ask before checking out a PR if local changes might
be affected.

### 3. Inspect for Malicious or Suspicious Changes

Treat outside contributions and dependency/workflow changes as higher risk
until they are understood. Look especially for:

- New network calls, telemetry, tracking, external downloads, curl/wget usage,
  remote scripts, or branch-based dependencies
- GitHub Actions permission expansion, secret exposure, `pull_request_target`,
  unpinned third-party actions, or changed release credentials
- Dependency changes, lockfile churn, install scripts, typosquatting, package
  manager config changes, or vendored/minified code
- Obfuscated code, encoded payloads, unexplained binaries, generated files that
  do not match their source, or large unrelated rewrites
- Changes to OTA behavior, firmware manifests, update URLs, release assets,
  GitHub Pages output, or Home Assistant service calls
- Unsafe parsing, string escaping issues, unchecked indexes, buffer sizing,
  memory lifetime, LVGL object lifetime, or shell command construction
- Backward-incompatible config changes, silent defaults, or user-visible
  behavior changes with no docs or migration note

Do not accuse the contributor of intent unless there is direct evidence. Say
"suspicious", "high risk", or "could allow" when intent is unknown.

### 4. Review Correctness and Fit

Read the PR description, changed files, and surrounding code. Classify the
change so checks match the affected area:

- **Docs:** accuracy, broken links, outdated examples, docs build
- **Web UI:** escaping, state handling, generated output, layout, config save
  and load behavior
- **Firmware YAML:** ESPHome schema, substitutions, includes, device coverage,
  generated slot wiring
- **C++ component:** bounds checks, ownership, LVGL lifetime, service calls,
  compile risk
- **Icons/fonts/timezones:** generated asset consistency and validation scripts
- **CI/release/dependencies:** permissions, secrets, action pinning, lockfile
  integrity, release behavior

Prioritize merge-blocking defects and practical risk over style comments.

### 5. Run Focused Checks

Run checks relevant to the changed files. Explain skipped checks plainly.

Common checks:

```bash
npm run check:config
python3 scripts/build.py --check
python3 scripts/generate_device_slots.py --check
python3 scripts/check_icon_groups.py
python3 scripts/check_timezones.py
npm run docs:build
npm audit --json
```

Use the `compile` skill when firmware, common YAML, device YAML, or C++ changes
could affect firmware builds and the user wants merge-level confidence.

### 6. Decide Whether to Merge

Choose exactly one recommendation:

- **Merge:** No meaningful findings, relevant checks pass, and residual risk is
  low.
- **Merge after small fixes:** The PR is basically sound but needs minor,
  specific changes first.
- **Request changes:** There are bugs, stale generated files, missing tests, or
  documentation gaps that should be fixed before merge.
- **Hold:** There is suspicious behavior, dependency/release/workflow risk, or
  a maintainer decision needed before the PR is safe to merge.

## Output Format

Lead with findings when there are bugs or risks. Keep the explanation
approachable for a technical non-developer.

```text
Recommendation: <merge / merge after small fixes / request changes / hold>

What changed:
- <plain-English summary>

Findings:
- <severity, file/line, issue, impact, suggested fix>

Security and trust notes:
- <malicious-code, supply-chain, workflow, secret, release, or external-call risk>

Checks run:
- <command>: <pass/fail/skipped and why>

Questions or decisions:
- <only if needed>
```

If there are no findings, say that clearly and name any checks not run or
residual risk that still remains.
