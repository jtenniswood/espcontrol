---
name: mock-feature-screens
description: Create realistic EspControl feature screen mockups by fitting actual editor screenshots into the local device photo templates, with custom card colours, soft reflections and gentle blur. Use for device feature illustrations and requested configuration screenshots or documentation integration; not for generic image generation or changing firmware behaviour.
---

# Mock Feature Screens

Create the same style as the approved Weather example: a real device photo, correctly projected UI, blue/cyan cards where appropriate, and a restrained glass finish. This is the user's chosen screenshot-and-template compositing workflow. Preserve the enclosure, desk and perspective; do not regenerate the photo or invent an interface.

## Build the screen

- Resolve the active EspControl worktree with `git rev-parse --show-toplevel`. Inspect the relevant card documentation, `product/v2/cards/` and `src/webserver/cards/` for supported settings and preview behaviour.
- Render the actual web editor using sample configuration. Keep the example focused on the requested feature; use fictitious entities and supported configuration options. Use realistic card sizes and related examples rather than enlarging a simple one-value card to fill the display. Let the real preview choose typography, icons and layout.
- Use [references/capture.md](references/capture.md) for the capture command and fixture format. The bundled `assets/weather-example.json` reproduces the established four-mode example; adapt the configuration for other features.
- Capture configuration dialogs only when requested or useful to the requested documentation. Leave browser screenshots crisp; apply the glass effect only to the photographed device.

## Composite the device

Use `scripts/compose-screen.cjs` with a template alias or an explicit local photo path. Source photos are not bundled with this repository skill.

| Template | Alias / local filename | Appearance |
|---|---|---|
| Original | `original` / `device-template.webp` | Original device angle and tabletop scene |
| Template 2 | `template2` / `device-template-2.webp` | Alternate angle with a darker enclosure and wider desk view |

Use the template the user requests. Otherwise, vary between the two templates for new mockups to avoid repetition: prefer the one used least recently in the current set of feature images, and alternate them when producing several together. If there is no recent example to compare, either template is suitable. Keep the existing template when revising an image unless the user asks to change it. Aliases load from `ESPCONTROL_MOCK_TEMPLATE_DIR` when set. Otherwise they use the existing personal skill assets at `${CODEX_HOME:-$HOME/.codex}/skills/mock-feature-screens/assets`. To use Template 2, substitute `template2` in this command:

```sh
node <skill-dir>/scripts/compose-screen.cjs \
  original \
  <screen-preview.png> <feature-device.webp> --glass
```

Requires Node.js, the repository web development dependencies, Playwright Chromium and `sharp`. Locate Node.js and `sharp` with `load_workspace_dependencies` when available. Use the returned Node executable and set `NODE_PATH` to the returned package directory if needed; do not assume they are on the shell PATH. If dependencies are missing, set them up locally without changing product dependencies just to create an image.

The compositor fits the entire UI with one projective transform, removes the preview's four-pixel enclosure border and uses the template's transparent cutout. `--glass` applies the approved slight blur and diffuse reflection. Keep this finish unless the user requests a change; avoid dramatic glare or blurring text beyond readability. Omit the flag for a crisp comparison.

## Check and deliver

- Inspect the finished image: labels and forecast values are legible, card edges follow the device plane, and reflections stay on the screen. The compositor verifies corner alignment and every pixel outside the cutout after lossless encoding. Fix source configuration rather than painting over incorrect UI.
- Keep source photos outside the repository, either in the existing personal skill asset folder or a local folder specified with `ESPCONTROL_MOCK_TEMPLATE_DIR`. If a template is unavailable, ask for a local template rather than substituting a different photograph. Creating a finished mockup does not authorize publishing its source photo.
- Save the finished image in the requested workspace. For repository documentation, follow its branch/worktree and PR instructions. Lead the feature page with the device image, add useful alt text and a short sample-data caption, and keep the setup and usage guidance concise. Validate the built page at desktop and mobile sizes. Do not claim publication until deployment is verified.
- If the request is only for an image, return that image without editing docs. Show the finished result and saved path; describe it as an illustrative screen composite, not hardware-test evidence.
