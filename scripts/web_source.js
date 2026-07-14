#!/usr/bin/env node
"use strict";

const fs = require("fs");
const os = require("os");
const path = require("path");
const childProcess = require("child_process");

const ROOT = path.resolve(__dirname, "..");
const outputDirectories = new Map();

function freshWebOutputDir(options = {}) {
  const testHooks = options.testHooks !== false;
  const key = testHooks ? "test" : "production";
  if (outputDirectories.has(key)) return outputDirectories.get(key);
  const outputDir = fs.mkdtempSync(path.join(os.tmpdir(), `espcontrol-web-${key}-`));
  const args = [path.join(ROOT, "scripts", "build.py"), "www", "--temporary-output", outputDir];
  if (testHooks) args.push("--test-hooks");
  const result = childProcess.spawnSync("python3", args, { cwd: ROOT, encoding: "utf8" });
  if (result.status !== 0) {
    fs.rmSync(outputDir, { recursive: true, force: true });
    throw new Error(result.stderr || result.stdout || "Fresh web bundle build failed");
  }
  outputDirectories.set(key, outputDir);
  process.once("exit", () => fs.rmSync(outputDir, { recursive: true, force: true }));
  return outputDir;
}

function loadBuiltWebSource(slug = "guition-esp32-p4-jc1060p470") {
  return fs.readFileSync(path.join(freshWebOutputDir(), slug, "www.js"), "utf8");
}

module.exports = { freshWebOutputDir, loadBuiltWebSource };
