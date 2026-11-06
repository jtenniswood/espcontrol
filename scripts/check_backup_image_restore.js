#!/usr/bin/env node
"use strict";

const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

async function main() {
  const tests = loadTypeScriptModule(path.resolve(
    __dirname, "..", "tests", "web", "card_images_feature.test.ts",
  ));
  await tests.runCardImagesFeatureTests();
  console.log("Backup image restore checks passed.");
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
