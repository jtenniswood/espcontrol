"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("camera name survives hidden labels and stays a primary editor field", () => {
  loadTypescriptTest("tests/web/camera_name.test.ts").runCameraNameTests();
});
