# Native Power surface regression

This optional test uses real LVGL software drawing and flushed RGB565 pixels,
not the host UI doubles. Use LVGL from a build made with the ESPHome version in
`.github/esphome.env`; nothing is downloaded or vendored by this test.

```sh
cmake -S tests/native_power_surface -B build/tests/native-power-surface \
  -DESPCONTROL_LVGL_DIR=/path/to/managed_components/lvgl__lvgl \
  -DESPCONTROL_RENDER_SANITIZERS=ON
cmake --build build/tests/native-power-surface
ctest --test-dir build/tests/native-power-surface --output-on-failure
```

The existing UI-test generator extracts the production Flow surface, Sensor
background, grid-card creation and display-correction functions. Checks compare
an actually rendered native masked Sensor card with Flow in both themes, verify
unmasked foreground colours and opaque backing against changing Home content,
exercise profile-sized surfaces, correction and repeated cleanup. Standard host
tests cover the production open/layout path and foreground object ancestry.

This is a software rendering regression, not a physical framebuffer capture or
measurement of ESP32 stack, heap, DMA or display-driver behaviour. Hardware
retesting remains necessary.
