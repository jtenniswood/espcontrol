# DESLOPPIFY.md

This scan focuses on maintainability risks in the hand-written parts of the project. Generated bundles, vendored libraries, and compatibility fixtures were treated as background unless they exposed a practical risk.

## 1. Critical issues

No critical issues were found in this pass. I did not find an obvious data-loss path, leaked secret, unauthenticated remote control path, or broken build configuration that should block normal work immediately.

## 2. Medium cleanup items

### 2.1 Dynamic HTML fragments in the web UI

- Where: `src/webserver/application/button_settings.ts`, `src/webserver/application/preview_interactions.ts`, `src/webserver/application/app_status_preview.ts`, and related helpers in `src/webserver/application/state.ts`.
- Why it matters: Several UI controls were assembled with `innerHTML`. Most user-facing text was escaped, but this pattern is easy to get wrong when new fields are added. It also makes future edits harder because markup, data, and escaping rules are mixed together.
- Recommendation: Prefer small DOM helper functions (`textContent`, `classList`, explicit child elements) for interactive controls and dynamic labels.
- Safe to fix now: Yes. This is low-risk because it preserves the same visible elements while reducing accidental HTML injection risk.
- Status: Fixed in this pass for the context menu, icon picker, card type picker, and sun information preview.

### 2.2 Duplicated POST retry handling

- Where: `src/webserver/application/api.ts`.
- Why it matters: `post()` and `postOptional()` had almost identical retry loops. Any future change to reconnect behavior, fallback URL ordering, or failure messaging would have to be made in two places.
- Recommendation: Move the shared "try each POST URL until one succeeds or the list is exhausted" logic into one helper.
- Safe to fix now: Yes. The behavior is small, local, and covered by existing web smoke checks.
- Status: Fixed in this pass with `postFirstAvailable()`.

### 2.3 Oversized browser smoke test script

- Where: `scripts/check_web_browser_smoke.js`.
- Why it matters: The script mixes route setup, fake device events, layout measurement, settings checks, backup import checks, and interaction tests in one 1,300+ line file. It works, but it is becoming difficult to extend without accidental coupling between tests.
- Recommendation: Split the file into focused helpers, for example browser harness, fake ESPHome state, layout assertions, settings assertions, and interaction scenarios.
- Safe to fix now: Wait. This is worthwhile, but it should be a dedicated test-maintenance PR so failures are easy to review.
- Status: Documented for a follow-up.

### 2.4 Dev tooling dependency audit findings

- Where: `package-lock.json` dependency tree, reported by `npm audit --omit=optional --json`.
- Why it matters: The audit reports one high and two moderate advisories through Vite/VitePress dev tooling. These affect local docs/dev-server tooling rather than the firmware runtime, but they are still worth tracking because docs tooling is run by developers and CI.
- Recommendation: Review VitePress/Vite upgrade options in a dedicated dependency PR, because npm reports no automatic fix for the current dependency tree.
- Safe to fix now: Wait. Dependency upgrades can affect docs generation and should be isolated with `npm run docs:build` and CI checks.
- Status: Documented for a follow-up.

### 2.5 Web card normalization remains concentrated in one large file

- Where: `src/webserver/application/config_codec.ts`, especially `normalizeButtonConfig()` and `buttonConfigFields()`.
- Why it matters: Card rules, migration rules, serialization details, and some UI-specific logic live together. The TypeScript model already owns several lower-level parsing helpers, so future card changes have to be checked across multiple conceptual layers.
- Recommendation: Gradually move pure card normalization rules into the typed model layer, then let the UI module call those helpers.
- Safe to fix now: Wait. This touches saved configuration compatibility and should be sliced by card type with focused contract tests.
- Status: Documented for a follow-up.

### 2.6 Todo card full/lite implementations share one large firmware header

- Where: `components/espcontrol/button_grid_todo.h`.
- Why it matters: Full, lite, and disabled todo behavior live in one long header with parallel constants, request handling, response parsing, and modal rendering. The preprocessor branches are intentional for memory control, but they make regressions harder to spot.
- Recommendation: Extract shared parsing/request helpers first, then consider separate full/lite implementation headers behind the same public entry points.
- Safe to fix now: Wait. This needs firmware compile checks across all supported devices.
- Status: Documented for a follow-up.

## 3. Nice-to-have polish

### 3.1 Repeated status badge markup

- Where: `src/webserver/application/settings_page.ts`.
- Why it matters: Several settings cards build the same "ON" badge markup by hand. It is not broken, but it adds noise and makes small UI changes repetitive.
- Recommendation: Add a tiny `statusBadge(label)` helper and use it for schedule, clock bar, screensaver, idle, and cover art badges.
- Safe to fix now: Yes, but lower value than the safety/duplication fixes above.
- Status: Fixed in this pass with `statusBadge(label)`.

### 3.2 Generated and vendored files dominate simple scans

- Where: `docs/public/webserver/**`, `components/libjpeg-turbo-esp32/**`, and generated files such as `src/webserver/generated/card_contract.ts`.
- Why it matters: Basic repository scans produce noisy results unless generated and vendor paths are excluded. That makes cleanup work look worse than it is.
- Recommendation: Add a short note or helper command for maintainability scans that excludes generated bundles and vendored libraries.
- Safe to fix now: Yes, but documentation-only and not required for this pass.
- Status: Fixed in this pass by extending the broad review scan note in `dev-docs/README.md`.
