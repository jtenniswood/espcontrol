# GSL3680 external component

Vendored from `kvj/esphome@dca6f3eed895ee03b894a7d172855c919ee7eda1`.

This copy keeps the 10-inch P4 touchscreen driver in the same release stream as the dashboard firmware so crash fixes can be shipped without waiting for the upstream branch.

Local changes:

- Clamp the reported touch count to the two points read by this driver before calling the vendor touch tracking routine.
- Feed the watchdog during the long firmware upload to the touch controller.

License details from the source repository are included in `LICENSE.md`.
