# Temporary P4 V3 display fix

Vendored from ESPHome **2026.9.0**, as shipped in
`ghcr.io/esphome/esphome:2026.9.0`, with its original license in `LICENSE`.
Upstream: https://github.com/esphome/esphome/tree/2026.9.0/esphome/components/mipi_dsi

The sole source change is in `MipiDsi::setup()`: value-initialize `phy_clk_src`
instead of assigning the deprecated `MIPI_DSI_PHY_CLK_SRC_DEFAULT` alias.
ESP-IDF 5.5.5's `esp_lcd_new_dsi_bus()` explicitly maps zero to the correct
default for the selected silicon: XTAL for production P4, PLL_F20M for legacy
P4. The deprecated alias always selects PLL_F20M, which aborts in the V3 HAL.
All display models, timings, initialization and drawing code are unchanged.

Only the JC8012P4A1 V3 test package loads this external component; existing
devices continue to use ESPHome's bundled driver. Keep the copy pinned to the
tested ESPHome version and remove it when the upstream fix is available.

Diagnosis and XTAL experiments were supplied by Horstexplorer and
MichaelMKKelly in https://github.com/jtenniswood/espcontrol/pull/1954.
