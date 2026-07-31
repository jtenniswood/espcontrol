#!/usr/bin/env node
"use strict";

const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

const testModule = loadTypeScriptModule(path.resolve(__dirname, "..", "tests", "web", "state_contract.test.ts"));
testModule.runStateContractTests();
console.log("Typed application state and event sequence tests passed.");
