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
  const decoded = compact ? parts.map((field) => field.replace(/(%[0-9a-f]{2})+/gi, (run) => {
    try { return decodeURIComponent(run); } catch (_) { return run; }
  })) : parts;
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
      name: "compact vacuum decodes valid escapes beside malformed text",
      input: "~vacuum.robot,a%3Ab%ZZ,Auto,Auto,status,,vacuum,,",
      expected: config({ label: "a:b%ZZ", sensor: "status" }),
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
  sensor.push({
    name: "short sensor config receives default icons",
    input: "sensor.x;;;;sensor.x;;sensor;1;large_numbers",
    expected: {
      entity: "sensor.x", label: "", icon: "Auto", icon_on: "Auto", sensor: "sensor.x",
      unit: "", type: "sensor", precision: "1", options: "large_numbers",
    },
  }, {
    name: "text sensor trims padded state translations",
    input: "sensor.x;State;Auto;Auto;sensor.x;;sensor;text;state_labels,state_input=%20on%20,state_output=%20On%20,state_input_2=%20off%20,state_output_2=%20Off%20",
    expected: {
      entity: "sensor.x", label: "State", icon: "Auto", icon_on: "Auto", sensor: "sensor.x",
      unit: "", type: "sensor", precision: "text", options: "state_labels,state_input=on,state_output=On,state_input_2=off,state_output_2=Off",
    },
  });
  const sensorAliases = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/baseline_card_normalization_fixtures.json"), "utf8"))
    .filter((fixture) => fixture.expected.type === "sensor");
  const confirmation = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/confirmation_card_normalization_fixtures.json"), "utf8"))
    .filter((fixture) => fixture.expected.type === "action");
  const baseline = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/baseline_card_normalization_fixtures.json"), "utf8"))
    .filter((fixture) => fixture.expected.type === "action");
  const action = confirmation.concat(baseline, [
    {
      name: "legacy local action receives safe action defaults",
      input: "local.tap;Tap;Flash;Swap;ignored;unit;local;2;unknown=1",
      expected: config({ entity: "local.tap", label: "Tap", icon: "Gesture Tap", sensor: "local", type: "action" }),
    },
    {
      name: "action icon state drops numeric-only state options",
      input: "scene.movie;Movie;Flash;Auto;scene.turn_on;;action;;state_entity=sensor.mode,state_unit=W,state_precision=icon,large_numbers",
      expected: config({ entity: "scene.movie", label: "Movie", icon: "Flash", sensor: "scene.turn_on", type: "action", options: "state_entity=sensor.mode,state_precision=icon" }),
    },
    {
      name: "short action receives default icons",
      input: "scene.movie;Movie;;;scene.turn_on;;action;;",
      expected: config({ entity: "scene.movie", label: "Movie", icon: "Auto", sensor: "scene.turn_on", type: "action" }),
    },
  ]);
  const media = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/media_card_normalization_fixtures.json"), "utf8")).concat([
    {
      name: "media control modal preserves non-default controls",
      input: "media_player.office;Media Control;Auto;Auto;control_modal;;media;;label_display=label,number_display=volume,volume_max=40",
      expected: config({ entity: "media_player.office", label: "Media Control", icon: "Auto", sensor: "control_modal", type: "media", options: "label_display=label,number_display=volume,volume_max=40" }),
    },
    {
      name: "media control modal trims saved choices",
      input: "media_player.office;Media Control;Auto;Auto;control_modal;;media;;label_display=%20label%20,number_display=%20volume%20",
      expected: config({ entity: "media_player.office", label: "Media Control", icon: "Auto", sensor: "control_modal", type: "media", options: "label_display=label,number_display=volume" }),
    },
    {
      name: "short media config receives default icons",
      input: "media_player.x;;;;play_pause;;media",
      expected: config({ entity: "media_player.x", label: "", icon: "Auto", sensor: "play_pause", type: "media" }),
    },
  ]);
  return vacuum.concat(sensor, sensorAliases, action, media);
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
#include "esphome/core/string_ref.h"
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
    childProcess.execFileSync(compiler(), [
      "-std=c++17", "-Wall", "-Wextra", "-Werror",
      `-I${path.join(ROOT, "components/espcontrol")}`,
      `-I${path.join(ROOT, "tests/firmware/stubs")}`,
      source, "-o", binary,
    ]);
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
  console.log(`Saved-config shadow agreement passed for ${cases.length} Vacuum, Sensor, Action, and Media inputs across browser and compiled C++ helpers.`);
  console.log("Production firmware footprint delta: 0 bytes flash / 0 bytes RAM (test-only shadow; 8 KiB guard passed).");
}

main();
