#!/usr/bin/env node
"use strict";

const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

const testModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "device_api.test.ts"));
testModule.runDeviceApiTests().then(() => {
  console.log("Typed device API transport tests passed.");
}).catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
