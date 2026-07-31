"use strict";

const fs = require("node:fs");
const path = require("node:path");
const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const ROOT = path.resolve(__dirname, "../../..");
const fixtures = JSON.parse(fs.readFileSync(
  path.join(ROOT, "compatibility", "fixtures", "product_compatibility.json"),
  "utf8",
));

describe("saved-configuration migrations", () => {
  const { runMigrationTests } = loadTypescriptTest("tests/web/migrations.test.ts");

  test("applies generated aliases to shared compatibility fixtures", () => {
    runMigrationTests(fixtures["legacy-v1"].oldButtonStrings);
  });
});
