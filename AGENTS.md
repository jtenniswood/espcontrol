# EspControl — Agent Instructions

EspControl is a firmware project that turns affordable ESP32 touchscreens into
smart home control panels for Home Assistant. It ships firmware, a web-based
setup UI, configuration tools, and documentation.

**Non-developer context:** The project owner is technical but not a software
developer. When explaining changes, use plain language — imagine describing
what you did to a friend who understands smart home gear and networking but
does not read code for a living. Avoid jargon unless it is necessary, and
when you use a technical term, briefly say what it means.

## Repository Structure

- `devices/` — ESPHome YAML definitions for each supported touchscreen
- `components/` — Custom ESPHome components (C++ and Python)
- `common/` — Shared templates, includes, and configuration used across devices
- `src/` — Web UI source (the setup page served by the panel)
- `docs/` — VitePress documentation site
- `scripts/` — Build, check, and release helper scripts
- `builds/` — Pre-built firmware binaries
- `.agents/skills/` — Agent skill definitions for compile, PR, release, and update workflows

## How to Work on Changes

1. **Always work on a feature branch — never commit directly to `main`.**
2. **Create a git worktree** for each task so multiple features can be developed
   at the same time without stepping on each other:
   ```bash
   git worktree add -b feature/<short-name> /tmp/espcontrol-<short-name> main
   ```
3. **Work inside the worktree.** Do all file edits, builds, and tests there.

## Committing and Pushing

- **Commit early and often** — each logical change should be a single commit
  with a clear message describing what changed and why.
- **Push the branch** after every commit so the work is backed up and available
  on GitHub for testing:
  ```bash
  git push -u origin feature/<short-name>
  ```
- **Never use the word "Codex"** in branch names, commit messages, or PR titles.
  Describe the change itself, not the tool that made it.

## Pull Requests

- Open a PR from the feature branch into `main` once the change is ready.
- PR descriptions should explain **what changed, why, and how to test it on a
  device.** Write for a non-developer reader.
- Keep `main` always releasable. PRs should be small enough to review and
  test on a real touchscreen before merging.

## Testing on Devices

- The primary test target is a real ESP32 touchscreen running the firmware.
- The `.agents/skills/compile` skill can test-compile all devices via Docker.
  Use it before committing if you are unsure whether YAML changes are valid.
- When describing how to test, give step-by-step instructions a non-developer
  can follow: what to flash, what to look for on screen, what Home Assistant
  entity to check.

## Style and Conventions

- YAML files use 2-space indentation.
- Commit messages use imperative mood ("Add relay support" not "Added relay support").
- Keep device YAML readable — these files are maintained by hand and should
  make sense to someone reading them in a text editor.
- When adding or changing user-visible features, update the relevant docs in
  `docs/` as part of the same PR.
