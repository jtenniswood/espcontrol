# Weather documentation images

The admin screenshots were captured from the actual ESP32-P4-86 web editor on this branch, using a local simulated device and `weather.forecast_home`. They contain sample configuration, not a connection to a user's panel. The four screenshots show the four values in **Type**.

The device image is an AI-generated illustration, made with the built-in image generation tool. The supplied ESP32-P4-86 photo guides the enclosure and tabletop setting; a capture of the actual editor preview guides the screen. It is not evidence of hardware testing. Weather values are illustrative.

## Saved assets

- `docs/public/images/weather-device-mockup.webp` — generated device illustration, compressed to WebP without changing its content.
- `docs/public/images/weather-admin-current.png` — Current Conditions editor.
- `docs/public/images/weather-admin-today.png` — Today's Forecast editor.
- `docs/public/images/weather-admin-tomorrow.png` — Tomorrow's Forecast editor.
- `docs/public/images/weather-admin-daily.png` — Daily Forecast editor.
- `assets/documentation/weather-screen-reference.png` — actual preview screenshot used as the screen reference for generation.

The screenshots were captured with Playwright using the existing browser smoke harness's local routes, simulated EventSource, and native configuration loader. The preview uses `esp32-p4-86`, four weather cards with modes `""`, `today`, `tomorrow`, `daily`, and grid order `1,2,3,4h`. Each card uses `weather.forecast_home`. Open the daily card's editor and select each Type to capture the four settings dialogs. No live panel or Home Assistant credentials are needed.

## Device mockup prompt

Use case: product-mockup
Asset type: Weather documentation hero image.
Primary request: Create a polished realistic photograph of a small square EspControl ESP32-P4-86 tabletop touch panel, using image 1 as the hardware reference and image 2 as the EXACT screen artwork reference.
Input images: Image 1 is the supplied real device photo, a reference for the white narrow enclosure, broad black glass bezel, thickness, stand angle and side USB cable. Image 2 is the real software preview and must be reproduced faithfully inside the display, including all text, weather icons, cards and typography.
Scene/backdrop: Single device on a warm light oak tabletop with a softly blurred neutral home background. No other screens, plants, logos or props. Soft daylight and restrained glass reflections that do not obscure the screen.
Composition/framing: Landscape 3:2 product photograph, centered device occupying most of the image, slight three-quarter perspective but nearly front-on for legibility, entire square device visible, subtle grounded shadow. Hardware has a square display and square enclosure like image 1, not a landscape tablet.
Screen content: Match image 2 exactly: black screen background, dark charcoal generously rounded cards, white simple weather icons and labels, grey temperatures. Clock bar 17°, 15:31, wifi. Top row has three equal cards Cloudy, Today 18°/2°, Tomorrow 20°/9°. Bottom card spans the full width and two rows with eight days: Today 18°/2°, Tues 20°/9°, Weds 18°/14°, Thurs 20°/13°, Fri 14°/6°, Sat 18°/10°, Sun 19°/11°, Mon 17°/8°. Preserve the actual icon shapes and alignment from image 2.
Constraints: No blue cards, no radar map, no invented controls, no extra screen labels, no captions or writing outside the screen, no watermark. This is an illustrative device mockup showing sample weather, not a real hardware capture.
