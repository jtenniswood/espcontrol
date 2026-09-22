---
name: user-feedback
description: Draft user-reviewable testing posts for EspControl feature branches, especially manual ESPHome installation instructions and focused hardware feedback steps. Use when the user asks for a feedback request, tester announcement, or review draft for a branch.
---

# User Feedback

Create a complete draft that the user can review before it is posted. Do not publish, comment, or send the draft unless the user separately asks for that action.

## Drafting workflow

1. Identify the feature branch, pull request, affected display, and the exact user-visible behavior being tested. Inspect repository device documentation or configuration when the device path is not explicit.
2. Use the branch ref in all GitHub ESPHome package sources that need to provide the feature. A package URL alone normally resolves the default branch; include `ref: <branch>` when testing an unmerged branch.
3. For EspControl manual ESPHome installs, include the branch component substitution when the nested device package uses `espcontrol_component_ref`, for example:

   ```yaml
   substitutions:
     espcontrol_component_ref: "<branch>"
   ```

4. If the feature changes the web UI, point `web_server.js_url` at the matching branch web asset when practical. Prefer the repository’s established jsDelivr pattern:

   ```yaml
   web_server:
     js_url: https://cdn.jsdelivr.net/gh/jtenniswood/espcontrol@<branch>/docs/public/webserver/embedded/www.js?device=<device-slug>&v=<test-label>
   ```

5. Include a complete, copyable ESPHome YAML example. Use the device’s documented `packages.yaml` path and matching device slug; do not invent paths. Include Wi-Fi placeholders using the repository’s normal `!secret` convention unless the user requests inline credentials.
6. Give testers a short numbered checklist tied to the feature’s acceptance criteria. For climate controls, cover capability visibility, the old control’s behavior, the new control’s behavior, independent backend attributes/services, labels, and unsupported-entity behavior when relevant.
7. Ask testers to report the display model, ESPHome version, entity or device details, available modes, observed behavior, and any errors. Keep the language friendly and concrete.

## Output boundaries

- Return the full draft in Markdown, normally with a title, setup/configuration section, test checklist, and reporting request.
- Do not include a command-line `esphome run ...` section when the user asks for an ESPHome Device Builder/manual-install post without command-line instructions.
- Preserve the user’s requested wording and omissions. Do not add unrelated release notes, implementation details, or claims that a test passed.
- Clearly distinguish branch-testing instructions from production/release installation instructions.
