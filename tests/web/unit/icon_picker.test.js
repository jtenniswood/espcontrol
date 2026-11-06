"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("icon picker shows the current custom icon and prefers built-in name matches", () => {
  loadTypescriptTest("tests/web/icon_picker.test.ts").runIconPickerTests();
});
