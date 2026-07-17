"use strict";

const fs = require("node:fs");
const path = require("node:path");
const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const ROOT = path.resolve(__dirname, "../../..");
const manifest = JSON.parse(fs.readFileSync(path.join(ROOT, "devices", "manifest.json"), "utf8"));
const fixtures = JSON.parse(fs.readFileSync(
  path.join(ROOT, "compatibility", "fixtures", "product_compatibility.json"),
  "utf8",
));

describe("device profiles", () => {
  const { runDeviceProfileTests } = loadTypescriptTest("tests/web/device_profiles.test.ts");

  test("initializes every manifest-driven web layout", () => {
    runDeviceProfileTests(manifest, fixtures.current.deviceProfiles);
  });
});
