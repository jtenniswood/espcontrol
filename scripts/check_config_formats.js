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

function splitFields(value, delim) {
  const out = [];
  let start = 0;
  while (start <= value.length) {
    let end = value.indexOf(delim, start);
    if (end < 0) end = value.length;
    out.push(value.slice(start, end));
    start = end + 1;
  }
  return out;
}

function decodeField(value) {
  return String(value || "").replace(/%([0-9a-fA-F]{2})/g, (_, hex) => {
    return String.fromCharCode(parseInt(hex, 16));
  });
}

function subpageTypeFromCode(code) {
  return {
    D: "calendar",
    S: "sensor",
    W: "weather",
    L: "slider",
    C: "cover",
    R: "garage",
    P: "push",
    I: "internal",
    G: "subpage",
  }[code || ""] || (code || "");
}

function firmwareParseButtonConfig(str) {
  const compact = str && str[0] === "~";
  const parts = compact ? splitFields(str.slice(1), ",").map(decodeField) : splitFields(str || "", ";");
  return {
    entity: parts[0] || "",
    label: parts[1] || "",
    icon: parts[2] || "",
    icon_on: parts[3] || "",
    sensor: parts[4] || "",
    unit: parts[5] || "",
    type: parts[6] || "",
    precision: parts[7] || "",
  };
}

function firmwareParseSubpageConfig(str) {
  if (!str) return { order: [], buttons: [] };
  const compact = str[0] === "~";
  const body = compact ? str.slice(1) : str;
  const pipes = splitFields(body, "|");
  if (pipes.length < 2) return { order: [], buttons: [] };
  const order = pipes[0] ? pipes[0].split(",").map((s) => s.trim()) : [];
  const buttons = [];
  for (let i = 1; i < pipes.length; i++) {
    if (compact) {
      const f = splitFields(pipes[i], ",");
      buttons.push({
        type: subpageTypeFromCode(f[0] || ""),
        entity: decodeField(f[1]),
        label: decodeField(f[2]),
        icon: decodeField(f[3]) || "Auto",
        icon_on: decodeField(f[4]) || "Auto",
        sensor: decodeField(f[5]),
        unit: decodeField(f[6]),
        precision: decodeField(f[7]),
      });
    } else {
      const f = splitFields(pipes[i], ":");
      buttons.push({
        entity: f[0] || "",
        label: f[1] || "",
        icon: f[2] || "Auto",
        icon_on: f[3] || "Auto",
        sensor: f[4] || "",
        unit: f[5] || "",
        type: f[6] || "",
        precision: f[7] || "",
      });
    }
  }
  return { order, buttons };
}

function buttonShape(b) {
  return {
    entity: b.entity || "",
    label: b.label || "",
    icon: b.icon || "Auto",
    icon_on: b.icon_on || "Auto",
    sensor: b.sensor || "",
    unit: b.unit || "",
    type: b.type || "",
    precision: b.precision || "",
  };
}

function subpageShape(sp) {
  return {
    order: Array.from(sp.order || []),
    buttons: Array.from(sp.buttons || [], buttonShape),
  };
}

function assertButtonRoundTrip(hooks, name, button, expectCompact) {
  const encoded = hooks.serializeButtonConfig(button);
  assert.strictEqual(encoded[0] === "~", expectCompact, `${name}: compact marker`);
  assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig(encoded)), buttonShape(button), `${name}: web round-trip`);
  assert.deepStrictEqual(buttonShape(firmwareParseButtonConfig(encoded)), buttonShape(button), `${name}: firmware parse`);
}

function assertSubpageRoundTrip(hooks, name, subpage, expectCompact) {
  const encoded = hooks.serializeSubpageConfig(subpage);
  assert.strictEqual(encoded[0] === "~", expectCompact, `${name}: compact marker`);
  assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig(encoded)), subpageShape(subpage), `${name}: web round-trip`);
  assert.deepStrictEqual(subpageShape(firmwareParseSubpageConfig(encoded)), subpageShape(subpage), `${name}: firmware parse`);
  return encoded;
}

const hooks = loadHooks();
assert(hooks, "web config helpers were not exported");

assertButtonRoundTrip(hooks, "normal button", {
  entity: "light.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Lightbulb",
  sensor: "sensor.kitchen_power",
  unit: "W",
  type: "",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "delimiter button", {
  entity: "sensor.kitchen_temperature",
  label: "Kitchen; west, 50% | prep: zone",
  icon: "Thermometer",
  icon_on: "Auto",
  sensor: "sensor.kitchen_temperature",
  unit: "deg;C",
  type: "sensor",
  precision: "1",
}, true);

assertButtonRoundTrip(hooks, "internal relay push button", {
  entity: "relay_1",
  label: "Door Strike",
  icon: "Gesture Tap",
  icon_on: "Auto",
  sensor: "push",
  unit: "",
  type: "internal",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "garage label button", {
  entity: "cover.garage",
  label: "Garage Door",
  icon: "Garage",
  icon_on: "Garage Open",
  sensor: "",
  unit: "",
  type: "garage",
  precision: "",
}, false);

assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig("light.legacy;Legacy;Auto;Lightbulb;sensor.legacy;W;sensor;1")), {
  entity: "light.legacy",
  label: "Legacy",
  icon: "Auto",
  icon_on: "Lightbulb",
  sensor: "sensor.legacy",
  unit: "W",
  type: "sensor",
  precision: "1",
}, "legacy button parse");

assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig("~light.compact,Compact%3B%20Label,Auto,Auto,sensor.compact,deg%3BC,sensor,2")), {
  entity: "light.compact",
  label: "Compact; Label",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "sensor.compact",
  unit: "deg;C",
  type: "sensor",
  precision: "2",
}, "compact button parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("1,B,2|light.legacy:Legacy:Auto:Lightbulb:::|sensor.room:Room:Thermometer:Auto:sensor.room:deg C:sensor:1")), {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.legacy", label: "Legacy", icon: "Auto", icon_on: "Lightbulb" }),
    buttonShape({ entity: "sensor.room", label: "Room", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.room", unit: "deg C", type: "sensor", precision: "1" }),
  ],
}, "legacy subpage parse");

assertSubpageRoundTrip(hooks, "normal subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.kitchen", label: "Kitchen", icon: "Auto", icon_on: "Lightbulb" }),
    buttonShape({ type: "calendar" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "internal relay subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "relay_1", label: "Relay", icon: "Power Plug", type: "internal" }),
    buttonShape({ entity: "relay_2", label: "Bell", icon: "Gesture Tap", sensor: "push", type: "internal" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "delimiter subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.zone", label: "Kitchen: west | 50%, main", icon: "Auto", icon_on: "Auto" }),
    buttonShape({ entity: "sensor.zone", label: "Temp: west | 50%", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.zone", unit: "deg:C", type: "sensor", precision: "1" }),
  ],
}, true);

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B,2|L,light.strip,Strip%20A,Lightbulb,Lightbulb%20On,h,,|S,sensor.temp,Temp,Thermometer,,sensor.temp,deg%20C,1")), {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.strip", label: "Strip A", icon: "Lightbulb", icon_on: "Lightbulb On", sensor: "h", type: "slider" }),
    buttonShape({ entity: "sensor.temp", label: "Temp", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.temp", unit: "deg C", type: "sensor", precision: "1" }),
  ],
}, "compact subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|D")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ type: "calendar" }),
  ],
}, "compact calendar subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|R,cover.garage,,Garage,Garage%20Open")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.garage", icon: "Garage", icon_on: "Garage Open", type: "garage" }),
  ],
}, "compact garage subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|R,cover.garage,Garage%20Door,Garage,Garage%20Open")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.garage", label: "Garage Door", icon: "Garage", icon_on: "Garage Open", type: "garage" }),
  ],
}, "compact garage label subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|I,relay_2,Gate,Power%20Plug,Power,push")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "relay_2", label: "Gate", icon: "Power Plug", icon_on: "Power", sensor: "push", type: "internal" }),
  ],
}, "compact internal relay subpage parse");

const largeSubpage = {
  order: Array.from({ length: 25 }, (_, i) => (i === 4 ? "B" : String(i + 1))),
  buttons: Array.from({ length: 25 }, (_, i) => buttonShape({
    entity: `light.room_${i + 1}`,
    label: `Room ${i + 1} scene with long descriptive label`,
    icon: "Lightbulb",
    icon_on: "Lightbulb On Outline",
  })),
};
const largeEncoded = assertSubpageRoundTrip(hooks, "oversized subpage", largeSubpage, false);
assert(largeEncoded.length > 255, "oversized subpage should exceed one ESPHome text value");

assert.strictEqual(
  hooks.shouldHideLogMessage("[12:48:01.064][S][text_sensor]: 'Screen: Date' >> '2026-04-22'"),
  true,
  "screen date status state logs should be hidden",
);
assert.strictEqual(
  hooks.shouldHideLogMessage("[12:48:23.929][S][sensor]: 'Wifi Strength' >> 92 %"),
  true,
  "wifi strength status state logs should be hidden",
);
assert.strictEqual(
  hooks.shouldHideLogMessage("[D][sensor:094]: 'Wifi Strength': Sending state 92.00000 % with 0 decimals of accuracy"),
  true,
  "wifi strength debug state logs should be hidden",
);
assert.strictEqual(
  hooks.shouldHideLogMessage("[S][sensor]: 'Indoor Temperature' >> 21.0 C"),
  false,
  "unrelated sensor status logs should stay visible",
);

console.log("Config format golden tests passed.");
