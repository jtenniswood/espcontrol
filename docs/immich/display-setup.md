---
title: Connect Immich Photos to EspControl
description: Show Immich photos on your EspControl screensaver or Camera card using the companion's Home Assistant image entity and optional photo metadata.
---

# Connect to EspControl

Use the **EspControl Immich Companion** HACS integration to turn your Immich library into a photo screensaver. The companion selects and prepares photos, Home Assistant exposes the frame's **Image** entity, and EspControl displays it.

## Before you start

- [Install the companion](/immich/installation) and create a frame. Use the HACS integration for this setup; the optional add-on alone does not create Home Assistant entities.
- Confirm the frame's **Image** entity shows a photo in Home Assistant. On the frame's device page, open the entity and copy its actual `image.*` entity ID.
- Connect your EspControl panel to the same Home Assistant instance. The Camera screensaver supports ESP32-P4 panels and the **4-inch 4848S040 ESP32-S3** panel. Update the [panel firmware](/features/firmware-updates) if its Camera controls are missing.

## Match the image to your display

On the companion frame's Home Assistant device page, open **Configuration → Screen shape** and choose the shape that matches the panel's current [rotation](/features/rotation):

| Panel orientation | Companion screen shape | Image output |
|---|---|---|
| Landscape | Landscape | 1280 × 800 |
| Portrait | Portrait | 800 × 1280 |
| Square | Square | 720 × 720 |

These are the companion's output sizes; EspControl scales the image to its own screen. For example, the square output is resized for the 480 × 480 4848S040 panel.

Start with **Photo fit → Show full image** in the companion to keep the whole photo against a matching background. Choose **Crop to fit** if you prefer to fill the frame by trimming the edges. Photo orientation and [portrait pairing](/immich/using-your-frame#fit-the-screen-and-select-photos) control which photos appear independently of the screen shape.

## Use the photo screensaver

1. Open the EspControl panel's web setup page.
2. Go to **Settings → Sleep & Schedule → Screensaver**.
3. Select **Timer** and choose an inactivity delay, or select **Sensor** and enter your Home Assistant presence sensor.
4. Set **Then → Camera**. This option accepts both cameras and photo image entities.
5. Enter the companion frame's `image.*` entity ID in **Camera Entity**.
6. Under **Expanded Image**, choose **Show full image** (Fit) to preserve the complete companion image, or **Crop to fit** (Fill) to crop it to the panel. This is a second sizing step after the companion's own Photo fit setting.
7. Finish editing the entity field so the setting saves, then let the screensaver activate. Settings save automatically.

The companion's **Slideshow Timer** controls how often the photo changes, while the EspControl screensaver timer controls when the panel starts showing photos. The companion starts with a 30-second slideshow interval. EspControl refreshes the image when Home Assistant reports an update and retains the previous successful image while downloading the next one.

Touch the screen to return to the panel. See [Screensaver](/features/screensaver) for presence wake, brightness, the optional clock overlay and Night Schedule priority.

## Add photo details

With **Then → Camera** selected, enable **Display Metadata** and enter one of the companion's `sensor.*` entities in **Photo Metadata Entity**. For example, use the frame's **Date** or **Location** sensor. Copy the actual entity ID from Home Assistant; names depend on your frame.

The panel accepts one metadata sensor. The companion also provides People, Tags, Rating, Camera and Favourite sensors, which you can use in Home Assistant. For paired portraits, these sensors describe the **left photo**. Missing details are hidden on the panel. See [Photo metadata](/features/screensaver#photo-metadata) for positioning and clock-overlay behaviour.

## Use a Camera card

To keep photos alongside your room controls, add a [Camera card](/card-types/cameras) and select the same companion `image.*` entity. Tap the card to open its larger image view. The screensaver and card can use the same frame, or you can create independent companion frames for different photo selections.

## Playback and a first check

Open the frame's device page in Home Assistant to pause with **Slideshow**, advance with **Next**, or revisit a slide with **Previous**. Next resumes playback; Previous does not pause it. These controls also work in Home Assistant dashboards and automations. See [Using your frame](/immich/using-your-frame#playback-details-and-automations).

To check the setup, wait for a photo to appear on the panel, press **Next** in Home Assistant, and confirm the panel updates. If metadata is enabled, confirm it matches the displayed photo. Check that tapping the screensaver wakes the panel.

If no photo appears, check the Image entity in Home Assistant first and follow [companion troubleshooting](/immich/installation#troubleshooting). If it works there, recheck the panel's Camera Entity, firmware support and Home Assistant connection. If photos look soft, review [image quality and original-download permissions](/immich/installation#image-quality).
