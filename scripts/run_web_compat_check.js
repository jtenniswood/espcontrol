#!/usr/bin/env node
"use strict";

const childProcess = require("node:child_process");
const path = require("node:path");

const ROOT = path.resolve(__dirname, "..");
const CHECKS = {
  config: {
    legacy: "check_config_formats.js",
    suites: ["encoding", "migrations", "card_contracts"],
  },
  "web-smoke": {
    legacy: "check_web_smoke.js",
    suites: ["backup_compatibility", "preview_logic", "device_profiles", "application_contracts"],
  },
};

const name = process.argv[2];
const check = CHECKS[name];
if (!check) {
  console.error(`Unknown compatibility check: ${name || "(missing)"}`);
  process.exit(2);
}

const suitePaths = check.suites.map((suite) =>
  path.join(ROOT, "tests", "web", "unit", `${suite}.test.js`));
const result = childProcess.spawnSync(process.execPath, ["--test", ...suitePaths], {
  cwd: ROOT,
  stdio: "inherit",
});
if (result.error) throw result.error;
if (result.status !== 0) process.exit(result.status || 1);

require(path.join(__dirname, check.legacy));
