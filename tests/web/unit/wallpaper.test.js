"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
test("wallpaper resize, RGB565 wire format and transparent card fills", () => {
  loadTypescriptTest("tests/web/wallpaper.test.ts").runWallpaperTests();
});
