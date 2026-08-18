"use strict";

const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("debounces and defers card-editor rendering", () => {
  const { runButtonSettingsRenderQueueTests } = loadTypescriptTest("tests/web/button_settings_render_queue.test.ts");
  runButtonSettingsRenderQueueTests();
});
