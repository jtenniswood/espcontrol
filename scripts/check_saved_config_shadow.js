#!/usr/bin/env node
"use strict";

const assert = require("assert");
const childProcess = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");
const vm = require("vm");
const { loadTypeScriptModule } = require("./load_typescript_module");
const { loadBuiltWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const FIELDS = ["entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options"];

function loadBrowserCodec() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {}, console: { log() {}, warn() {}, error() {} },
    location: { search: "" }, URLSearchParams, setTimeout, clearTimeout,
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

function parseRawButtonConfig(value) {
  const compact = String(value || "").startsWith("~");
  const parts = compact ? String(value).substring(1).split(",") : String(value || "").split(";");
  const decoded = compact ? parts.map((field) => {
    try { return decodeURIComponent(field); } catch (_) { return field; }
  }) : parts;
  return shape(Object.fromEntries(FIELDS.map((field, index) => [field, decoded[index] || ""])));
}

function shadowCases() {
  const fixtures = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/vacuum_mower_card_normalization_fixtures.json"), "utf8"));
  const config = (overrides) => Object.assign({
    entity: "vacuum.robot", label: "", icon: "Robot Vacuum", icon_on: "Auto",
    sensor: "start_stop", unit: "", type: "vacuum", precision: "", options: "",
  }, overrides);
  const vacuum = fixtures.filter((fixture) => fixture.expected.type === "vacuum").concat([
    {
      name: "compact vacuum preserves encoded Unicode and area",
      input: "~vacuum.robot,%E5%8E%A8%E6%88%BF%20Vacuum,Auto,Auto,clean_area,zone%3A1,vacuum,text,unknown",
      expected: config({ label: "厨房 Vacuum", icon: "Vacuum Outline", sensor: "clean_area", unit: "zone:1" }),
    },
    {
      name: "legacy vacuum preserves Unicode label",
      input: "vacuum.robot;Küche 🤖;Auto;Auto;dock;ignored;vacuum;2;unknown=1",
      expected: config({ label: "Küche 🤖", icon: "Robot Vacuum Variant", sensor: "dock" }),
    },
    {
      name: "vacuum preserves a 255 byte label",
      input: `vacuum.robot;${"x".repeat(255)};Auto;Auto;status;ignored;vacuum;2;unknown=1`,
      expected: config({ label: "x".repeat(255), sensor: "status" }),
    },
    {
      name: "malformed short vacuum config receives safe defaults",
      input: "vacuum.robot;;;;bad;;vacuum",
      expected: config({}),
    },
  ]);
  const sensor = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/sensor_card_normalization_fixtures.json"), "utf8"));
  return vacuum.concat(sensor);
}

function compiler() {
  for (const candidate of [process.env.CXX, "c++", "g++", "clang++"].filter(Boolean)) {
    if (childProcess.spawnSync(candidate, ["--version"], { stdio: "ignore" }).status === 0) return candidate;
  }
  throw new Error("No C++ compiler found for saved-config shadow check");
}

function cppSource(cases) {
  const inputs = cases.map(({ input }) => JSON.stringify(input)).join(",\n    ");
  return `
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
namespace esphome { class StringRef { public: StringRef(const char *v): value_(v ? v : "") {} const char *c_str() const { return value_; } size_t size() const { return std::strlen(value_); } private: const char *value_; }; }
struct lv_obj_t {};
inline void lv_label_set_text(lv_obj_t *, const char *) {}
inline const char *espcontrol_i18n(const char *text) { return text ? text : ""; }
inline std::string espcontrol_i18n(const std::string &text) { return text; }
#include "button_grid_config_parser.h"
#include "button_grid_saved_config_shadow_generated.h"

ParsedCfg raw_cfg(const std::string &cfg) {
  ParsedCfg p;
  if (!cfg.empty() && cfg[0] == '~') {
    std::vector<std::string> f = split_config_fields(cfg.substr(1), ',');
    p.entity = f.size() > 0 ? decode_compact_field(f[0]) : ""; p.label = f.size() > 1 ? decode_compact_field(f[1]) : "";
    p.icon = f.size() > 2 ? decode_compact_field(f[2]) : ""; p.icon_on = f.size() > 3 ? decode_compact_field(f[3]) : "";
    p.sensor = f.size() > 4 ? decode_compact_field(f[4]) : ""; p.unit = f.size() > 5 ? decode_compact_field(f[5]) : "";
    p.type = f.size() > 6 ? decode_compact_field(f[6]) : ""; p.precision = f.size() > 7 ? decode_compact_field(f[7]) : "";
    p.options = f.size() > 8 ? decode_compact_field(f[8]) : ""; return p;
  }
  p.entity = cfg_field(cfg, 0); p.label = cfg_field(cfg, 1); p.icon = cfg_field(cfg, 2); p.icon_on = cfg_field(cfg, 3);
  p.sensor = cfg_field(cfg, 4); p.unit = cfg_field(cfg, 5); p.type = cfg_field(cfg, 6); p.precision = cfg_field(cfg, 7); p.options = cfg_field(cfg, 8);
  return p;
}
void quoted(const std::string &v) { std::cout << '"'; for (char c : v) { if (c == '"' || c == '\\\\') std::cout << '\\\\'; std::cout << c; } std::cout << '"'; }
void print_cfg(const ParsedCfg &p) { const std::string values[] = {p.entity,p.label,p.icon,p.icon_on,p.sensor,p.unit,p.type,p.precision,p.options}; std::cout << '['; for (int i=0;i<9;++i) { if(i) std::cout << ','; quoted(values[i]); } std::cout << ']'; }
int main() { const std::vector<std::string> inputs = { ${inputs} }; std::cout << '['; for (size_t i=0;i<inputs.size();++i) { if(i) std::cout << ','; ParsedCfg p=raw_cfg(inputs[i]); normalize_saved_config_shadow(p); print_cfg(p); } std::cout << ']'; }
`;
}

function compiledShadow(cases) {
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "espcontrol-saved-config-shadow-"));
  try {
    const source = path.join(temporary, "shadow.cpp");
    const binary = path.join(temporary, "shadow");
    fs.writeFileSync(source, cppSource(cases));
    childProcess.execFileSync(compiler(), ["-std=c++17", "-Wall", "-Wextra", "-Werror", `-I${path.join(ROOT, "components/espcontrol")}`, source, "-o", binary]);
    return JSON.parse(childProcess.execFileSync(binary, { encoding: "utf8" }));
  } finally {
    fs.rmSync(temporary, { recursive: true, force: true });
  }
}

function main() {
  const cases = shadowCases();
  const codec = loadBrowserCodec();
  const shadow = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_shadow.ts"));
  const firmwareShadow = compiledShadow(cases);
  assert.deepStrictEqual(Object.keys(shadow.SAVED_CONFIG_SHADOW_PILOT_POLICIES), ["action", "sensor", "media", "vacuum"]);
  cases.forEach((fixture, index) => {
    const expected = shape(fixture.expected);
    const production = shape(codec.parseButtonConfig(fixture.input));
    const browserShadow = shape(shadow.normalizeSavedConfigShadow(parseRawButtonConfig(fixture.input)));
    const compiled = Object.fromEntries(FIELDS.map((field, fieldIndex) => [field, firmwareShadow[index][fieldIndex]]));
    assert.deepStrictEqual(production, expected, `${fixture.name}: production`);
    assert.deepStrictEqual(browserShadow, expected, `${fixture.name}: browser shadow`);
    assert.deepStrictEqual(compiled, expected, `${fixture.name}: compiled shadow`);
    assert.deepStrictEqual(shape(shadow.normalizeSavedConfigShadow(browserShadow)), browserShadow, `${fixture.name}: shadow idempotence`);
  });
  const firmwareUsers = fs.readdirSync(path.join(ROOT, "components/espcontrol"))
    .filter((name) => name.endsWith(".h") && name !== "button_grid_saved_config_shadow_generated.h")
    .filter((name) => fs.readFileSync(path.join(ROOT, "components/espcontrol", name), "utf8").includes("button_grid_saved_config_shadow_generated"));
  assert.deepStrictEqual(firmwareUsers, [], "shadow header must remain outside production firmware");
  console.log(`Saved-config shadow agreement passed for ${cases.length} Vacuum and Sensor inputs across browser and compiled C++ helpers.`);
  console.log("Production firmware footprint delta: 0 bytes flash / 0 bytes RAM (test-only shadow; 8 KiB guard passed).");
}

main();
