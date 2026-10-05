---
title: Contributing
description:
  How to get started if you want to contribute fixes, card types, firmware changes, or web configurator improvements to EspControl.
---

# Contributing

Thanks for wanting to help improve EspControl.

## Before Opening a Pull Request

- Check the existing [issues](https://github.com/jtenniswood/espcontrol/issues)
  to see if the change is already being discussed.
- Keep the change focused so it is easier to test.
- Run `npm run prepare:ci` before submitting code changes, then review and commit
  the regenerated files alongside your source changes.
- If your change affects how users configure or use the panel, update the docs
  in this site too.

For small fixes, a pull request is usually enough. For larger changes, especially
new card types or firmware behaviour, opening an issue first makes it easier to
agree what should change before you spend time building it.

## Preparing a Branch

Use Node.js 24, Python 3.12 or newer, CMake 3.20 or newer, and a C++ compiler.
Set up the Python dependencies in a virtual environment to match CI:

```sh
python3 -m venv .venv
. .venv/bin/activate
. .github/esphome.env
python -m pip install "esphome==${ESPHOME_VERSION}" "tzdata==2026.3"
export PYTHONTZPATH="$(python -c 'import pathlib, tzdata; print(pathlib.Path(tzdata.__file__).parent / "zoneinfo")')"
npm run prepare:ci
```

The preparation command performs a clean install from the lockfile, checks that
the authored product definitions agree, and regenerates the device manifest,
device YAML, web and firmware assets, and product snapshot. It then runs the CI
check suite, MIPI model check, docs build, and browser installer check. Chromium
is installed automatically; on a Linux machine missing its system libraries,
run `npx playwright install-deps chromium` once.

Review `git diff` before committing. Preparation does not rewrite compatibility
expectations, increase bundle-size budgets, or reconcile conflicting authored
product definitions. Fix those deliberately when a check reports them. For a
dependency update, regenerate and commit `package-lock.json` using Node.js 24;
preparation rejects a lockfile that no longer matches `package.json`.

For checks without regeneration, use `npm run check:ci`. It reports all
independent failures in one run and skips checks whose prerequisites failed.
The fast and release profiles still stop at the first failure; use
`npm run check:fast -- --keep-going` when investigating several failures locally.

The web bundle has explicit limits of 2,750,000 minified bytes and 1,150,000 gzip
bytes, initially about 5% above its measured size. Ordinary growth and shrinkage
within those limits are allowed; the check reports remaining space. Review
changes to `compatibility/fixtures/web_migration_baseline.json` explicitly if a
feature needs more space. Saved-config and migration expectations remain exact.

## Pull Request Testing

Pull requests should make the next step obvious. The PR template asks for the
practical impact, documentation decision, testing status, and whether a physical
device test is needed before merge.

Description edits rerun the separate PR Process check without restarting code
CI. The CI Gate check covers the code and docs jobs, which run independently so
a code-check failure does not hide a docs-build failure.

When a pull request changes firmware, device configuration, shared display
files, or web files used by the device, an automated workflow posts suggested
device testing notes. Treat those notes as the starting point for manual device
testing after the automated checks pass.

A successful compile or CI run confirms that the project builds. It does not
replace testing on the affected display when the change touches the on-device
experience.

Firmware-related pull requests automatically compile every supported factory and
P4 recovery image. The **Firmware Compile Gate** check fails if any required
build fails. Changes outside firmware inputs skip compilation. PR, nightly and
release build summaries report application size, OTA partition capacity and remaining
headroom; less than 128 KiB free produces a warning, and an oversized image fails
the build. Sizes use the compiled application binary and partition table, not the
larger USB factory image.
