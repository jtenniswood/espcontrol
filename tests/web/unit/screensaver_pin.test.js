"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("device-local screensaver PIN setup and backup clearing", async () => {
  await loadTypescriptTest("tests/web/screensaver_pin.test.ts").runScreensaverPinTests();
});
