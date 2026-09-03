# Development Environment

This page owns the local toolchain requirements. Workflow-specific commands and
checks live in [Task Playbooks](playbooks/README.md).

## Required Tools

| Tool | Used for | Repository expectation |
|---|---|---|
| Git | Branches, worktrees, diffs, and commits | Use an isolated worktree for normal changes. |
| Node.js and npm | TypeScript, web bundles, VitePress, JavaScript checks, and task entry points | CI uses Node.js 24. Run `npm ci` after checkout or lockfile changes. |
| Python 3 | Generators, validators, the check graph, and local ESPHome wrapper | Python must include `venv` support for CI-equivalent dependency isolation. |
| CMake and CTest | Host-side firmware tests in `check:fast` | CMake 3.20 or newer. CI falls back to 3.31.10. |
| C++ compiler | Firmware parser, modal-layout, and saved-config host checks | A C++17 compiler available as `c++`, `g++`, or `clang++`. |
| Playwright Chromium | Browser-level configurator checks | The npm package is pinned by `package-lock.json`; install its Chromium runtime separately. |
| ESPHome CLI | Local compile, flash, and log workflows | Match `ESPHOME_VERSION` in `.github/esphome.env`; `scripts/local_esphome.py` rejects a different version. |
| Docker | Full firmware matrix, nightly, and release builds | A running Docker daemon able to pull the pinned ESPHome image. |
| GitHub CLI | PR and release-maintainer workflows | Authenticate before using repository release or PR automation. |

## Install Project and Browser Dependencies

```bash
npm ci
npx playwright install chromium
```

On Linux, use Playwright's dependency installer when the host libraries are not
already available:

```bash
npx playwright install --with-deps chromium
```

Install CMake 3.20 or newer and a C++17 compiler through the host package
manager. The CI fallback version is documented in `.github/workflows/ci.yml`.

For local firmware work, install the ESPHome version named in
`.github/esphome.env` into the active Python environment. Full matrix and
release firmware builds use the matching `ghcr.io/esphome/esphome` Docker image
instead of the host CLI.

## Verify the Environment

```bash
node --version
npm --version
python3 --version
cmake --version
c++ --version
docker info
npx playwright --version
```

Then verify generated output and the broad local check graph:

```bash
python3 scripts/build.py --check
npm run check:fast
```

Browser checks additionally require the installed Chromium runtime. Firmware
compiles require either the pinned host ESPHome CLI for local device work or
Docker for the repository's matrix/release workflow.
