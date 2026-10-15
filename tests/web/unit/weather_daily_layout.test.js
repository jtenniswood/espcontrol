"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { spawnSync } = require("node:child_process");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");

const root = path.resolve(__dirname, "../../..");
const { planWeatherCardLayout } = loadTypeScriptModule(path.join(root, "src/webserver/application/weather_daily_layout.ts"));
const metrics = [84, 96, 112, 50, 26, 13];

test("S3 card spans choose readable rows and grids with stacked narrow tiles", () => {
  const spans = [
    [1, 1, 1, 1, false], [2, 1, 2, 1, false], [3, 1, 4, 1, false],
    [1, 2, 1, 2, false], [2, 2, 2, 2, false], [3, 2, 4, 2, false],
    [1, 3, 1, 3, false], [2, 3, 2, 3, false], [3, 3, 3, 3, false],
  ];
  for (const [wide, tall, columns, rows, list] of spans) {
    const width = 150 * wide + 10 * (wide - 1) - 28;
    const height = 138 * tall + 10 * (tall - 1) - 28;
    assert.deepEqual(planWeatherCardLayout(width, height, ...metrics, 8),
      { count: Math.min(8, columns * rows), columns, rows, list }, `${wide}w ${tall}h`);
  }
  assert.deepEqual(planWeatherCardLayout(442, 258, ...metrics, 6),
    { count: 6, columns: 3, rows: 2, list: false }, "shorter provider forecasts rebalance the grid");
  assert.deepEqual(planWeatherCardLayout(122, 82, ...metrics, 8),
    { count: 1, columns: 1, rows: 1, list: true }, "stale status leaves a readable compact row");
});

test("browser and firmware agree across sizes and provider limits", () => {
  const cases = [];
  for (const width of [100, 122, 194, 282, 442, 900]) {
    for (const height of [82, 110, 205, 258, 406, 700]) {
      for (const available of [0, 1, 2, 5, 6, 7, 8, 9]) cases.push([width, height, ...metrics, available]);
    }
  }
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "weather-layout-parity-"));
  try {
    const source = path.join(temporary, "layout.cpp");
    const binary = path.join(temporary, "layout");
    fs.writeFileSync(source, `#include <iostream>
#include "weather_daily_forecast.h"
int main() {
  int w,h,mw,mh,lw,lh,cg,rg; size_t available;
  size_t cases; std::cin >> cases;
  for(size_t i=0;i<cases;++i) {
    if(!(std::cin >> w >> h >> mw >> mh >> lw >> lh >> cg >> rg >> available)) return 1;
    const auto plan=espcontrol::weather::card_layout(w,h,mw,mh,lw,lh,cg,rg,available);
    std::cout << plan.count << ' ' << plan.columns << ' ' << plan.rows << ' ' << plan.list << '\\n';
  }
}
`);
    const compiled = spawnSync(process.env.CXX || "c++", ["-std=c++17", "-Wall", "-Wextra", "-Werror",
      "-I", path.join(root, "components/espcontrol"), source, "-o", binary], { encoding: "utf8" });
    assert.equal(compiled.status, 0, compiled.stderr);
    const result = spawnSync(binary, [], { input: cases.length + "\n" + cases.map(values => values.join(" ")).join("\n") + "\n", encoding: "utf8", timeout: 10000 });
    assert.equal(result.status, 0, result.stderr);
    const plans = result.stdout.trim().split("\n").map(line => line.split(" ").map(Number));
    assert.equal(plans.length, cases.length);
    for (const [index, values] of cases.entries()) {
      const plan = planWeatherCardLayout(...values);
      assert.deepEqual([plan.count, plan.columns, plan.rows, Number(plan.list)], plans[index], values.join(" "));
      assert(plan.count <= Math.min(values[8], 8));
      assert(plan.count <= plan.columns * plan.rows);
    }
  } finally {
    fs.rmSync(temporary, { recursive: true, force: true });
  }
});
