"use strict";

const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

describe("browserless application contracts", () => {
  const { runClipboardFeatureTests } = loadTypescriptTest("tests/web/clipboard_feature.test.ts");
  const { runDeviceApiTests } = loadTypescriptTest("tests/web/device_api.test.ts");
  const { runSettingsFeatureTests } = loadTypescriptTest("tests/web/settings_feature.test.ts");
  const { runStateContractTests } = loadTypescriptTest("tests/web/state_contract.test.ts");

  test("plans clipboard transfers", () => {
    runClipboardFeatureTests();
  });

  test("preserves settings normalization", () => {
    runSettingsFeatureTests();
  });

  test("preserves state and event aliases", () => {
    runStateContractTests();
  });

  test("preserves request fallback and ordering", async () => {
    await runDeviceApiTests();
  });
});
