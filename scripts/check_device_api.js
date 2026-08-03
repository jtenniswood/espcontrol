#!/usr/bin/env node
"use strict";

const path = require("path");
const fs = require("fs");
const { loadTypeScriptModule } = require("./load_typescript_module");

const testModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "device_api.test.ts"));
testModule.runDeviceApiTests().then(() => {
  const transactionSource = fs.readFileSync(
    path.resolve(__dirname, "..", "src", "webserver", "application", "configuration_transaction.ts"),
    "utf8",
  );
  if (!transactionSource.includes("return retryConfigurationPost(domain, name, objectIds, value, fallback, error);")) {
    throw new Error("transient initial snapshot failures must retain the requested edit for retry");
  }
  if (transactionSource.includes("rejectWaiters(error)")) {
    throw new Error("a transient initial snapshot failure must not reject already queued edits");
  }
  console.log("Typed device API transport tests passed.");
}).catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
