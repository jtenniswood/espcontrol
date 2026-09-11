const test = require("node:test");
const fs = require("node:fs");
const path = require("node:path");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const ROOT = path.resolve(__dirname, "../../..");
const migrationFixture = JSON.parse(fs.readFileSync(
  path.join(ROOT, "compatibility", "fixtures", "panel_config_migration_v1.json"),
  "utf8",
));

test("native PanelConfig migration client", async () => {
  const { runNativePanelConfigTests } = loadTypescriptTest("tests/web/native_panel_config.test.ts");
  await runNativePanelConfigTests(migrationFixture);
});
