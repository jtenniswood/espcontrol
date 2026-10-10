"use strict";
const test = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const fs = require("node:fs");
const path = require("node:path");

test("sensor colour rules serialization and matching", () => {
  const { runSensorColourRulesTests } = loadTypescriptTest("tests/web/sensor_colour_rules.test.ts");
  const data = fs.readFileSync(path.resolve(__dirname, "../../../common/config/unicode_case_folding_16.txt"), "utf8");
  const mappings = data.split("\n").flatMap(line => {
    const fields = line.split("#")[0].split(";").map(field => field.trim());
    return ["C", "F"].includes(fields[1]) ? [[String.fromCodePoint(parseInt(fields[0], 16)), fields[2].split(" ").map(code => String.fromCodePoint(parseInt(code, 16))).join("")]] : [];
  });
  const defaults = JSON.parse(fs.readFileSync(path.resolve(__dirname, "../../../common/config/sensor_colour_default_fixtures.json"), "utf8"));
  const whitespace = JSON.parse(fs.readFileSync(path.resolve(__dirname, "../../../common/config/sensor_colour_whitespace_fixtures.json"), "utf8"));
  runSensorColourRulesTests(mappings, defaults, whitespace);
});
