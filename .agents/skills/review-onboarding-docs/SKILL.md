---
name: review-onboarding-docs
description: Research-only audit of product and device documentation for first-time users. Use when explicitly asked to review, critique, assess, or audit getting-started, installation, setup, configuration, or support docs; or when a bold, high-level documentation strategy with five recommendations is needed. Do not use to edit documentation.
---

# Review Onboarding Docs

Audit documentation from the perspective of a capable but non-developer first-time device owner. The goal is a calm, obvious path from choosing the product through installation, setup, configuration, and successful everyday use. Advanced material must remain available without interrupting that path.

This is research only. Inspect, analyse, and recommend; do not edit files, create issues, commit, or open a pull request.

## Establish the user journey

1. Identify the intended new-user outcomes: understand what is needed, install the device safely, connect it, configure it, confirm it works, and know where to get help.
2. Find the current entry points, navigation, key guides, prerequisites, installation instructions, configuration references, troubleshooting, and advanced/developer content. Follow links as a new user would.
3. Identify the audience assumed by each page. Treat unexplained technical vocabulary, hidden prerequisites, and choices without decision guidance as friction.
4. If external research would materially improve the assessment, browse current, authoritative examples of onboarding or technical-documentation practice. Use it to inform principles, not to copy a competitor's structure. Cite sources in the report.

## Evaluate the experience

Assess the documentation as a journey, not as isolated pages. Look for:

- A clear starting point, purpose, expectations, supported hardware, and prerequisites.
- A linear happy path with visible progress, decision points, confirmation checks, and recovery when something fails.
- Plain language, short purposeful steps, and images/diagrams only where they reduce ambiguity.
- Separation of task guides (what a new owner must do) from reference material (every option and detail).
- Progressive disclosure: newcomers encounter only the minimum needed next; advanced configuration, integrations, variants, migration notes, and developer material are reachable but do not dominate.
- Information architecture, navigation labels, duplication, contradictory instructions, stale material, and dead-end links.
- Safety-critical steps, irreversible actions, privacy/network considerations, and realistic troubleshooting.

Do not preserve a weak structure merely because it already exists. Consider bold alternatives such as a new onboarding hub, task-oriented navigation, consolidating duplicate material, moving detailed configuration into a reference area, or retiring/moving pages that obscure the primary path.

## Form recommendations

Prioritise changes by their likely improvement to successful first-time setup, not by how easy they are to make. Use direct evidence from the documentation. Clearly separate observed facts from inferences.

Return a concise report with:

1. **Journey verdict** — one paragraph: who the current docs serve well, where a newcomer is likely to stall, and the most important structural problem.
2. **Five high-impact suggestions** — exactly five, ordered by impact. For each include:
   - a decisive title and the problem it solves;
   - evidence, naming the relevant pages/areas;
   - the recommended end state, including substantial reorganisation when warranted;
   - how advanced content becomes progressively discoverable;
   - the user benefit and a practical success signal.
3. **Suggested future structure** — a compact proposed top-level navigation or learning path, when reorganisation is one of the recommendations.
4. **Scope boundary** — list material that should stay advanced/reference-only rather than be placed in the main onboarding flow.

Avoid a long issue inventory, minor wording edits, or generic advice. Be direct about material that should move, merge, split, or be removed from the newcomer path. Do not make documentation changes in this run.
