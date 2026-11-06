#!/usr/bin/env node
"use strict";

const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

async function main() {
  const testModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "preview_grid.test.ts"));
  testModule.runPreviewGridTests();
  const previewTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "preview_feature.test.ts"));
  previewTestModule.runPreviewFeatureTests();
  const backupTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "backup_feature.test.ts"));
  backupTestModule.runBackupFeatureTests();
  const cardImagesTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "card_images_feature.test.ts"));
  await cardImagesTestModule.runCardImagesFeatureTests();
  const settingsTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "settings_feature.test.ts"));
  settingsTestModule.runSettingsFeatureTests();
  const clipboardTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "clipboard_feature.test.ts"));
  clipboardTestModule.runClipboardFeatureTests();
  console.log("Typed settings, backup, card-image, and preview feature tests passed.");
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
