"use strict";

const Module = require("module");
const path = require("path");
const esbuild = require("esbuild");

function loadTypeScriptModule(entryPath) {
  const result = esbuild.buildSync({
    absWorkingDir: path.resolve(__dirname, ".."),
    bundle: true,
    entryPoints: [entryPath],
    format: "cjs",
    logLevel: "silent",
    platform: "node",
    target: "node20",
    write: false,
  });
  if (result.outputFiles.length !== 1) throw new Error(`Unexpected output while loading ${entryPath}`);
  const loaded = new Module(entryPath, module);
  loaded.filename = entryPath;
  loaded.paths = Module._nodeModulePaths(path.dirname(entryPath));
  loaded._compile(result.outputFiles[0].text, entryPath);
  return loaded.exports;
}

module.exports = { loadTypeScriptModule };
