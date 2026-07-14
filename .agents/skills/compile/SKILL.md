---
name: compile
description: >-
  Test compile ESPHome firmware for all devices using Docker and fix any YAML or
  configuration errors until every device compiles successfully. Use when the
  user says "/compile", "test compile", "compile check", "build test",
  "check builds", "verify compile", or wants to make sure all devices compile
  before committing.
---

# Test Compile - Build All Devices

Compile ESPHome firmware for every device in the repo using Docker. If a build
fails, diagnose the error, fix the source YAML, and re-compile until all
devices pass.

## Device Slugs

```text
guition-esp32-p4-jc1060p470
guition-esp32-p4-jc4880p443
guition-esp32-s3-4848s040
```

## Workflow

### 1. Compile Each Device

Run one Docker compile per device. Use `builds/<slug>.factory.yaml`, the same
entry point CI uses, so the test matches what a release build would do.

```bash
docker run --rm \
  -v "/Users/jtenniswood/Library/CloudStorage/Dropbox/Git/espcontrol:/config" \
  ghcr.io/esphome/esphome:2026.6.5 \
  compile /config/builds/<slug>.factory.yaml
```

Run devices sequentially because each compile is resource-intensive. Use a
10-minute timeout or block window when available so compiles are not treated as
stalled too early. If a compile backgrounds, poll the terminal output until it
finishes and read the result.

If `ghcr.io/esphome/esphome:2026.6.5` is missing, pull it first:

```bash
docker pull ghcr.io/esphome/esphome:2026.6.5
```

### 2. Interpret Results

- Success: output ends with `Successfully compiled program` or
  `INFO Successfully compiled program.`
- Failure: inspect `ERROR` lines and the file/line references around them.

Common failure categories:

- YAML syntax errors, including indentation and missing keys
- Missing `!include` targets or wrong relative paths
- Unknown component or platform
- Undefined substitution variables
- Type mismatches, such as a string where an integer is expected
- Missing `packages:` keys

### 3. Fix and Retry on Failure

When a device fails:

1. Read the full error output carefully.
2. Identify the failing file and line from the error message.
3. Open and read that file to understand context.
4. Fix the root cause in source YAML.
5. Re-run the same Docker compile command for that device.
6. Repeat until it succeeds, then move to the next device.

Do not skip a device or mark it as known broken. Every listed device must
compile.

Fixes should target files under `common/`, `devices/`, or `components/` when
possible, not the `builds/*.yaml` wrappers because those are thin entry points.

### 4. Web UI Asset Note

The factory YAML bundles `www.js` via `js_include`. If the referenced JavaScript
file is missing, compile will fail. Run this first when web UI assets may be out
of date:

```bash
python3 scripts/build.py
```

### 5. Report

After all devices pass, summarize:

```text
Compile results:
  guition-esp32-p4-jc1060p470  OK
  guition-esp32-p4-jc4880p443  OK
  guition-esp32-s3-4848s040    OK

Fixes applied:
  - <file>: <what was wrong and what was changed>
```

If no fixes were needed, show only the passing results.
