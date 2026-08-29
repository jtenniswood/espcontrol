"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("sensor card mode controller", () => {
  const { runSensorCardModeControllerTests } = loadTypescriptTest("tests/web/sensor_card_mode_controller.test.ts");
  runSensorCardModeControllerTests();
});
