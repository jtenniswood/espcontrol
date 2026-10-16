# Weather documentation images

The device image combines the user's supplied transparent-screen photo template with the actual ESP32-P4-86 editor preview. A single projective transform fits the entire screenshot to the template's four screen corners. No image generation is used in the final image. The photograph outside the cutout remains pixel-identical, verified after lossless WebP encoding. The screen contains sample data and is not evidence of hardware testing.

## Saved assets

- `docs/public/images/weather-device-mockup.webp` — photo template with the weather preview composited into its screen.
- `docs/public/images/weather-admin-current.png` — Current Conditions editor.
- `docs/public/images/weather-admin-today.png` — Today's Forecast editor.
- `docs/public/images/weather-admin-tomorrow.png` — Tomorrow's Forecast editor.
- `docs/public/images/weather-admin-daily.png` — Daily Forecast editor.
- `assets/documentation/weather-screen-reference.png` — actual preview screenshot used as the screen artwork.
- `assets/documentation/compose-screen.cjs` — reusable screen compositing helper, requiring Node.js and `sharp`.

## Recreate the device image

The original photo template is kept outside the repository. Supply your local copy of `Template.png`. From the repository root, with `sharp` available on Node's module path:

```sh
node assets/documentation/compose-screen.cjs \
  /path/to/Template.png \
  assets/documentation/weather-screen-reference.png \
  docs/public/images/weather-device-mockup.webp
```

The helper uses the template's transparent screen mask and detects its corners: top left `(1008, 357)`, top right `(2235, 567)`, bottom right `(1943, 1874)`, bottom left `(862, 1505)`. It removes the preview's four-pixel simulated enclosure outline, fits all screen content with one perspective transform, and blends the cutout's antialiased edge. It checks corner alignment and verifies all 5,366,835 pixels outside the cutout remain unchanged after encoding. Keep the source template unchanged when using it for other feature examples.

## Capture the editor screenshots

The admin screenshots were captured with Playwright using the existing browser smoke harness's local routes, simulated EventSource, and native configuration loader. They show the actual editor with a simulated device, not a live panel.

The preview uses `esp32-p4-86`, four weather cards with modes `""`, `today`, `tomorrow`, `daily`, and grid order `1,2,3,4h`. Each card uses `weather.forecast_home`. Open the daily card's editor and select each Type to capture the four settings dialogs. No live panel or Home Assistant credentials are needed.
