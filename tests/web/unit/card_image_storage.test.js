"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { createCardImageService } = loadTypescriptTest("src/webserver/application/card_image_service.ts");

test("unavailable storage cannot clear card references during image deletion", async () => {
  for (const requiresUsbFlash of [true, false]) {
    const requests = [];
    let cleared = false;
    const service = createCardImageService({
      layout: { config: {}, numSlots: 9 },
      requestApi: {
        postQueueIdle: async () => {}, resetPostQueueError() {}, postQueueHadError: () => false,
      },
      persistence: { clearCardImageReferences() { cleared = true; } },
      async fetch(url, request) {
        requests.push([url, request?.method || "GET"]);
        return new Response(JSON.stringify({ available: false, requires_usb_flash: requiresUsbFlash, images: [] }));
      },
      renderPreview() {}, renderButtonSettings() {},
    });
    await assert.rejects(service.deleteSafely("saved-image"), requiresUsbFlash
      ? /USB reinstall.*Export a backup.*image-storage/
      : /Could not access card image storage/);
    assert.equal(cleared, false, "existing references survive a blocked delete");
    assert.deepEqual(requests, [["/api/card-images", "GET"]], "no mutation is sent");
  }
});
