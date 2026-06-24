# Best-Practice Improvements Prompt

Use this prompt to ask an agent to review EspControl against the agent-first
engineering practices described in OpenAI's harness engineering article:

https://openai.com/index/harness-engineering/

```text
You are reviewing the EspControl repository:

https://github.com/jtenniswood/espcontrol

First, read OpenAI's "Harness engineering: leveraging Codex in an agent-first
world":

https://openai.com/index/harness-engineering/

Use the article as a lens, not as a checklist to copy blindly. The goal is to
find practical gaps in this repository and recommend targeted improvements that
would make future agent-assisted development safer, clearer, and easier to test.

Repository context:

- EspControl is an ESPHome-based firmware and web configurator project for
  ESP32 touchscreens used with Home Assistant.
- The repo contains firmware components, ESPHome YAML, a browser setup page,
  generated web bundles, public VitePress docs, release workflows, and internal
  developer docs.
- The project already has internal guidance under dev-docs/, repository skills
  under .agents/skills/, generated-file ownership rules, check scripts, GitHub
  Actions, and worktree-based development expectations.
- The user is technical but not a software developer, so recommendations should
  be plain English, practical, and easy to judge.

Inspect the repository before making recommendations. At minimum, review:

- AGENTS.md, if present
- README.md
- dev-docs/README.md
- dev-docs/architecture.md
- dev-docs/source-of-truth.md
- dev-docs/check-matrix.md
- dev-docs/working-tree-rules.md
- dev-docs/failure-cookbook.md
- .agents/skills/
- .github/workflows/
- package.json scripts
- scripts/check_* and scripts/build.py
- devices/manifest.json
- common/config/card_contract.json
- src/webserver/
- components/espcontrol/

Look especially for gaps related to these harness-engineering themes:

1. Repository knowledge as the system of record
   - Is the repo easy for an agent to navigate from a small entry point?
   - Are product rules, architecture rules, source-of-truth rules, and decision
     history discoverable without relying on chat history or human memory?
   - Are docs concise, current, cross-linked, and mechanically checked?

2. Agent legibility
   - Can an agent understand the product, device matrix, generated files,
     firmware/web relationship, and testing expectations from repo-local files?
   - Are important runtime facts encoded in structured files rather than prose
     or duplicated hand-maintained tables?

3. Mechanical guardrails
   - Which architectural or product rules are only documented but not enforced?
   - Where would a custom lint, schema check, smoke test, or generated assertion
     prevent repeated mistakes?
   - Are check failures written clearly enough that an agent can fix the issue
     without guessing?

4. Worktree-local validation
   - Can the repo be built, served, tested, and inspected cleanly from separate
     git worktrees?
   - Are local logs, browser checks, firmware compile checks, and generated
     outputs easy for an agent to run and interpret?
   - What would make hardware-adjacent testing more repeatable without risking
     main?

5. Feedback loops and review
   - Does the PR workflow give agents enough information to respond to CI,
     review comments, release checks, and device testing notes?
   - Are failures routed to the right playbook or troubleshooting page?
   - Could recurring maintenance tasks catch stale docs, duplicate patterns, or
     architectural drift earlier?

6. Entropy control
   - Where are inconsistent patterns spreading?
   - Which files, modules, generated outputs, or docs would benefit from a
     recurring cleanup/check process?
   - What "golden principles" should be encoded into docs or checks so future
     changes stay consistent?

Output format:

Start with a short executive summary in plain English.

Then provide a prioritized list of recommendations. For each recommendation,
include:

- Priority: High, Medium, or Low
- Gap: what is missing or weak today
- Why it matters: the practical risk or friction it creates
- Targeted change: the smallest useful improvement
- Suggested files: likely files or folders to edit
- Verification: how to check the improvement worked
- Effort: Small, Medium, or Large

Keep the recommendations specific to EspControl. Prefer concrete improvements
such as "add a generated docs freshness check for X" or "add a playbook entry
for Y" over vague advice such as "improve documentation."

Call out anything the repo is already doing well so we do not waste time
rebuilding existing strengths.

Do not implement changes in this pass. Produce a review and recommended next
steps only.
```
