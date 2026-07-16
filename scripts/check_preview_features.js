#!/usr/bin/env node
"use strict";

const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

const testModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "preview_grid.test.ts"));
testModule.runPreviewGridTests();
const previewTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "preview_feature.test.ts"));
previewTestModule.runPreviewFeatureTests();
const backupTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "backup_feature.test.ts"));
backupTestModule.runBackupFeatureTests();
const settingsTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "settings_feature.test.ts"));
settingsTestModule.runSettingsFeatureTests();
const clipboardTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "clipboard_feature.test.ts"));
clipboardTestModule.runClipboardFeatureTests();
const haDirectoryTestModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "ha_directory_feature.test.ts"));
haDirectoryTestModule.runHaDirectoryFeatureTests();
console.log("Typed settings, backup, and preview feature tests passed.");
