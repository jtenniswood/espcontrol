---
name: flash-displays
description: Flash EspControl display firmware from this repository using ESPHome. Use when the user invokes /flash-displays with no extra display name, or asks to flash, reflash, update, or upload firmware to all known displays in sequence, or to a specific display such as 7inch, 7-inch P4, 10inch, 10-inch V1, 10-inch V2, P4-86, 4.3-inch P4, 4-inch P4, or 4-inch S3, over an explicitly supplied OTA target or USB.
---

# Flash Displays

## Overview

Use the local development ESPHome configs to flash the known EspControl displays. If the user invokes `/flash-displays` with no additional display name or target, assume they mean all displays. Flash one requested display, or flash all displays in the fixed order below. Use OTA with the default hard-coded target unless the user provides a different target; use USB only when the user explicitly asks for USB.

## Device Map

| Request names | ESPHome config directory | Default OTA target |
|---|---|---|
| `7inch`, `7-inch`, `7inch P4`, `7-inch P4`, `JC1060P470` | `devices/guition-esp32-p4-jc1060p470` | `192.168.6.102` |
| `10inch`, `10-inch`, `10inch P4`, `10-inch P4`, `10inch V1`, `10-inch V1`, `JC8012P4A1` | `devices/guition-esp32-p4-jc8012p4a1` | `192.168.6.103` |
| `10inch V2`, `10-inch V2`, `JC8012P4A1 V2` | `devices/guition-esp32-p4-jc8012p4a1-v2` | Ask for the target |
| `4inch P4`, `4-inch P4`, `P4-86`, `86 Panel`, `Waveshare P4-86`, `esp32-p4-86` | `devices/esp32-p4-86` | `192.168.6.104` |
| `4.3inch P4`, `4.3-inch P4`, `P4 4.3inch`, `P4 4.3-inch`, `JC4880P443` | `devices/guition-esp32-p4-jc4880p443` | `192.168.6.101` |
| `4inch S3`, `4-inch S3`, `4848S040` | `devices/guition-esp32-s3-4848s040` | `192.168.6.105` |

Treat the 10-inch panel at `192.168.6.103`, and an ambiguous or default `10inch` request, as V1 hardware. Always flash that panel with the V1 `JC8012P4A1` configuration in `devices/guition-esp32-p4-jc8012p4a1`; never substitute the V2 configuration because that firmware will not work on this panel. Select the V2 directory only when the user explicitly requests the 10-inch V2 panel and supplies a different OTA target or explicitly requests USB.

All screens can also be flashed over USB when explicitly requested. Use the selected screen's config directory and the local serial target, normally `/dev/cu.usbmodem201301`.

If the user says only `4inch` or `4-inch`, ask whether they mean the 4-inch P4 screen or the 4-inch S3 screen.

For `/flash-displays` with no extra target, or for `all`, flash in this sequence by default over OTA using the default targets above:

1. 7-inch P4.
2. 10-inch P4 V1.
3. 4-inch P4 / P4-86.
4. 4.3-inch P4.
5. 4-inch S3.

## YAML Selection

Use `dev.yaml` by default. If the user names another YAML file, use that file instead.

- If the user explicitly says `dev`, `dev file`, or `dev.yaml`, use `dev.yaml` instead.
- If the user gives a bare filename such as `esphome.yaml`, resolve it inside the selected display's config directory.
- If the user gives a repo-relative path such as `devices/guition-esp32-p4-jc8012p4a1/esphome.yaml`, resolve it from the repository root.
- For the 10-inch panel at `192.168.6.103`, or an ambiguous/default 10-inch request, require the selected YAML to resolve inside `devices/guition-esp32-p4-jc8012p4a1`, which is the V1 configuration. Allow `devices/guition-esp32-p4-jc8012p4a1-v2` only when the user explicitly requests V2 and supplies a different OTA target or explicitly requests USB; otherwise stop and clarify instead of flashing it.
- Only use YAML files inside this repository. If the selected file does not exist, ask for the correct file instead of guessing.
- Use the required local secrets file described below. Do not create secret values or print, modify, copy, or commit the file contents.

## Secrets File

All development YAML files require a local `secrets.yaml` containing `wifi_ssid` and `wifi_password`. Use this existing file as the only secrets source:

```text
/Users/jtenniswood/Git/espcontrol/secrets.yaml
```

Before flashing each selected display:

1. Confirm the source exists with `test -f /Users/jtenniswood/Git/espcontrol/secrets.yaml`. If it is missing, stop and tell the user; do not create or guess secret values.
2. Run the following from the selected display's config directory. This creates the ignored local symlink only when `secrets.yaml` is absent, and verifies that any existing file or symlink resolves to the required source:

   ```bash
   SECRETS_SOURCE=/Users/jtenniswood/Git/espcontrol/secrets.yaml
   if [ ! -e secrets.yaml ] && [ ! -L secrets.yaml ]; then
     ln -s "$SECRETS_SOURCE" secrets.yaml
   fi
   test "$(realpath secrets.yaml)" = "$(realpath "$SECRETS_SOURCE")"
   ```

3. If verification fails, stop and explain that the selected display already has a different `secrets.yaml`; do not replace it without the user's approval.

Never display the secrets file, include its contents in command output, or add it to Git. The per-device `.gitignore` files exclude `secrets.yaml`.

## Workflow

1. Confirm the repository state:
   - Run `git status --short --branch`.
   - Use `main` as the source. If not on `main`, switch only when it is safe and there are no blocking local changes; otherwise explain the issue.
   - If the worktree is dirty, do not revert or commit unrelated changes. Tell the user the flash will use the current local checkout as-is.
   - If the worktree is clean, run `git pull --ff-only` before flashing.
2. Resolve the requested display names from the device map. If the user invoked `/flash-displays` without naming a display, resolve it as `all`. If the request is ambiguous, ask one short clarification.
3. Resolve the YAML file from the user's request. If none is provided, use `dev.yaml`.
4. Prepare and verify the required local `secrets.yaml` symlink in each selected display's config directory by following the Secrets File section. Do not print or commit the secrets.
5. Resolve OTA targets from an explicit user-supplied target first, then from the device's default hard-coded target. If a needed OTA target is missing, ask for that target or ask whether to use USB.
6. If the user says `USB`, `over USB`, `use USB`, `local`, or similar, use USB for the selected display instead of OTA.
   - For a single display, use that display's config directory and the USB target.
   - For `all over USB`, flash the displays in the normal all-display sequence, but ask the user to connect the correct display before each USB flash if the connected device is not clearly identifiable.
7. For OTA targets, check reachability first with `ping -c 2 -W 1000 <target>`.
8. For USB flashing:
   - List ports with `ls -1 /dev/cu.*`.
   - Prefer `/dev/cu.usbmodem201301` when present.
   - If that port is missing and exactly one obvious `/dev/cu.usbmodem*` port exists, use it.
   - If no clear USB modem port exists, ask the user to connect the display or choose the port.
9. Flash each selected display with the command below, running displays sequentially. Do not run multiple flashes in parallel.
10. After each OTA flash, ping the target again. A first ping may fail during reboot; retry once after a short delay before reporting a problem.
11. Do not commit or push for flashing alone. Commit/push only if this skill or other source files were intentionally changed as part of the user request.

## Commands

Use this substitution so ESPHome builds from the local repository checkout:

```bash
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run <yaml-file> --device <target> --no-logs
```

Run from the appropriate config directory:

```bash
# 7-inch P4 over OTA
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-p4-jc1060p470
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device 192.168.6.102 --no-logs

# 7-inch P4 over USB, only when explicitly requested
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-p4-jc1060p470
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device /dev/cu.usbmodem201301 --no-logs

# 10-inch P4 V1 over OTA
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-p4-jc8012p4a1
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device 192.168.6.103 --no-logs

# 10-inch P4 V1 over USB, only when explicitly requested
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-p4-jc8012p4a1
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device /dev/cu.usbmodem201301 --no-logs

# 4-inch P4 / P4-86 over OTA
cd /Users/jtenniswood/Git/espcontrol/devices/esp32-p4-86
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device 192.168.6.104 --no-logs

# 4-inch P4 / P4-86 over USB, only when explicitly requested
cd /Users/jtenniswood/Git/espcontrol/devices/esp32-p4-86
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device /dev/cu.usbmodem201301 --no-logs

# 4.3-inch P4 over OTA
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-p4-jc4880p443
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device 192.168.6.101 --no-logs

# 4.3-inch P4 over USB, only when explicitly requested
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-p4-jc4880p443
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device /dev/cu.usbmodem201301 --no-logs

# 4-inch S3 over OTA
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-s3-4848s040
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device 192.168.6.105 --no-logs

# 4-inch S3 over USB, only when explicitly requested
cd /Users/jtenniswood/Git/espcontrol/devices/guition-esp32-s3-4848s040
esphome -s espcontrol_component_url file:///Users/jtenniswood/Git/espcontrol run dev.yaml --device /dev/cu.usbmodem201301 --no-logs
```

## Reporting

Keep user updates concise:

- Say which display is currently compiling/uploading.
- Mention known ESPHome warnings only if they affect the result; framework, platform, GPIO19/GPIO20, and MIPI narrowing warnings are normally non-blocking.
- Final response: list each requested display as flashed successfully, or clearly identify the display that failed and the blocking symptom.
