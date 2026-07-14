# reTerminal D1001 — audio & battery add-ons

Reference for the battery/power, media-player, and microphone packages added to
the Seeed reTerminal D1001 device config. These are additive packages the
device includes via the manifest `firmware.package.extraPackages` map:

| Package | File | Feature |
|---|---|---|
| `power` | `device/power.yaml` | Battery & external-power reporting (A) |
| `audio` | `device/audio.yaml` | Speaker + media player (B) |
| `microphone` | `device/microphone.yaml` | Room noise + sound detection (C) |

All hardware constants are traced to Seeed's reTerminal D1001 BSP
(`github.com/Seeed-Studio/reTerminal-D1001`) and the "Pin Assignment" tables on
the Seeed wiki. BSP citations use `H:` = `.../include/esp32_p4_re_terminal_d1001.h`
and `C:` = `.../src/esp32_p4_re_terminal_d1001.c`.

## Pin map

### Audio (two separate I2S buses)
The D1001 wires ES8311 (output) and ES7210 (mics) on **separate** I2S
peripherals — unlike the Waveshare p4-86 which shares one full-duplex TDM bus
via the `esp_audio_stack` external component. So this device uses stock
`i2s_audio` with one bus each; mic and speaker run concurrently.

| Signal | ESP32-P4 pin | Device | Source |
|---|---|---|---|
| ES8311 MCLK / BCLK / WS / DOUT | 33 / 32 / 31 / 30 | Speaker (out) | wiki; H:L63-67 |
| ES7210 MCLK / BCLK / WS / DIN | 29 / 28 / 27 / 26 | Mics (in) | wiki; H:L57-61 |
| ES8311 I2C address | `0x18` on expander bus (GPIO20/21) | Speaker DAC | schematic p6 (`I2CADDR:0x18`); C:L429-430 |
| ES7210 I2C address | `0x40` on expander bus (GPIO20/21) | Mic ADC | schematic p6 (`Codec I2C ADDR:0x40`); C:L475-476 |
| Amp enable (EN_PA) | `xl9535` pin **13** (Seeed "P13" = BSP bit 11) | Class-D amp | wiki PA table; H:L69, C:L1418 |

### Battery & power
| Signal | ESP32-P4 pin | Detail | Source |
|---|---|---|---|
| Battery voltage | GPIO18 (ADC1_CH2) | `mV = adc_cali × 2` (2:1 divider), 12 dB / 12-bit | C:L928, L1091 |
| Battery-read enable | `xl9535` pin 6 (HIGH) | gates the divider | H:L85, C:L1415 |
| VBUS / USB voltage | GPIO17 (ADC1_CH1) | `mV = adc_cali × 2`; present > 4000 mV | C:L922, L1090, L1133 |
| Charge status | GPIO15 | HIGH = done, LOW = charging (inverted) | H:L82, C:L1042-1046 |
| Battery % curve | — | Seeed 21-pt discharge table, 3262 mV→0 %, 4047 mV→100 % | C:L97 |

## Home Assistant entities

| Entity | Type | Package |
|---|---|---|
| Battery Voltage | sensor (V, diagnostic) | power |
| Battery Level | sensor (%, `battery`) | power |
| Charging | binary_sensor (`battery_charging`) | power |
| External Power | binary_sensor (`plug`) | power |
| Power Outage | binary_sensor (`problem`) | power |
| Speaker | media_player | audio |
| Noise Level | sensor (dB, diagnostic) | microphone |
| Sound Detected | binary_sensor (`sound`) | microphone |
| Sound Detection Threshold | number (config) | microphone |
| Microphone Mute | switch | microphone |

Power Outage also sets a device-local global `power_outage_active` for the
on-screen alert banner (separate task), since HA may be unreachable in an outage.

## Memory / CPU impact

Base D1001 firmware (screen only) compiled at **RAM 23.2 %** (118 712 / 512 000 B
internal) and **Flash 51.3 %** (≈4.17 MB / 8.13 MB). The audio/mic packages push
work into PSRAM (`task_stack_in_psram` / `buffers_in_psram` on the mixer,
resamplers, media_player) to protect the internal-RAM/LVGL budget. Exact deltas
should be read from a full `esphome compile` of the current config (the config
validates on ESPHome 2026.6.4; a full compile in CI reports the new totals).

## Tuning knobs

| Knob | Where | Default | Notes |
|---|---|---|---|
| Speaker max volume | `audio.yaml` `volume_max` | `0.85` | protects the 2 W mono driver |
| Speaker initial volume | `audio.yaml` `volume_initial` | `0.5` | |
| Amp settle delay | `audio.yaml` `on_play`/`on_announcement` | `100 ms` | raise if the amp clicks/pops on start |
| Mic gain | `microphone.yaml` `mic_gain` | `24db` | ES7210 analog gain |
| Sound detection threshold | HA number **Sound Detection Threshold** | `-45 dB` | **calibrate on-device** (see below) |
| Sound Detected release | `microphone.yaml` `delayed_off` | `12 s` | occupancy-style hold |
| Battery telemetry rate | `power.yaml` `update_interval` | `60 s` | |
| Outage poll / debounce | `power.yaml` (VBUS 2 s, `delayed_on` 4 s) | — | outage trips within ~5 s |

## Flagged for on-device verification

Config validates, but these could only be confirmed against real hardware:

1. **Sound level dB scale** — ESPHome `sound_level` reports dBFS (0 = full scale,
   negative below); the `-45 dB` threshold is a starting guess. Watch the **Noise
   Level** entity in a quiet vs. talking room and set **Sound Detection
   Threshold** accordingly.
2. **Concurrent mic + speaker + LVGL** — two I2S buses plus the display animating;
   watch for DMA/PSRAM contention or audio stutter and adjust buffer durations /
   task priorities if needed.
3. **Amp gating pop** — if enabling the amp on play produces an audible pop,
   increase the `on_play` settle delay.
