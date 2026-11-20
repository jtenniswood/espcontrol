"use strict";

const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

test("Power contract, typed configuration, native backup and card transfer remain compatible", () => {
  loadTypescriptTest("tests/web/power_card.test.ts").runPowerCardTests();
});
