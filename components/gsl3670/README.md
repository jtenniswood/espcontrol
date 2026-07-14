# GSL3670 external component

Silead GSL3670 capacitive touch driver for the Seeed reTerminal D1001 (8" P4
panel). Adapted from the sibling `gsl3680` driver in this repository; the two
Silead controllers share the same boot/firmware-upload protocol.

The firmware table in `gsl3670_firmware.h` is the D1001's blob, taken from
Seeed's reTerminal D1001 BSP (github.com/Seeed-Studio/reTerminal-D1001).

Local changes:

- Make the touch reset pin optional and accept any `GPIOPin`. On the D1001 the
  touch reset line lives on the XL9535 I2C expander and the BSP leaves it
  unused, so no reset pin is wired.

License details from the source repository are included in `LICENSE.md`.
