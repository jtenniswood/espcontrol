# Capture the actual EspControl preview

Use the active repository's installed development dependencies. The capture helper builds a fresh test bundle into a temporary directory and uses the existing browser smoke harness to simulate a panel. It does not connect to a real device or Home Assistant.

```sh
node <skill-dir>/scripts/capture-preview.cjs \
  --repo <active-worktree> \
  --config <fixture.json> \
  --out <screen-preview.png>
```

For a configuration-dialog screenshot, add `--admin-slot <slot>` and `--admin-out <settings.png>`. Both options are required together. This captures the configured card's real editor without changing its settings.

If the host shell lacks Node or packages, use `load_workspace_dependencies`, put the returned Node binary directory on PATH for the build subprocess, and make `sharp` available through NODE_PATH for compositing. Prefer the repository's existing dependency install for the web build and Playwright. Browser launch may require the host's normal execution approval.

## Fixture format

Supply a JSON object with `buttons` (slot-to-saved-card-record mapping), `settings` including `button_order`, and optionally `subpages`. These are native configuration fields used by the repository's `nativeConfigState` browser fixture. Do not infer record syntax from the number of semicolons: inspect examples and current model code for the requested card type.

The included `assets/weather-example.json` shows all four weather modes using supported `card_off_color` options: cyan `00B8CF`, blue `0457D9`, sky blue `0083D6`, forecast blue `075ACB`. Use a feature-appropriate subset or palette when requested.

Common grid tokens for this square device are an unsuffixed slot for 1×1, `w` for 2×1, `b` for 2×2, `x` for 3×1, `h` for 3×2 and `q` for 3×3. Verify tokens against `src/webserver/model/grid.ts` when needed. Fit the screen rather than adding empty placeholder cells to the illustration.

Both supported photo templates show a square ESP32-P4-86 display and work with the same captured screen. The helper captures that device profile at a consistent scale. For another device shape, use a matching template and capture profile instead of stretching this square example.

## Harness compatibility

The helper exposes a small set of functions from `scripts/check_web_browser_smoke.js` before its main test entrypoint; it does not run the full test suite. If that file, entrypoint or helper names change, it stops with a compatibility error. Inspect and adapt the capture bridge to the current repository rather than editing production code or reusing a stale screenshot.

If a requested detail popup is not represented by the browser preview, explain that limitation. Use an actual supplied screen capture or implement a faithful source-based reference with explicit agreement; do not label a guessed UI as an actual screenshot.

## Local template setup

No original photo templates are tracked in the repository. Existing users can keep them in the personal skill's `assets` directory. For another machine, keep the supplied transparent-screen images in a local folder using the filenames `device-template.webp` and `device-template-2.webp` (convert PNG to lossless WebP when needed), then set `ESPCONTROL_MOCK_TEMPLATE_DIR` to that folder. The helper accepts lossless WebP or PNG content. You can also pass an explicit local image path instead of an alias.

An explicit `ESPCONTROL_MOCK_TEMPLATE_DIR` is authoritative: a missing file reports an error instead of falling back to a different directory. Both templates use the same square preview and `--glass` finish. Keep this local asset folder outside the checkout; only finished mockups belong in documentation assets.
