# Weather documentation images

The admin screenshots were captured from the actual ESP32-P4-86 web editor on this branch, using a local simulated device and `weather.forecast_home`. They contain sample configuration, not a connection to a user's panel. The four screenshots show the four values in **Type**.

The device image is an AI-edited illustration, made with the built-in image generation tool. The supplied ESP32-P4-86 photograph is the edit target, retaining the casual desk setting, background devices, reflections and camera angle. A capture of the actual editor preview supplies the replacement screen content. It is not evidence of hardware testing. Weather values are illustrative.

## Saved assets

- `docs/public/images/weather-device-mockup.webp` — generated device illustration, compressed to WebP without changing its content.
- `docs/public/images/weather-admin-current.png` — Current Conditions editor.
- `docs/public/images/weather-admin-today.png` — Today's Forecast editor.
- `docs/public/images/weather-admin-tomorrow.png` — Tomorrow's Forecast editor.
- `docs/public/images/weather-admin-daily.png` — Daily Forecast editor.
- `assets/documentation/weather-screen-reference.png` — actual preview screenshot used as the screen reference for generation.

The screenshots were captured with Playwright using the existing browser smoke harness's local routes, simulated EventSource, and native configuration loader. The preview uses `esp32-p4-86`, four weather cards with modes `""`, `today`, `tomorrow`, `daily`, and grid order `1,2,3,4h`. Each card uses `weather.forecast_home`. Open the daily card's editor and select each Type to capture the four settings dialogs. No live panel or Home Assistant credentials are needed.

## Device mockup prompt

Use case: compositing
Asset type: Corrected Weather documentation photo.
Input images: IMAGE 1 is the edit target, the real device photo. IMAGE 2 is the exact flat screen artwork.
Change ONLY the foreground device's illuminated display pixels. Replace that display with IMAGE 2 using a single physically correct four-corner perspective projection (one planar homography for the entire screenshot). Determine the four actual LCD corners inside the black glass bezel of IMAGE 1 and project the square screenshot to those exact four corners. Preserve the black glass bezel's original width around every edge.
Critical geometry: The LCD is on the very same flat plane as the glass bezel. The screen's top and bottom edges must follow the respective top and bottom edges of the original LCD. Left and right edges must follow the original LCD side edges. Every card, icon, text baseline and gap must be projected together as part of the same screen texture, sharing the same two vanishing points as the device. Do not place a front-on UI, a floating rectangle, a differently tilted screen, or separately skewed cards inside the photo. Keep the exact relative positions, scale, square aspect before projection, and margins of image 2. The screen must not bend, bulge, fan out or have inconsistent row angles. Only perspective caused by the actual device angle is acceptable.
Keep the original photograph's enclosure, camera angle, desk, background screens, USB cable, grain, casual lighting and focus completely unchanged. Preserve natural screen reflections above the projected screenshot. A casual photograph with ordinary reflections, not a polished product rendering.
UI content must be precisely as IMAGE 2: top bar 17°, 15:31, wifi. Top row Cloudy; Today 18°/2°; Tomorrow 20°/9°. Lower full-width two-row card: Today 18°/2°, Tues 20°/9°, Weds 18°/14°, Thurs 20°/13°, Fri 14°/6°, Sat 18°/10°, Sun 19°/11°, Mon 17°/8°. Preserve all eight days, icons, labels and values. No extra text or watermarks.
