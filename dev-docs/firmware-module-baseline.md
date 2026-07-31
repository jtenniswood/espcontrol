# Firmware Module Migration Baseline

This baseline supports the incremental compiled-module migration described by
[ADR 0006](adr/0006-hybrid-compiled-firmware-modules.md). It compares the clean
`main` build at `f52b671d` with the foundation branch at `15fb7736`.

## Method

- ESPHome: `2026.6.5`
- Entry points: all six `builds/*.factory.yaml` files
- Cleanliness: `builds/.esphome` was removed before every build; the downloaded
  platform toolchain was retained between builds.
- Builds ran sequentially in the ESPHome container.
- Flash and static RAM are the final PlatformIO figures. `Linked` means the
  application ELF and combined factory image were both created successfully.

## Results

| Factory build | Main flash | Branch flash | Flash change | Main static RAM | Branch static RAM | RAM change | Main time | Branch time | Linker |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `esp32-p4-86` | 6,768,654 B | 6,768,654 B | 0 B (0.00%) | 116,136 B | 116,136 B | 0 B (0.00%) | 909.43 s | 932.93 s | Passed |
| `guition-esp32-p4-jc1060p470` | 3,890,828 B | 3,890,828 B | 0 B (0.00%) | 118,580 B | 118,580 B | 0 B (0.00%) | 481.88 s | 532.85 s | Passed |
| `guition-esp32-p4-jc4880p443` | 3,839,772 B | 3,839,772 B | 0 B (0.00%) | 117,668 B | 117,668 B | 0 B (0.00%) | 451.04 s | 453.07 s | Passed |
| `guition-esp32-p4-jc8012p4a1` | 4,212,644 B | 4,206,396 B | -6,248 B (-0.15%) | 117,944 B | 117,944 B | 0 B (0.00%) | 521.81 s | 473.24 s | Passed |
| `guition-esp32-p4-jc8012p4a1-v2` | 4,212,692 B | 4,206,452 B | -6,240 B (-0.15%) | 117,944 B | 117,944 B | 0 B (0.00%) | 522.66 s | 503.35 s | Passed |
| `guition-esp32-s3-4848s040` | 3,568,739 B | 3,568,739 B | 0 B (0.00%) | 123,416 B | 123,416 B | 0 B (0.00%) | 809.42 s | 415.85 s | Passed |

No device has a flash or static-RAM increase. The S3 main duration includes the
initial platform installation, so it is retained as the observed clean-build
time but should not be used as a performance comparison. Runtime heap and PSRAM
remain physical-device measurements for the later behaviour-module PRs.
