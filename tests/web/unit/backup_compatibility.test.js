"use strict";

const fs = require("node:fs");
const path = require("node:path");
const { describe, test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const ROOT = path.resolve(__dirname, "../../..");
const fixtures = JSON.parse(fs.readFileSync(
  path.join(ROOT, "product", "v2", "product_compatibility.json"),
  "utf8",
));
const migrationFixture = JSON.parse(fs.readFileSync(
  path.join(ROOT, "compatibility", "fixtures", "panel_config_migration_v1.json"),
  "utf8",
));

describe("backup compatibility", () => {
  const { runBackupFeatureTests } = loadTypescriptTest("tests/web/backup_feature.test.ts");
  const { runBackupCompatibilityTests } = loadTypescriptTest("tests/web/backup_compatibility.test.ts");

  test("creates and imports current backups", () => {
    runBackupFeatureTests(migrationFixture);
  });

  test("accepts the shared legacy backup fixture", () => {
    runBackupCompatibilityTests(fixtures["legacy-v1"].backup);
  });
});
