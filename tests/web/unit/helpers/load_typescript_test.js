"use strict";

const path = require("node:path");
const { loadTypeScriptModule } = require("../../../../scripts/load_typescript_module");

const ROOT = path.resolve(__dirname, "../../../..");
const cache = new Map();

function loadTypescriptTest(relativePath) {
  const entry = path.resolve(ROOT, relativePath);
  if (cache.has(entry)) return cache.get(entry);

  const loaded = loadTypeScriptModule(entry);
  cache.set(entry, loaded);
  return loaded;
}

module.exports = { loadTypescriptTest };
