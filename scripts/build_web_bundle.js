#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const esbuild = require("esbuild");

const ROOT = path.resolve(__dirname, "..");
const ENTRY = path.join(ROOT, "src", "webserver", "entry.ts");

async function bundleDevice(slug, config, testHooks) {
  const result = await esbuild.build({
    bundle: true,
    define: {
      __ESPCONTROL_DEVICE_ID__: JSON.stringify(slug),
      __ESPCONTROL_DEVICE_CONFIG__: JSON.stringify(config),
      __ESPCONTROL_TEST_HOOKS_ENABLED__: testHooks ? "true" : "false",
    },
    entryPoints: [ENTRY],
    format: "iife",
    logLevel: "silent",
    minify: true,
    platform: "browser",
    target: "es2020",
    write: false,
  });
  if (result.outputFiles.length !== 1) throw new Error(`${slug}: esbuild returned an unexpected output set`);
  return result.outputFiles[0].text;
}

async function main() {
  const request = JSON.parse(fs.readFileSync(0, "utf8"));
  if (!request.outputDir || !request.devices) throw new Error("Expected outputDir and devices");
  for (const [slug, config] of Object.entries(request.devices)) {
    const outputPath = path.join(request.outputDir, slug, "www.js");
    fs.mkdirSync(path.dirname(outputPath), { recursive: true });
    fs.writeFileSync(outputPath, await bundleDevice(slug, config, !!request.testHooks));
  }
}

main().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
