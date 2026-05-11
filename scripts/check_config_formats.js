#!/usr/bin/env node
"use strict";

const assert = require("assert");
const path = require("path");
const vm = require("vm");
const { loadBundledWebSource } = require("./web_source");

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
  vm.runInContext(loadBundledWebSource(), sandbox, { filename: SOURCE });
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
    A: "action",
    D: "calendar",
    T: "timezone",
    S: "sensor",
    W: "weather",
    F: "weather_forecast",
    L: "slider",
    C: "cover",
    N: "light_temperature",
    R: "garage",
    K: "lock",
    M: "media",
    H: "climate",
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
    options: parts[8] || "",
  };
}

function firmwareParseSubpageConfig(str) {
  if (!str) return { order: [], buttons: [] };
  const compact = str[0] === "~";
  const body = compact ? str.slice(1) : str;
  const pipes = splitFields(body, "|");
  if (pipes.length < 2) return { order: [], buttons: [] };
  const order = pipes[0] ? pipes[0].split(",").map((s) => {
    const token = s.trim();
    const eq = token.indexOf("=");
    return eq >= 0 ? token.slice(0, eq) : token;
  }) : [];
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
        options: decodeField(f[8]),
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
        options: f[8] || "",
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
    options: b.options || "",
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

function assertButtonMigration(hooks, name, encoded, expected) {
  assert.strictEqual(hooks.buttonConfigNeedsMigration(encoded), true, `${name}: migration detected`);
  const migrated = buttonShape(hooks.parseButtonConfig(encoded));
  assert.deepStrictEqual(migrated, buttonShape(expected), `${name}: migrated shape`);
  const canonical = hooks.serializeButtonConfig(migrated);
  assert.strictEqual(hooks.buttonConfigNeedsMigration(canonical), false, `${name}: canonical is idempotent`);
  assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig(canonical)), buttonShape(expected), `${name}: canonical round-trip`);
}

function assertSubpageRoundTrip(hooks, name, subpage, expectCompact) {
  const encoded = hooks.serializeSubpageConfig(subpage);
  assert.strictEqual(encoded[0] === "~", expectCompact, `${name}: compact marker`);
  assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig(encoded)), subpageShape(subpage), `${name}: web round-trip`);
  assert.deepStrictEqual(subpageShape(firmwareParseSubpageConfig(encoded)), subpageShape(subpage), `${name}: firmware parse`);
  return encoded;
}

function assertSubpageMigration(hooks, name, encoded, expected) {
  assert.strictEqual(hooks.subpageConfigNeedsMigration(encoded), true, `${name}: migration detected`);
  const migrated = subpageShape(hooks.parseSubpageConfig(encoded));
  assert.deepStrictEqual(migrated, subpageShape(expected), `${name}: migrated shape`);
  const canonical = hooks.serializeSubpageConfig(migrated);
  assert.strictEqual(hooks.subpageConfigNeedsMigration(canonical), false, `${name}: canonical is idempotent`);
  assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig(canonical)), subpageShape(expected), `${name}: canonical round-trip`);
}

const hooks = loadHooks();
assert(hooks, "web config helpers were not exported");
assert.strictEqual(hooks.previewHtmlValue({ labelHtml: "" }, "labelHtml", "fallback"), "", "empty preview label suppresses fallback");
assert.strictEqual(hooks.previewHtmlValue({}, "labelHtml", "fallback"), "fallback", "missing preview label uses fallback");
assert.strictEqual(hooks.normalizeTemperatureUnit("fahrenheit"), "°F", "fahrenheit unit normalization");
assert.strictEqual(hooks.normalizeTemperatureUnit("centigrade"), "°C", "centigrade unit normalization");
assert.strictEqual(hooks.normalizeScreensaverAction("Screen Dimmed"), "dim", "dimmed screensaver action normalization");
assert.strictEqual(hooks.screensaverActionOption("dim"), "Screen Dimmed", "dimmed screensaver action option");
assert.strictEqual(hooks.normalizeScreensaverDimmedBrightness(0), 10, "dimmed screensaver brightness fallback");
assert.strictEqual(hooks.normalizeScreensaverDimmedBrightness(101), 100, "dimmed screensaver brightness maximum");
assert.strictEqual(hooks.temperatureUnitSymbolFor("America/New_York (GMT-5)", "Auto"), "°F", "auto unit for US timezone");
assert.strictEqual(hooks.temperatureUnitSymbolFor("Europe/London (GMT+0)", "Auto"), "°C", "auto unit for UK timezone");
assert.strictEqual(hooks.temperatureUnitSymbolFor("Europe/London (GMT+0)", "°F"), "°F", "manual fahrenheit override");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 0), "wifi-strength-1", "wifi preview first strength icon");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 49), "wifi-strength-2", "wifi preview second strength icon");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 74), "wifi-strength-3", "wifi preview third strength icon");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 100), "wifi-strength-4", "wifi preview fourth strength icon");
assert.strictEqual(hooks.networkPreviewIconSlug("ethernet", 100), "ethernet", "ethernet preview icon");
const duplicateWrapGrid = Array.from({ length: 20 }, (_, i) => i + 1);
duplicateWrapGrid[1] = 0;
duplicateWrapGrid[2] = 0;
const duplicateWidePlacement = hooks.findDuplicatePlacementFor(duplicateWrapGrid, 19, 3, 20);
assert.strictEqual(duplicateWidePlacement.pos, 1, "duplicate placement wraps to earlier slots when a matching space exists");
assert.strictEqual(duplicateWidePlacement.size, 3, "duplicate placement preserves card size when the wrapped space fits");
duplicateWrapGrid[2] = 3;
const duplicateFallbackPlacement = hooks.findDuplicatePlacementFor(duplicateWrapGrid, 19, 3, 20);
assert.strictEqual(duplicateFallbackPlacement.pos, 1, "duplicate placement still wraps when copied size will not fit");
assert.strictEqual(duplicateFallbackPlacement.size, 1, "duplicate placement falls back to a normal card when the copied size will not fit");
const importedPlainOrder = hooks.importedButtonOrderFor("1,2,3", { 1: 2 });
assert.deepStrictEqual({
  grid: Array.from(importedPlainOrder.grid),
  sizes: Object.assign({}, importedPlainOrder.sizes),
}, {
  grid: [1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
  sizes: {},
}, "same-size imports clear stale button sizing");
const importedSizedOrder = hooks.importedButtonOrderFor("1d,2,3", {});
assert.strictEqual(importedSizedOrder.sizes["1"], 2, "imported button sizing is preserved");
const importedExtraTallOrder = hooks.importedButtonOrderFor("1t,2,3", {});
assert.strictEqual(importedExtraTallOrder.sizes["1"], 5, "imported extra tall sizing is preserved");
assert.deepStrictEqual(Array.from(importedExtraTallOrder.grid.slice(0, 11)), [1, 2, 3, 0, 0, -1, 0, 0, 0, 0, -1], "extra tall spans three rows");
const importedExtraWideOrder = hooks.importedButtonOrderFor("1x,2,3", {});
assert.strictEqual(importedExtraWideOrder.sizes["1"], 6, "imported extra wide sizing is preserved");
assert.deepStrictEqual(Array.from(importedExtraWideOrder.grid.slice(0, 5)), [1, -1, -1, 2, 3], "extra wide spans three columns");
const duplicateExtraWideGrid = Array.from({ length: 20 }, (_, i) => i + 1);
duplicateExtraWideGrid[1] = 0;
duplicateExtraWideGrid[2] = 3;
const duplicateExtraWideFallback = hooks.findDuplicatePlacementFor(duplicateExtraWideGrid, 19, 6, 20);
assert.strictEqual(duplicateExtraWideFallback.pos, 1, "extra wide duplicate placement falls back to a free single slot");
assert.strictEqual(duplicateExtraWideFallback.size, 1, "extra wide duplicate placement falls back to normal size when no matching space fits");
assert.strictEqual(hooks.screensaverTimeoutSupportedFor(10, false, 60, 3600), true, "short timeout allowed before limits load");
assert.strictEqual(hooks.screensaverTimeoutSupportedFor(10, true, 60, 3600), false, "short timeout blocked after old limits load");
assert.strictEqual(hooks.screensaverTimeoutSupportedFor(10, true, 10, 3600), true, "short timeout allowed after new limits load");

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

assertButtonRoundTrip(hooks, "switch text sensor when on", {
  entity: "switch.washing_machine",
  label: "Washer",
  icon: "Washer",
  icon_on: "Washer",
  sensor: "sensor.washing_machine_status",
  unit: "",
  type: "",
  precision: "text",
}, false);

const confirmSwitch = {
  entity: "switch.printer",
  label: "Printer",
  icon: "Printer 3D",
  icon_on: "Printer 3D",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "confirm_off,confirm_message=Stop the print?,confirm_yes=Power Down,confirm_no=Keep On",
};
assertButtonRoundTrip(hooks, "switch off confirmation", confirmSwitch, false);
const parsedConfirmSwitch = hooks.parseButtonConfig(hooks.serializeButtonConfig(confirmSwitch));
assert.strictEqual(hooks.switchConfirmationEnabled(parsedConfirmSwitch), true, "switch confirmation enabled");
assert.strictEqual(hooks.switchConfirmationMessage(parsedConfirmSwitch), "Stop the print?", "switch confirmation message");
assert.strictEqual(hooks.switchConfirmationYesText(parsedConfirmSwitch), "Power Down", "switch confirmation yes text");
assert.strictEqual(hooks.switchConfirmationNoText(parsedConfirmSwitch), "Keep On", "switch confirmation no text");

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

assertButtonRoundTrip(hooks, "large sensor numbers option", {
  entity: "sensor.blood_glucose",
  label: "Blood Glucose",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "sensor.blood_glucose",
  unit: "",
  type: "sensor",
  precision: "",
  options: "large_numbers",
}, false);

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

assertButtonRoundTrip(hooks, "lock button", {
  entity: "lock.front_door",
  label: "Front Door",
  icon: "Lock",
  icon_on: "Lock Open",
  sensor: "",
  unit: "",
  type: "lock",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "cover toggle button", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "toggle",
  unit: "",
  type: "cover",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "cover tilt button", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "tilt",
  unit: "",
  type: "cover",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "cover open command button", {
  entity: "cover.office_blind",
  label: "Open Blind",
  icon: "Blinds Open",
  icon_on: "Auto",
  sensor: "open",
  unit: "",
  type: "cover",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "cover close command button", {
  entity: "cover.office_blind",
  label: "Close Blind",
  icon: "Blinds",
  icon_on: "Auto",
  sensor: "close",
  unit: "",
  type: "cover",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "cover stop command button", {
  entity: "cover.office_blind",
  label: "Stop Blind",
  icon: "Stop",
  icon_on: "Auto",
  sensor: "stop",
  unit: "",
  type: "cover",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "cover set position command button", {
  entity: "cover.office_blind",
  label: "Half Blind",
  icon: "Blinds",
  icon_on: "Auto",
  sensor: "set_position",
  unit: "50",
  type: "cover",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "calendar large numbers option", {
  entity: "sensor.date",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "calendar",
  precision: "datetime",
  options: "large_numbers",
}, false);

assertButtonRoundTrip(hooks, "timezone card", {
  entity: "America/New_York (GMT-5)",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "timezone",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "timezone large numbers option", {
  entity: "America/New_York (GMT-5)",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "timezone",
  precision: "",
  options: "large_numbers",
}, false);

assertButtonRoundTrip(hooks, "weather tomorrow card", {
  entity: "weather.forecast_home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "weather",
  precision: "tomorrow",
}, false);

assertButtonRoundTrip(hooks, "weather tomorrow card custom label", {
  entity: "weather.forecast_home",
  label: "Garden",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "weather",
  precision: "tomorrow",
}, false);

assertButtonRoundTrip(hooks, "weather today card", {
  entity: "weather.forecast_home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "weather",
  precision: "today",
}, false);

assertButtonRoundTrip(hooks, "weather large temperature numbers option", {
  entity: "weather.forecast_home",
  label: "Today",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "weather",
  precision: "today",
  options: "large_numbers",
}, false);

assert.strictEqual(
  buttonShape(hooks.parseButtonConfig("weather.forecast_home;;;;;;weather;today;large_numbers")).options,
  "large_numbers",
  "weather forecast preserves large numbers option");
assert.strictEqual(
  buttonShape(hooks.parseButtonConfig("weather.forecast_home;;;;;;weather;;large_numbers")).options,
  "",
  "weather current conditions clears large numbers option");

assertButtonRoundTrip(hooks, "media play pause card", {
  entity: "media_player.living_room",
  label: "Play/Pause",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "play_pause",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "media play pause state card", {
  entity: "media_player.office",
  label: "Office",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "play_pause",
  unit: "",
  type: "media",
  precision: "state",
}, false);

assertButtonRoundTrip(hooks, "media previous card", {
  entity: "media_player.living_room",
  label: "Previous",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "previous",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "media next card", {
  entity: "media_player.living_room",
  label: "Next",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "next",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonMigration(hooks, "legacy media previous label", "media_player.living_room;Skip Previous;Auto;Auto;previous;;media", {
  entity: "media_player.living_room",
  label: "Previous",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "previous",
  type: "media",
});

assertButtonMigration(hooks, "legacy media next label", "media_player.living_room;Skip Next;Auto;Auto;next;;media", {
  entity: "media_player.living_room",
  label: "Next",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "next",
  type: "media",
});

assertButtonRoundTrip(hooks, "media volume card", {
  entity: "media_player.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "volume",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonMigration(hooks, "legacy media volume defaults", "media_player.kitchen;;Volume High;Auto;volume;;media", {
  entity: "media_player.kitchen",
  label: "Volume",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "volume",
  type: "media",
});

assertButtonRoundTrip(hooks, "media position card", {
  entity: "media_player.office",
  label: "Office",
  icon: "Progress Clock",
  icon_on: "Auto",
  sensor: "position",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonMigration(hooks, "legacy media position defaults", "media_player.office;;Progress Clock;Auto;position;;media", {
  entity: "media_player.office",
  label: "Position",
  icon: "Progress Clock",
  icon_on: "Auto",
  sensor: "position",
  type: "media",
});

assertButtonRoundTrip(hooks, "media now playing card", {
  entity: "media_player.office",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "now_playing",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "media now playing track position control", {
  entity: "media_player.office",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "now_playing",
  unit: "",
  type: "media",
  precision: "progress",
}, false);

assertButtonRoundTrip(hooks, "media now playing play pause control", {
  entity: "media_player.office",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "now_playing",
  unit: "",
  type: "media",
  precision: "play_pause",
}, false);

assertButtonRoundTrip(hooks, "climate card", {
  entity: "climate.living_room",
  label: "Living Room",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate",
  precision: "1",
}, false);

assertButtonRoundTrip(hooks, "climate card precision 2", {
  entity: "climate.bedroom",
  label: "Bedroom",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate",
  precision: "2",
}, false);

assertButtonRoundTrip(hooks, "climate card firmware precision 3", {
  entity: "climate.office",
  label: "Office",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate",
  precision: "3",
}, false);

assertButtonRoundTrip(hooks, "climate card custom range", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate",
  precision: "1:16:30",
}, false);

assertButtonMigration(hooks, "climate clears ignored fields", "climate.living_room;Living;Thermostat;Radiator;sensor.temp;deg C;climate;bad", {
  entity: "climate.living_room",
  label: "Living",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate",
  precision: "",
});

assertButtonRoundTrip(hooks, "light temperature card", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "kelvin",
  unit: "2000-6500",
  type: "light_temperature",
  precision: "color",
}, false);

const subpageStateOff = buttonShape({
  label: "Windows",
  icon: "Window Closed",
  type: "subpage",
});
const subpageStateIcon = buttonShape({
  label: "Lighting",
  icon: "Lightbulb",
  icon_on: "Lightbulb Group",
  sensor: "indicator",
  type: "subpage",
});
const subpageStateIconEntity = buttonShape({
  entity: "cover.office_blind",
  label: "Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "indicator",
  type: "subpage",
});
const subpageStateNumeric = buttonShape({
  label: "Open Windows",
  icon: "Window Closed",
  sensor: "sensor.open_windows",
  unit: "",
  type: "subpage",
});
const subpageStateNumericPrecision = buttonShape({
  label: "Average Temp",
  icon: "Thermometer",
  sensor: "sensor.average_temperature",
  unit: "°C",
  type: "subpage",
  precision: "1",
});
const subpageStateText = buttonShape({
  label: "Washer",
  icon: "Washer",
  sensor: "sensor.washer_state",
  type: "subpage",
  precision: "text",
});

assertButtonRoundTrip(hooks, "subpage state off", subpageStateOff, false);
assertButtonRoundTrip(hooks, "subpage state icon", subpageStateIcon, false);
assertButtonRoundTrip(hooks, "subpage state icon entity", subpageStateIconEntity, false);
assertButtonRoundTrip(hooks, "subpage state numeric", subpageStateNumeric, false);
assertButtonRoundTrip(hooks, "subpage state numeric precision", subpageStateNumericPrecision, false);
assertButtonRoundTrip(hooks, "subpage state text", subpageStateText, false);

assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateOff), "off", "subpage state off");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateIcon), "icon", "subpage icon state");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateIconEntity), "icon", "subpage icon entity state");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateNumeric), "numeric", "subpage numeric state");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateText), "text", "subpage text state");

assertButtonMigration(hooks, "legacy weather forecast card", "weather.forecast_home;Weather;Auto;Auto;;;weather_forecast", {
  entity: "weather.forecast_home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "weather",
  precision: "tomorrow",
});

assertButtonMigration(hooks, "legacy text sensor card", "sensor.washer_state;Washer;Washer;Auto;;;text_sensor", {
  entity: "",
  label: "",
  icon: "Washer",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "sensor",
  precision: "text",
});

assertButtonMigration(hooks, "legacy media controls card", "media_player.living_room;Living Room;Speaker;Auto;controls;;media", {
  entity: "media_player.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "play_pause",
  unit: "",
  type: "media",
  precision: "",
});

assertButtonRoundTrip(hooks, "scene action card", {
  entity: "scene.movie_mode",
  label: "Movie Mode",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "scene.turn_on",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "script action card", {
  entity: "script.goodnight",
  label: "Goodnight",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "automation action card", {
  entity: "automation.goodnight",
  label: "Goodnight Automation",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "automation.trigger",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "button action card", {
  entity: "button.restart_router",
  label: "Restart Router",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "button.press",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "input button action card", {
  entity: "input_button.doorbell",
  label: "Doorbell",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_button.press",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "input boolean toggle action card", {
  entity: "input_boolean.guest_mode",
  label: "Guest Mode",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_boolean.toggle",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "input number action card", {
  entity: "input_number.target_level",
  label: "Target Level",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_number.set_value",
  unit: "50",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "input select delimiter action card", {
  entity: "input_select.house_mode",
  label: "House Mode",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_select.select_option",
  unit: "Away; overnight | 50%, main",
  type: "action",
  precision: "",
}, true);

assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig("light.legacy;Legacy;Auto;Lightbulb;sensor.legacy;W;sensor;1")), buttonShape({
  entity: "light.legacy",
  label: "Legacy",
  icon: "Auto",
  icon_on: "Lightbulb",
  sensor: "sensor.legacy",
  unit: "W",
  type: "sensor",
  precision: "1",
}), "legacy button parse");

assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig("~light.compact,Compact%3B%20Label,Auto,Auto,sensor.compact,deg%3BC,sensor,2")), buttonShape({
  entity: "light.compact",
  label: "Compact; Label",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "sensor.compact",
  unit: "deg;C",
  type: "sensor",
  precision: "2",
}), "compact button parse");

assertButtonMigration(hooks, "legacy horizontal slider card", "light.strip;Strip;Lightbulb;Lightbulb On;h;;slider", {
  entity: "light.strip",
  label: "Strip",
  icon: "Lightbulb",
  icon_on: "Lightbulb On",
  sensor: "",
  unit: "",
  type: "slider",
  precision: "",
});

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("1,B,2|light.legacy:Legacy:Auto:Lightbulb:::|sensor.room:Room:Thermometer:Auto:sensor.room:deg C:sensor:1")), {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.legacy", label: "Legacy", icon: "Auto", icon_on: "Lightbulb" }),
    buttonShape({ entity: "sensor.room", label: "Room", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.room", unit: "deg C", type: "sensor", precision: "1" }),
  ],
}, "legacy subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("1,B|cover.office_blind:Office Blind:Blinds:Blinds Open:tilt::cover")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Office Blind", icon: "Blinds", icon_on: "Blinds Open", sensor: "tilt", type: "cover" }),
  ],
}, "legacy cover tilt subpage parse");

assertSubpageMigration(hooks, "legacy mixed subpage migration", "1,B,2,3,4|weather.forecast_home:Weather:Auto:Auto:::weather_forecast|sensor.washer_state:Washer:Washer:Auto:::text_sensor|media_player.living_room:Living Room:Speaker:Auto:controls::media|light.strip:Strip:Lightbulb:Lightbulb On:h::slider", {
  order: ["1", "B", "2", "3", "4"],
  buttons: [
    buttonShape({ entity: "weather.forecast_home", label: "", icon: "Auto", icon_on: "Auto", type: "weather", precision: "tomorrow" }),
    buttonShape({ entity: "", label: "", icon: "Washer", icon_on: "Auto", type: "sensor", precision: "text" }),
    buttonShape({ entity: "media_player.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", sensor: "play_pause", type: "media" }),
    buttonShape({ entity: "light.strip", label: "Strip", icon: "Lightbulb", icon_on: "Lightbulb On", type: "slider" }),
  ],
});

assertSubpageRoundTrip(hooks, "normal subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.kitchen", label: "Kitchen", icon: "Auto", icon_on: "Lightbulb" }),
    buttonShape({ type: "calendar" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "date time large numbers subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ type: "calendar", precision: "datetime", options: "large_numbers" }),
    buttonShape({ entity: "America/New_York (GMT-5)", type: "timezone", options: "large_numbers" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "switch confirmation subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape(confirmSwitch),
  ],
}, false);

assertSubpageRoundTrip(hooks, "internal relay subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "relay_1", label: "Relay", icon: "Power Plug", type: "internal" }),
    buttonShape({ entity: "relay_2", label: "Bell", icon: "Gesture Tap", sensor: "push", type: "internal" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "cover toggle subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Office Blind", icon: "Blinds", icon_on: "Blinds Open", sensor: "toggle", type: "cover" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "cover tilt subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Office Blind", icon: "Blinds", icon_on: "Blinds Open", sensor: "tilt", type: "cover" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "cover command subpage", {
  order: ["1", "B", "2", "3"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Open", icon: "Blinds Open", icon_on: "Auto", sensor: "open", type: "cover" }),
    buttonShape({ entity: "cover.office_blind", label: "Stop", icon: "Stop", icon_on: "Auto", sensor: "stop", type: "cover" }),
    buttonShape({ entity: "cover.office_blind", label: "50%", icon: "Blinds", icon_on: "Auto", sensor: "set_position", unit: "50", type: "cover" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "action subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "scene.movie_mode", label: "Movie Mode", icon: "Flash", sensor: "scene.turn_on", type: "action" }),
    buttonShape({ entity: "input_select.house_mode", label: "House Mode", icon: "Flash", sensor: "input_select.select_option", unit: "Away: overnight | 50%, main", type: "action" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "lock subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "lock.front_door", label: "Front Door", icon: "Lock", icon_on: "Lock Open", type: "lock" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "media subpage", {
  order: ["1", "B", "2", "3", "4", "5", "6"],
  buttons: [
    buttonShape({ entity: "media_player.living_room", label: "Play/Pause", icon: "Auto", sensor: "play_pause", type: "media" }),
    buttonShape({ entity: "media_player.living_room", label: "Previous", icon: "Auto", sensor: "previous", type: "media" }),
    buttonShape({ entity: "media_player.living_room", label: "Next", icon: "Auto", sensor: "next", type: "media" }),
    buttonShape({ entity: "media_player.kitchen", label: "Kitchen", icon: "Auto", sensor: "volume", type: "media" }),
    buttonShape({ entity: "media_player.office", label: "Office", icon: "Progress Clock", sensor: "position", type: "media" }),
    buttonShape({ entity: "media_player.office", label: "", icon: "Auto", sensor: "now_playing", type: "media" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.living_room", label: "Living Room", type: "climate", precision: "1" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage custom range", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", type: "climate", precision: "0:16:30" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "light temperature subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "light.living_room", label: "Living Room", icon: "Auto", sensor: "kelvin", unit: "2000-6500", type: "light_temperature", precision: "color" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "delimiter subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.zone", label: "Kitchen: west | 50%, main", icon: "Auto", icon_on: "Auto" }),
    buttonShape({ entity: "sensor.zone", label: "Temp: west | 50%", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.zone", unit: "deg:C", type: "sensor", precision: "1", options: "large_numbers" }),
  ],
}, true);

const customBackLabelSubpage = {
  order: ["1", "B", "2"],
  backLabel: "Return Home",
  buttons: [
    buttonShape({ entity: "light.zone", label: "Zone", icon: "Auto", icon_on: "Auto" }),
    buttonShape({ entity: "sensor.zone", label: "Temp", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.zone", unit: "°C", type: "sensor", precision: "1" }),
  ],
};
const customBackLabelEncoded = assertSubpageRoundTrip(hooks, "custom back label subpage", customBackLabelSubpage, true);
assert.strictEqual(hooks.parseSubpageConfig(customBackLabelEncoded).backLabel, "Return Home", "custom back label round-trips through subpage config");

assert.strictEqual(hooks.backOrderToken("B", "Back"), "B", "default back label keeps compact B token");
assert.strictEqual(hooks.backLabelFromOrder(["1", "B", "2"]), "Back", "missing back label defaults to Back");
assert.strictEqual(JSON.stringify(hooks.parseBackOrderToken("Bw=Return%20Home")), JSON.stringify({
  token: "Bw",
  label: "Return Home",
}), "back order token decodes custom label");
assert.strictEqual(JSON.stringify(hooks.parseBackOrderToken("Bt=Return%20Home")), JSON.stringify({
  token: "Bt",
  label: "Return Home",
}), "extra tall back order token decodes custom label");
assert.strictEqual(JSON.stringify(hooks.parseBackOrderToken("Bx=Return%20Home")), JSON.stringify({
  token: "Bx",
  label: "Return Home",
}), "extra wide back order token decodes custom label");
assertSubpageRoundTrip(hooks, "extra tall and extra wide subpage order", {
  order: ["Bt", "1t", "", "", "", "Bx", "2x"],
  buttons: [
    buttonShape({ entity: "light.tall", label: "Tall", icon: "Auto", icon_on: "Auto" }),
    buttonShape({ entity: "light.wide", label: "Wide", icon: "Auto", icon_on: "Auto" }),
  ],
}, false);

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B,2|L,light.strip,Strip%20A,Lightbulb,Lightbulb%20On,h,,|S,sensor.temp,Temp,Thermometer,,sensor.temp,deg%20C,1")), {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "light.strip", label: "Strip A", icon: "Lightbulb", icon_on: "Lightbulb On", sensor: "", type: "slider" }),
    buttonShape({ entity: "sensor.temp", label: "Temp", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.temp", unit: "deg C", type: "sensor", precision: "1" }),
  ],
}, "compact subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|D")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ type: "calendar" }),
  ],
}, "compact calendar subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|T,America/New_York%20%28GMT-5%29")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "America/New_York (GMT-5)", type: "timezone" }),
  ],
}, "compact timezone subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|F,weather.forecast_home")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "weather.forecast_home", type: "weather", precision: "tomorrow" }),
  ],
}, "compact weather forecast subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|W,weather.forecast_home,,,,,,today")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "weather.forecast_home", type: "weather", precision: "today" }),
  ],
}, "compact weather today subpage parse");

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

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|C,cover.office_blind,Office%20Blind,Blinds,Blinds%20Open,toggle")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Office Blind", icon: "Blinds", icon_on: "Blinds Open", sensor: "toggle", type: "cover" }),
  ],
}, "compact cover toggle subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|C,cover.office_blind,Office%20Blind,Blinds,Blinds%20Open,tilt")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Office Blind", icon: "Blinds", icon_on: "Blinds Open", sensor: "tilt", type: "cover" }),
  ],
}, "compact cover tilt subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|C,cover.office_blind,Office%20Blind,Blinds,,set_position,35")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.office_blind", label: "Office Blind", icon: "Blinds", icon_on: "Auto", sensor: "set_position", unit: "35", type: "cover" }),
  ],
}, "compact cover set position subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|I,relay_2,Gate,Power%20Plug,Power,push")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "relay_2", label: "Gate", icon: "Power Plug", icon_on: "Power", sensor: "push", type: "internal" }),
  ],
}, "compact internal relay subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|A,scene.movie_mode,Movie%20Mode,Flash,,scene.turn_on")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "scene.movie_mode", label: "Movie Mode", icon: "Flash", icon_on: "Auto", sensor: "scene.turn_on", type: "action" }),
  ],
}, "compact action subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|K,lock.front_door,Front%20Door,Lock,Lock%20Open")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "lock.front_door", label: "Front Door", icon: "Lock", icon_on: "Lock Open", type: "lock" }),
  ],
}, "compact lock subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|M,media_player.living_room,Play%2FPause,,,play_pause")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "media_player.living_room", label: "Play/Pause", icon: "Auto", icon_on: "Auto", sensor: "play_pause", type: "media" }),
  ],
}, "compact media subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|M,media_player.living_room,Living%20Room,Speaker,,controls")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "media_player.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", sensor: "play_pause", type: "media" }),
  ],
}, "legacy media controls subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|H,climate.living_room,Living%20Room,,,,,1")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.living_room", label: "Living Room", type: "climate", precision: "1" }),
  ],
}, "compact climate subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|H,climate.hallway,Hallway,,,,,0%3A16%3A30")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", type: "climate", precision: "0:16:30" }),
  ],
}, "compact climate range subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|N,light.living_room,Living%20Room,,,kelvin,2000-6500,color")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "light.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", sensor: "kelvin", unit: "2000-6500", type: "light_temperature", precision: "color" }),
  ],
}, "compact light temperature subpage parse");

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

console.log("Config format golden tests passed.");
