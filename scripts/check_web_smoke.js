#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "www.js");

function loadHooks() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: {
      readyState: "loading",
      activeElement: null,
      addEventListener() {},
    },
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(fs.readFileSync(SOURCE, "utf8"), sandbox, { filename: SOURCE });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

const hooks = loadHooks();
assert(hooks, "web test hooks were not exported");

const button = {
  entity: "light.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Lightbulb",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
};

const encoded = hooks.serializeButtonConfig(button);
assert.strictEqual(encoded, "light.kitchen;Kitchen;Auto;Lightbulb");
assert.deepStrictEqual(plain(hooks.parseButtonConfig(encoded)), button);

assert.strictEqual(hooks.normalizeTemperatureUnit("fahrenheit"), "\u00b0F");
assert.strictEqual(hooks.normalizeTemperatureUnit("centigrade"), "\u00b0C");
assert.strictEqual(hooks.previewHtmlValue({ labelHtml: "" }, "labelHtml", "fallback"), "");
assert.strictEqual(hooks.previewHtmlValue({}, "labelHtml", "fallback"), "fallback");

console.log("Web UI smoke tests passed.");
