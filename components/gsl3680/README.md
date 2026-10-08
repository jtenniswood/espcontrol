# GSL3680 external component

Vendored from `kvj/esphome@dca6f3eed895ee03b894a7d172855c919ee7eda1`.

This copy keeps the 10-inch P4 touchscreen driver in the same release stream as the dashboard firmware so crash fixes can be shipped without waiting for the upstream branch.

Local changes:

- Decode all five points returned by the controller before calling the vendor touch tracking routine.
- Feed the watchdog during the long firmware upload to the touch controller.
- Configure the scan core through the touch-count register (`0x80`), select
  firmware pages with four-byte writes, and verify the uploaded RAM before starting.
- Wait up to 300ms for the running marker and retry initialization once before
  reporting a startup failure. Logs distinguish RAM readback from startup failure.

License details from the source repository are included in `LICENSE.md`.
