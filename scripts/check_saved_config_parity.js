#!/usr/bin/env node
"use strict";

const assert = require("assert");
const childProcess = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");
const vm = require("vm");
const { loadBuiltWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const CONFIG_DIR = path.join(ROOT, "common", "config");
const FIELDS = ["entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options"];

function loadBrowserCodec() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    location: { search: "" },
    URLSearchParams,
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: { readyState: "loading", activeElement: null, addEventListener() {} },
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: "src/webserver/entry.ts" });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

function shape(value) {
  return Object.fromEntries(FIELDS.map((field) => [field, value[field] || (field === "icon" || field === "icon_on" ? "Auto" : "")]));
}

function loadCases() {
  const cases = [];
  const fixtureFiles = [
    "card_normalization_fixtures.json",
    ...fs.readdirSync(CONFIG_DIR).sort().filter((fileName) => fileName.endsWith("_card_normalization_fixtures.json")),
  ];
  for (const fileName of fixtureFiles) {
    const document = JSON.parse(fs.readFileSync(path.join(CONFIG_DIR, fileName), "utf8"));
    const groups = Array.isArray(document) ? [[fileName.replace(/_card_normalization_fixtures\.json$/, ""), document]] : Object.entries(document);
    for (const [group, fixtures] of groups) {
      for (const fixture of fixtures) {
        cases.push({ name: `${group}: ${fixture.name}`, encoded: fixture.input, expected: shape(fixture.expected) });
        if (Object.prototype.hasOwnProperty.call(fixture, "canonical")) {
          cases.push({ name: `${group}: ${fixture.name} (canonical)`, encoded: fixture.canonical, expected: shape(fixture.expected) });
        }
      }
    }
  }
  return cases;
}

function compiler() {
  for (const candidate of [process.env.CXX, "c++", "g++", "clang++"].filter(Boolean)) {
    const result = childProcess.spawnSync(candidate, ["--version"], { stdio: "ignore" });
    if (result.status === 0) return candidate;
  }
  throw new Error("No C++ compiler found for saved-configuration parity check");
}

function cppSource(cases) {
  const inputs = cases.map(({ encoded }) => JSON.stringify(encoded)).join(",\n    ");
  return `
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "esphome/core/string_ref.h"
struct lv_obj_t {};
inline void lv_label_set_text(lv_obj_t *, const char *) {}
inline const char *espcontrol_i18n(const char *text) { return text ? text : ""; }
inline std::string espcontrol_i18n(const std::string &text) { return text; }
#include "button_grid_config_parser.h"

void json_string(const std::string &value) {
  std::cout << '"';
  for (unsigned char ch : value) {
    switch (ch) {
      case '"': std::cout << static_cast<char>(92) << '"'; break;
      case '\\\\': std::cout << static_cast<char>(92) << static_cast<char>(92); break;
      case '\\b': std::cout << "\\\\b"; break;
      case '\\f': std::cout << "\\\\f"; break;
      case '\\n': std::cout << "\\\\n"; break;
      case '\\r': std::cout << "\\\\r"; break;
      case '\\t': std::cout << "\\\\t"; break;
      default:
        if (ch < 0x20) {
          char escaped[7];
          std::snprintf(escaped, sizeof(escaped), "\\\\u%04x", ch);
          std::cout << escaped;
        } else {
          std::cout << static_cast<char>(ch);
        }
    }
  }
  std::cout << '"';
}

void print_field(const char *name, const std::string &value, bool first = false) {
  if (!first) std::cout << ',';
  json_string(name);
  std::cout << ':';
  json_string(value);
}

int main() {
  const std::vector<std::string> inputs = {
    ${inputs}
  };
  std::cout << '[';
  for (size_t index = 0; index < inputs.size(); ++index) {
    if (index) std::cout << ',';
    const ParsedCfg parsed = parse_cfg(inputs[index]);
    std::cout << '{';
    print_field("entity", parsed.entity, true);
    print_field("label", parsed.label);
    print_field("icon", parsed.icon);
    print_field("icon_on", parsed.icon_on);
    print_field("sensor", parsed.sensor);
    print_field("unit", parsed.unit);
    print_field("type", parsed.type);
    print_field("precision", parsed.precision);
    print_field("options", parsed.options);
    std::cout << '}';
  }
  std::cout << ']';
  return 0;
}
`;
}

function compiledFirmwareResults(cases) {
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "espcontrol-config-parity-"));
  try {
    const source = path.join(temporary, "saved_config_parity.cpp");
    const binary = path.join(temporary, "saved_config_parity");
    fs.writeFileSync(source, cppSource(cases));
    childProcess.execFileSync(compiler(), [
      "-std=c++17", "-Wall", "-Wextra", "-Werror",
      `-I${path.join(ROOT, "components", "espcontrol")}`,
      `-I${path.join(ROOT, "tests", "firmware", "stubs")}`,
      source, "-o", binary,
    ], { stdio: "pipe" });
    return JSON.parse(childProcess.execFileSync(binary, { encoding: "utf8" }));
  } finally {
    fs.rmSync(temporary, { recursive: true, force: true });
  }
}

function main() {
  const cases = loadCases();
  const hooks = loadBrowserCodec();
  assert(hooks && hooks.parseButtonConfig && hooks.serializeButtonConfig, "browser config codec hooks are unavailable");
  const firmware = compiledFirmwareResults(cases);
  assert.strictEqual(firmware.length, cases.length, "firmware result count");
  cases.forEach((fixture, index) => {
    const browser = shape(hooks.parseButtonConfig(fixture.encoded));
    assert.deepStrictEqual(browser, fixture.expected, `${fixture.name}: browser expectation`);
    assert.deepStrictEqual(firmware[index], fixture.expected, `${fixture.name}: firmware expectation`);
    assert.deepStrictEqual(browser, firmware[index], `${fixture.name}: browser/firmware parity`);
    const canonical = hooks.serializeButtonConfig(browser);
    assert.deepStrictEqual(shape(hooks.parseButtonConfig(canonical)), browser, `${fixture.name}: browser idempotence`);
  });
  console.log(`Saved-configuration parity passed for ${cases.length} browser/firmware inputs.`);
}

main();
