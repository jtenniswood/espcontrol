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

Firmware-related pull requests compile one representative device for each chip
family, including its P4 recovery image where applicable. The nightly workflow
and releases compile every supported device and recovery image. The **Firmware
Compile Gate** check fails if any required build fails. Changes outside firmware
inputs skip compilation. PR, nightly and release build summaries report
application size, OTA partition capacity and remaining headroom; less than
128 KiB free produces a warning, and an oversized image fails the build. Sizes
use the compiled application binary and partition table, not the larger USB
factory image.

## Testing Web Connections on a Device

After flashing a branch, run this from the repository root in the ESPHome Python
environment described above. Replace the IP address with the display you want
to test:

```sh
python scripts/test_device_web_transport.py --host 192.168.1.50 \
  --native-api --require-stall-cleanup --firmware-label "my-test-branch"
```

The suite checks the version, editor response and native configuration, opens
and closes 80 event streams, holds two non-reading streams for 45 seconds while
checking healthy connections, then checks recovery. It checks for reboots using
device uptime and verifies that the saved configuration is unchanged. With
`--native-api`, it also monitors API connectivity and firmware error logs.
`--require-stall-cleanup` requires at least one timeout-cleanup message; a run
that never exercises that path fails the coverage check. The report records the
actual count. Other stalled connections may be closed by the server's normal
connection-limit recovery instead of reaching the timeout.

The command uses GET requests and diagnostic subscriptions. It does not flash,
change settings, import backups, or invoke Home Assistant controls. Run it while
using the affected camera/media pages to include that workload. Avoid changing
settings during the run, because that will fail the configuration comparison.

Reports are saved as timestamped JSON and Markdown files under
`.cache/device-web-transport/`. Use `--report-dir` to choose another location and
`--firmware-label` to distinguish runs. The label is user-supplied; the report
also records the device's reported version and, with API monitoring, compilation
time. Exit codes are `0` for a passed run, `1` for a failed check, `2` for setup or
argument errors, and `130` for an interrupted run. Reports do not include
configuration contents, credentials or raw firmware logs.

For an encrypted API, supply the existing key through an environment variable
and add `--api-noise-psk-env ESPCONTROL_API_NOISE_PSK`. Web authentication can use
`--web-username-env` and `--web-password-env`; both take environment-variable
names, not credential values. Do not put keys or passwords in command arguments.

HTTP-only checks need only Python's standard library:

```sh
npm run test:device-web -- --host 192.168.1.50
```

That mode explicitly reports native API checks and cleanup-log coverage as not
monitored. `--reconnects`, `--stall-seconds`, `--stalled-clients`, `--interval` and
`--timeout` can adjust the load. Shortening the run can prevent the firmware's
stalled-client timeout from being reached. Use `--stalled-clients 1` on a
constrained device or when other browser tabs are occupying its HTTP sockets.

The fake-device runner tests are included in CI and can be run independently
with `npm run check:device-web-transport-tests`; CI never contacts a physical panel. The
suite cannot detect a blue frame on the physical display or verify touch, so
those observations still need a person or a separate recording on an affected
device.
