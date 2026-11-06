# Temporary P4 V3 display fix

Vendored from ESPHome **2026.9.0**, as shipped in
`ghcr.io/esphome/esphome:2026.9.0`, with its original license in `LICENSE`.
Upstream: https://github.com/esphome/esphome/tree/2026.9.0/esphome/components/mipi_dsi

The PHY clock fix in `MipiDsi::setup()` value-initializes `phy_clk_src`
instead of assigning the deprecated `MIPI_DSI_PHY_CLK_SRC_DEFAULT` alias.
ESP-IDF 5.5.5's `esp_lcd_new_dsi_bus()` explicitly maps zero to the correct
default for the selected silicon: XTAL for production P4, PLL_F20M for legacy
P4. The deprecated alias always selects PLL_F20M, which aborts in the V3 HAL.

Two additional fixes address issue #2118:

- Normalize the requested DPI clock to the clock the hardware divider actually
  produces, and compensate the horizontal front porch before creating the panel.
  This preserves the intended frame rate (within pixel rounding) while keeping
  host and bridge timing consistent. For example, 70 MHz on a 240 MHz source
  becomes 80 MHz with a longer front porch. This works around the ESP-IDF timing
  defect discussed in https://github.com/espressif/esp-idf/issues/19188 without
  modifying framework files. Impossible compensated timings fail setup clearly.
- Wait for drawing completion only after a successful submission. Failed writes
  have no completion callback and previously left the display task waiting forever.

Panel initialization sequences, models and saved user settings are unchanged.

The JC8012P4A1 V3 and JC4880P443 V3 profiles, plus the separate 10.1-inch
V3 test package, load this external component; other devices continue to use ESPHome's bundled driver.
Keep the copy pinned to the tested ESPHome version and remove it when the
upstream fixes are available.

Diagnosis and XTAL experiments were supplied by Horstexplorer and
MichaelMKKelly in https://github.com/jtenniswood/espcontrol/pull/1954.
