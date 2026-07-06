#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "entry.js");
const MODULES_DIR = path.join(ROOT, "src", "webserver", "modules");
const TYPES_DIR = path.join(ROOT, "src", "webserver", "types");
const WEB_MODULE_ORDER = require("./web_modules.json");

function moduleFileNames() {
  return fs.readdirSync(MODULES_DIR)
    .filter((name) => name.endsWith(".js"))
    .map((name) => path.basename(name, ".js"))
    .sort();
}

function assertWebModuleOrderCoversFiles() {
  const actual = moduleFileNames();
  const actualSet = new Set(actual);
  const orderSet = new Set(WEB_MODULE_ORDER);
  const duplicates = WEB_MODULE_ORDER
    .filter((name, index) => WEB_MODULE_ORDER.indexOf(name) !== index)
    .sort();
  const missing = actual.filter((name) => !orderSet.has(name));
  const unknown = WEB_MODULE_ORDER.filter((name) => !actualSet.has(name)).sort();
  const errors = [];
  if (duplicates.length) errors.push(`duplicate entries: ${Array.from(new Set(duplicates)).join(", ")}`);
  if (missing.length) errors.push(`missing modules: ${missing.join(", ")}`);
  if (unknown.length) errors.push(`unknown modules: ${unknown.join(", ")}`);
  if (errors.length) {
    throw new Error(`scripts/web_modules.json does not match src/webserver/modules: ${errors.join("; ")}`);
  }
}

function indentChunk(text) {
  return text.trimEnd().split(/\r?\n/).map((line) => {
    return line.trim() ? `  ${line}` : "";
  }).join("\n");
}

function replaceMarkedBlock(source, startTag, endTag, content, required) {
  const pattern = new RegExp(
    `(^[^\\n]*${startTag}[^\\n]*\\n)(.*?)(^[^\\n]*${endTag}[^\\n]*$)`,
    "ms"
  );
  const match = source.match(pattern);
  if (!match) {
    if (required) throw new Error(`Missing source markers: ${startTag} / ${endTag}`);
    return source;
  }
  return source.slice(0, match.index + match[1].length) +
    content +
    source.slice(match.index + match[1].length + match[2].length);
}

function loadButtonTypes() {
  if (!fs.existsSync(TYPES_DIR)) return "";
  return fs.readdirSync(TYPES_DIR)
    .filter((name) => name.endsWith(".js"))
    .sort()
    .map((name) => {
      const typePath = path.join(TYPES_DIR, name);
      return `  // --- type: ${path.basename(name, ".js")} ---\n${indentChunk(fs.readFileSync(typePath, "utf8"))}`;
    })
    .join("\n") + "\n";
}

function loadWebModules() {
  assertWebModuleOrderCoversFiles();
  return WEB_MODULE_ORDER.map((name) => {
    const modulePath = path.join(MODULES_DIR, `${name}.js`);
    if (!fs.existsSync(modulePath)) {
      throw new Error(`Missing web module: ${path.relative(ROOT, modulePath)}`);
    }
    return `  // --- module: ${name} ---\n${indentChunk(fs.readFileSync(modulePath, "utf8"))}`;
  }).join("\n") + "\n";
}

function loadBundledWebSource() {
  let source = fs.readFileSync(SOURCE, "utf8");
  source = replaceMarkedBlock(source, "__BUTTON_TYPES_START__", "__BUTTON_TYPES_END__", loadButtonTypes(), false);
  source = replaceMarkedBlock(source, "__WEB_MODULES_START__", "__WEB_MODULES_END__", loadWebModules(), true);
  return source;
}

module.exports = {
  loadBundledWebSource,
};
