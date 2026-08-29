"use strict";

const { describe, test } = require("node:test");
const fs = require("node:fs");
const path = require("node:path");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const { runPanelConfigTests } = loadTypescriptTest("tests/web/panel_config.test.ts");
const fixture = JSON.parse(fs.readFileSync(
  path.resolve(__dirname, "../../../compatibility/fixtures/panel_config_v1.json"),
  "utf8",
));

describe("PanelConfig document", () => {
  test("encodes and validates the shared v1 fixture", () => {
    runPanelConfigTests(fixture);
  });
});
