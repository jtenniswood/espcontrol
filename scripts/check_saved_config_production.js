#!/usr/bin/env node
"use strict";

const assert = require("assert");
const childProcess = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

const ROOT = path.resolve(__dirname, "..");

function compiler() {
  for (const candidate of [process.env.CXX, "c++", "g++", "clang++"].filter(Boolean)) {
    if (childProcess.spawnSync(candidate, ["--version"], { stdio: "ignore" }).status === 0) return candidate;
  }
  throw new Error("No C++ compiler found for saved-config production check");
}

function checkCompiledHelper() {
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "espcontrol-saved-config-production-"));
  try {
    const source = path.join(temporary, "saved_config_vacuum.cpp");
    const binary = path.join(temporary, "saved_config_vacuum");
    fs.writeFileSync(source, `
#include <cassert>
#include <string>
#include "button_grid_saved_config_vacuum_generated.h"
struct Config {
  std::string type;
  std::string sensor;
  std::string unit;
  std::string precision;
  std::string options;
  std::string icon_on;
};
int main() {
  Config start{"action", "vacuum.start", "area", "2", "unknown=1", "Custom"};
  assert(migrate_saved_config_vacuum_legacy(start));
  assert(start.type == "vacuum" && start.sensor == "start_stop");
  assert(start.unit.empty() && start.precision.empty() && start.options.empty());
  assert(start.icon_on == "Auto");
  Config dock{"action", "vacuum.return_to_base", "area", "2", "unknown=1", "Custom"};
  assert(migrate_saved_config_vacuum_legacy(dock));
  assert(dock.type == "vacuum" && dock.sensor == "dock");
  Config unrelated{"action", "light.turn_on", "", "", "", "Auto"};
  assert(!migrate_saved_config_vacuum_legacy(unrelated));
  assert(normalize_saved_config_vacuum_sensor("dock") == "dock");
  assert(normalize_saved_config_vacuum_sensor("unknown") == "start_stop");
  assert(normalize_saved_config_vacuum_icon_on("Custom") == "Auto");
  assert(normalize_saved_config_vacuum_precision("2").empty());
  assert(normalize_saved_config_vacuum_options("").empty());
  assert(normalize_saved_config_vacuum_options("unknown=1").empty());
}
`);
    childProcess.execFileSync(compiler(), [
      "-std=c++17", "-Wall", "-Wextra", "-Werror",
      `-I${path.join(ROOT, "components/espcontrol")}`, source, "-o", binary,
    ]);
    childProcess.execFileSync(binary);
  } finally {
    fs.rmSync(temporary, { recursive: true, force: true });
  }
}

function main() {
  const contract = JSON.parse(fs.readFileSync(path.join(ROOT, "common/config/card_contract.json"), "utf8"));
  const fields = contract.cards.vacuum.normalization.fields;
  assert.strictEqual(fields.sensor.policy, "allowed");
  assert.strictEqual(fields.sensor.fallback, "start_stop");
  assert.strictEqual(fields.icon_on.policy, "default");
  assert.strictEqual(fields.icon_on.value, "Auto");
  assert.strictEqual(fields.precision.policy, "clear");
  assert.strictEqual(fields.options.policy, "clear");
  assert.deepStrictEqual(contract.cards.vacuum.normalization.migrationActions, ["legacy_vacuum_start", "legacy_vacuum_dock"]);

  const generated = loadTypeScriptModule(path.join(ROOT, "src/webserver/generated/saved_config_vacuum.ts"));
  const start = { type: "action", sensor: "vacuum.start", unit: "area", precision: "2", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generated.migrateSavedConfigVacuumLegacy(start), true);
  assert.deepStrictEqual(start, { type: "vacuum", sensor: "start_stop", unit: "", precision: "", options: "", icon_on: "Auto" });
  const dock = { type: "action", sensor: "vacuum.return_to_base", unit: "area", precision: "2", options: "unknown=1", icon_on: "Custom" };
  assert.strictEqual(generated.migrateSavedConfigVacuumLegacy(dock), true);
  assert.strictEqual(dock.type, "vacuum");
  assert.strictEqual(dock.sensor, "dock");
  assert.strictEqual(generated.migrateSavedConfigVacuumLegacy({ type: "action", sensor: "light.turn_on" }), false);
  assert.strictEqual(generated.normalizeSavedConfigVacuumSensor("dock"), "dock");
  assert.strictEqual(generated.normalizeSavedConfigVacuumSensor("unknown"), "start_stop");
  assert.strictEqual(generated.normalizeSavedConfigVacuumIconOn("Custom"), "Auto");
  assert.strictEqual(generated.normalizeSavedConfigVacuumPrecision("2"), "");
  assert.strictEqual(generated.normalizeSavedConfigVacuumOptions(""), "");
  assert.strictEqual(generated.normalizeSavedConfigVacuumOptions("unknown=1"), "");

  const browser = fs.readFileSync(path.join(ROOT, "src/webserver/application/config_codec.ts"), "utf8");
  assert.match(browser, /from "\.\.\/generated\/saved_config_vacuum";/);
  assert.match(browser, /migrateSavedConfigVacuumLegacy\(b\)/);
  assert.doesNotMatch(browser, /b\.type === "action" && b\.sensor === "vacuum\.(?:start|return_to_base)"/);
  assert.match(browser, /sensor = normalizeSavedConfigVacuumSensor\(sensor\);/);
  assert.match(browser, /precision = normalizeSavedConfigVacuumPrecision\(precision\);/);
  assert.match(browser, /iconOn = normalizeSavedConfigVacuumIconOn\(iconOn\);/);
  assert.match(browser, /type === "vacuum"[\s\S]*?normalizeSavedConfigVacuumOptions\(options\)/);
  assert.doesNotMatch(browser, /type === "vacuum" \|\| type === "lawn_mower"/);

  const firmware = fs.readFileSync(path.join(ROOT, "components/espcontrol/button_grid_config_parser.h"), "utf8");
  assert.match(firmware, /#include "button_grid_saved_config_vacuum_generated\.h"/);
  const vacuumStart = firmware.indexOf('if (p.type == "vacuum")');
  const mowerStart = firmware.indexOf('if (p.type == "lawn_mower")', vacuumStart);
  assert(vacuumStart >= 0 && mowerStart > vacuumStart, "Vacuum production normalization block not found");
  const vacuumBlock = firmware.slice(vacuumStart, mowerStart);
  assert.match(firmware, /migrate_saved_config_vacuum_legacy\(p\)/);
  assert.doesNotMatch(firmware, /p\.type == "action" && p\.sensor == "vacuum\.(?:start|return_to_base)"/);
  assert.match(vacuumBlock, /p\.sensor = normalize_saved_config_vacuum_sensor\(p\.sensor\);/);
  assert.match(vacuumBlock, /p\.precision = normalize_saved_config_vacuum_precision\(p\.precision\);/);
  assert.match(vacuumBlock, /p\.icon_on = normalize_saved_config_vacuum_icon_on\(p\.icon_on\);/);
  assert.match(vacuumBlock, /p\.options = normalize_saved_config_vacuum_options\(p\.options\);/);
  assert.doesNotMatch(vacuumBlock, /p\.options\.clear\(\);/);

  checkCompiledHelper();
  console.log("Saved-config production check passed: Vacuum migrations and routine fields use generated browser and compiled firmware helpers.");
}

main();
