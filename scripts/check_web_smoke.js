#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { loadBundledWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "entry.js");
const DEVICE_MANIFEST = path.join(ROOT, "devices", "manifest.json");
const WEB_OUTPUT_DIR = path.join(ROOT, "docs", "public", "webserver");
const ALL_ROTATIONS = ["0", "90", "180", "270"];

function createWebSandbox() {
  const domEvents = [];
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: {
      readyState: "loading",
      activeElement: null,
      addEventListener(type, listener) {
        domEvents.push({ type, listener });
      },
    },
  };
  sandbox.__domEvents = domEvents;
  sandbox.window = sandbox;
  sandbox.globalThis = sandbox;
  return sandbox;
}

function loadHooks() {
  const sandbox = createWebSandbox();
  vm.createContext(sandbox);
  vm.runInContext(loadBundledWebSource(), sandbox, { filename: SOURCE });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

function assertGeneratedRotationOptions(slug, generated, key, options) {
  assert(
    generated.includes(`${key}:${JSON.stringify(options)}`),
    `${slug}: generated web UI must include ${key} ${JSON.stringify(options)}`
  );
}

const hooks = loadHooks();
assert(hooks, "web test hooks were not exported");
assert.deepStrictEqual(Array.from(hooks.buttonTypesMissingCardMetadata()), [], "all registered card types define card metadata");
assert.deepStrictEqual(Array.from(hooks.SSE_ALIAS_GROUPS.clockBar), [
  "switch-screen__clock_bar",
  "switch-screen_clock_bar",
  "switch-clock_bar_enabled",
], "clock bar SSE aliases are registered together");
assert.deepStrictEqual(Array.from(hooks.SSE_ALIAS_GROUPS.scheduleWakeTimeout), [
  "number-screen__schedule_wake_timeout",
  "number-screen_schedule_wake_timeout",
  "number-schedule_wake_timeout",
], "schedule wake timeout SSE aliases are registered together");
assert.deepStrictEqual(Array.from(hooks.SSE_ALIAS_GROUPS.ntpServer1), [
  "text-screen__ntp_server_1",
  "text-ntp_server_1",
], "NTP server SSE aliases are registered together");

const manifest = JSON.parse(fs.readFileSync(DEVICE_MANIFEST, "utf8"));
for (const slug of Object.keys(manifest.devices || {})) {
  const webOutput = path.join(WEB_OUTPUT_DIR, slug, "www.js");
  const generated = fs.readFileSync(webOutput, "utf8");
  const sandbox = createWebSandbox();
  vm.createContext(sandbox);
  vm.runInContext(generated, sandbox, { filename: webOutput });
  assert(
    sandbox.__ESPCONTROL_TEST_HOOKS__.config,
    `${slug}: generated web UI must export the same test hooks used by local checks`
  );
  const generatedHooks = sandbox.__ESPCONTROL_TEST_HOOKS__.config;
  const generatedTimezones = Array.from(generatedHooks.defaultTimezoneOptions());
  assert(
    generatedTimezones.includes("UTC (GMT+0)") && generatedTimezones.includes("Europe/London (GMT+0)"),
    `${slug}: generated web UI must include fallback timezone choices`
  );
  assert(
    Array.from(generatedHooks.timezoneOptionsWithFallback([], "Custom/Zone (GMT+0)")).includes("Custom/Zone (GMT+0)"),
    `${slug}: timezone fallback must preserve the selected value`
  );
  assert(
    sandbox.__domEvents.some((event) => event.type === "DOMContentLoaded" && typeof event.listener === "function"),
    `${slug}: generated web UI must register DOMContentLoaded startup wiring`
  );
}

for (const [slug, device] of Object.entries(manifest.devices || {})) {
  if (!device.rotation || !device.rotation.enabled) continue;
  const webOutput = path.join(WEB_OUTPUT_DIR, slug, "www.js");
  const generated = fs.readFileSync(webOutput, "utf8");
  const featureConfig = generated.match(/features:\{[^}]*\}/)?.[0] || "";
  assert(
    /features:\{[^}]*screenRotation:!0/.test(generated),
    `${slug}: generated web UI must expose screen rotation when rotation is enabled`
  );
  assert.deepStrictEqual(device.rotation.options, ALL_ROTATIONS, `${slug}: normal rotation options`);
  assert.strictEqual(device.rotation.experimentalOptions, undefined, `${slug}: no hidden rotation options`);
  assertGeneratedRotationOptions(slug, featureConfig, "screenRotationOptions", ALL_ROTATIONS);
  assert(
    !featureConfig.includes("screenRotationExperimentalOptions"),
    `${slug}: generated web UI must not hide rotation options behind the dev flag`
  );
}

const button = {
  entity: "light.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Lightbulb",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "",
};

const encoded = hooks.serializeButtonConfig(button);
assert.strictEqual(encoded, "light.kitchen;Kitchen;Auto;Lightbulb");
assert.deepStrictEqual(plain(hooks.parseButtonConfig(encoded)), button);

const confirmationButton = {
  entity: "switch.printer",
  label: "3D Printer",
  icon: "Printer 3D",
  icon_on: "Printer 3D",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "confirm_off,confirm_message=Stop the print?,confirm_yes=Power Down,confirm_no=Keep On",
};
const confirmationRoundTrip = hooks.parseButtonConfig(hooks.serializeButtonConfig(confirmationButton));
assert.deepStrictEqual(plain(confirmationRoundTrip), confirmationButton);
assert.strictEqual(hooks.switchConfirmationEnabled(confirmationRoundTrip), true);
assert.strictEqual(hooks.switchConfirmationMode(confirmationRoundTrip), "off");
assert.strictEqual(hooks.switchConfirmationMessage(confirmationRoundTrip), "Stop the print?");
assert.strictEqual(hooks.switchConfirmationYesText(confirmationRoundTrip), "Power Down");
assert.strictEqual(hooks.switchConfirmationNoText(confirmationRoundTrip), "Keep On");
const confirmationOnRoundTrip = hooks.parseButtonConfig(hooks.serializeButtonConfig({
  entity: "switch.printer",
  label: "3D Printer",
  icon: "Printer 3D",
  icon_on: "Printer 3D",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "confirm_on",
}));
assert.strictEqual(hooks.switchConfirmationMode(confirmationOnRoundTrip), "on");
assert.strictEqual(hooks.switchConfirmationMessage(confirmationOnRoundTrip), "Turn on this device?");
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("alarm", false, false), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("alarm", true, false), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("alarm", true, true), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("alarm_action", false, false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("alarm_action", false, true), false);
assert(
  hooks.buttonTypePreviewFor("alarm", { label: "Alarm", icon: "Security", type: "alarm" }).iconHtml.includes("mdi-shield-off"),
  "alarm preview defaults to the status icon"
);
assert(
  hooks.buttonTypePreviewFor("alarm", { label: "Alarm", icon: "Alarm", type: "alarm", options: "icon_display=static" }).iconHtml.includes("mdi-bell-ring"),
  "alarm preview uses the selected Alarm icon"
);
assert.deepStrictEqual(Array.from(hooks.alarmCardTypeOptionValues(false)), ["control_panel", "away", "home", "disarm"]);
assert.deepStrictEqual(Array.from(hooks.alarmCardTypeOptionValues(true)), ["control_panel", "away", "home", "disarm"]);
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(hooks.parseButtonConfig(
  "alarm_control_panel.house;House;Security;Auto;;;alarm;;actions=away%7Cdisarm"
))), ["away", "disarm"]);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("fan_speed", false, false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("fan_speed", true, false), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("fan_speed", true, true), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("fan_switch", true, false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("fan_oscillate", true, true), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("option_select", false, false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("option_select", false, true), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("todo", false, false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("todo", true, false), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerForExperimental("todo", true, true), true);
assert(
  hooks.buttonTypePickerKeysForExperimental(false, false, "fan_speed").includes("fan_speed"),
  "saved fan cards remain represented while hidden"
);
assert(
  !hooks.buttonTypePickerKeysForExperimental(false, false, "todo").includes("todo"),
  "saved todo cards stay hidden in the picker while developer flag is off"
);

assert.strictEqual(hooks.normalizeTemperatureUnit("fahrenheit"), "\u00b0F");
assert.strictEqual(hooks.normalizeTemperatureUnit("centigrade"), "\u00b0C");
const climatePreviewButton = {
  entity: "climate.home",
  label: "Home",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate",
  precision: "",
  options: "",
};
const climatePreviewC = hooks.buttonTypePreviewFor("climate", climatePreviewButton, {
  temperatureUnit: "\u00b0C",
});
assert.strictEqual(climatePreviewC.buttonClass, "sp-climate-temp-card", "climate temperature preview uses temperature card class");
assert(climatePreviewC.iconHtml.includes("\u00b0C"), "climate preview uses Celsius unit");
const climateLargePreview = hooks.buttonTypePreviewFor("climate", {
  ...climatePreviewButton,
  options: "large_numbers",
}, {
  cardSize: 4,
  temperatureUnit: "\u00b0C",
});
assert(climateLargePreview.iconHtml.includes("sp-sensor-preview-large"), "climate temperature preview supports large numbers");
const climatePreviewF = hooks.buttonTypePreviewFor("climate", climatePreviewButton, {
  temperatureUnit: "\u00b0F",
});
assert(climatePreviewF.iconHtml.includes("\u00b0F"), "climate preview uses Fahrenheit unit");
const climatePreviewAuto = hooks.buttonTypePreviewFor("climate", climatePreviewButton, {
  temperatureUnit: "Auto",
  timezone: "America/New_York (GMT-5)",
});
assert(climatePreviewAuto.iconHtml.includes("\u00b0F"), "climate preview follows Auto timezone unit");
const climateLabelPreview = hooks.buttonTypePreviewFor("climate", {
  ...climatePreviewButton,
  options: "label_display=actual",
}, {
  temperatureUnit: "\u00b0F",
});
assert(climateLabelPreview.labelHtml.includes("21\u00b0F"), "climate actual label includes the configured unit");
const climateIconPreview = hooks.buttonTypePreviewFor("climate", {
  ...climatePreviewButton,
  options: "number_display=icon",
}, {
  temperatureUnit: "\u00b0C",
});
assert(climateIconPreview.iconHtml.includes("mdi-thermostat"), "climate icon mode preview uses the selected icon");
assert.strictEqual(climateIconPreview.buttonClass, undefined, "climate icon mode uses a standard card wrapper");
assert(!climateIconPreview.iconHtml.includes("sp-climate-card-icon"), "climate icon mode uses standard card icon layout");
assert(!climateIconPreview.iconHtml.includes("\u00b0C"), "climate icon mode preview does not show a large temperature");

function previewSensorValue(preview) {
  return (preview.iconHtml.match(/sp-sensor-value[^>]*>([^<]*)/) || [])[1] || "";
}

const datePreview = hooks.buttonTypePreviewFor("calendar", {
  type: "calendar",
  precision: "",
  options: "",
});
assert(datePreview.labelHtml.includes("mdi-calendar-month"), "date preview uses the calendar badge");
assert(datePreview.iconHtml.includes("sp-sensor-preview"), "date preview uses the shared sensor preview");

const dateTimePreview = hooks.buttonTypePreviewFor("calendar", {
  type: "calendar",
  precision: "datetime",
  options: "large_numbers",
}, {
  cardSize: 4,
  clockFormat: "24h",
});
assert(dateTimePreview.iconHtml.includes("sp-sensor-preview-large"), "date/time preview supports large numbers");
assert(previewSensorValue(dateTimePreview).includes(":"), "date/time preview renders a time value");

const wideDateTimePreview = hooks.buttonTypePreviewFor("calendar", {
  type: "calendar",
  precision: "datetime",
  options: "large_numbers",
}, {
  cardSize: 3,
  clockFormat: "24h",
});
assert(wideDateTimePreview.iconHtml.includes("sp-sensor-preview-large"), "date/time wide preview supports large numbers");
assert.strictEqual(wideDateTimePreview.labelHtml, "", "date/time wide large preview hides the date label");

const clockPreview = hooks.buttonTypePreviewFor("clock", {
  type: "clock",
  options: "large_numbers",
}, {
  cardSize: 4,
  clockFormat: "24h",
});
assert(clockPreview.iconHtml.includes("sp-sensor-preview-large"), "clock preview supports large numbers");
assert(previewSensorValue(clockPreview).includes(":"), "clock preview renders a time value");
assert.strictEqual(clockPreview.labelHtml, "", "clock preview does not render a date label");

const wideClockPreview = hooks.buttonTypePreviewFor("clock", {
  type: "clock",
  options: "large_numbers",
}, {
  cardSize: 3,
  clockFormat: "24h",
});
assert(wideClockPreview.iconHtml.includes("sp-sensor-preview-large"), "clock wide preview supports large numbers");
assert.strictEqual(wideClockPreview.labelHtml, "", "clock wide preview does not render a date label");

const timezonePreview = hooks.buttonTypePreviewFor("timezone", {
  entity: "America/New_York (GMT-5)",
  type: "timezone",
  options: "",
}, {
  clockFormat: "24h",
});
assert(timezonePreview.labelHtml.includes("New York"), "world clock preview uses the city label");
assert(timezonePreview.labelHtml.includes("mdi-map-clock"), "world clock preview uses the map clock badge");

const wideTimezonePreview = hooks.buttonTypePreviewFor("timezone", {
  entity: "America/New_York (GMT-5)",
  type: "timezone",
  options: "large_numbers",
}, {
  cardSize: 3,
  clockFormat: "24h",
});
assert(wideTimezonePreview.iconHtml.includes("sp-sensor-preview-large"), "world clock wide preview supports large numbers");
assert.strictEqual(wideTimezonePreview.labelHtml, "", "world clock wide large preview hides the city label");

const timezoneSamples = [
  "UTC (GMT+0)",
  "Pacific/Kiritimati (GMT+14)",
  "America/New_York (GMT-5)",
  "Pacific/Honolulu (GMT-10)",
];
assert(timezoneSamples.some((timezone) => {
  const button = { entity: timezone, type: "timezone", options: "" };
  const clock24 = previewSensorValue(hooks.buttonTypePreviewFor("timezone", button, { clockFormat: "24h" }));
  const clock12 = previewSensorValue(hooks.buttonTypePreviewFor("timezone", button, { clockFormat: "12h" }));
  return clock24 !== clock12;
}), "world clock preview follows 12/24-hour formatting");

const weatherCurrentPreview = hooks.buttonTypePreviewFor("weather", {
  entity: "weather.forecast_home",
  type: "weather",
  precision: "",
});
assert(weatherCurrentPreview.iconHtml.includes("mdi-weather-cloudy"), "weather current preview uses the current-condition icon");
assert(weatherCurrentPreview.labelHtml.includes("mdi-weather-cloudy"), "weather current preview uses the weather badge");

const weatherForecastPreview = hooks.buttonTypePreviewFor("weather", {
  entity: "weather.forecast_home",
  label: "Garden",
  type: "weather",
  precision: "today",
  options: "large_numbers",
}, {
  cardSize: 4,
  temperatureUnit: "\u00b0F",
});
assert(weatherForecastPreview.iconHtml.includes("sp-forecast-value"), "weather forecast preview uses forecast value styling");
assert(weatherForecastPreview.iconHtml.includes("sp-sensor-preview-large"), "weather forecast preview supports large numbers");
assert(weatherForecastPreview.iconHtml.includes("\u00b0F"), "weather forecast preview uses the selected temperature unit");
assert(weatherForecastPreview.labelHtml.includes("Garden"), "weather forecast preview uses the custom label");

const sensorNumericPreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "sensor.office_temperature",
  label: "Office",
  unit: "\u00b0C",
  type: "sensor",
  precision: "1",
  options: "large_numbers",
}, { cardSize: 4 });
assert(sensorNumericPreview.iconHtml.includes("sp-sensor-preview-large"), "sensor numeric preview supports large numbers");
assert(sensorNumericPreview.labelHtml.includes("mdi-gauge"), "sensor numeric preview uses the gauge badge");
assert(sensorNumericPreview.iconHtml.includes("\u00b0C"), "sensor numeric preview includes the unit");

const wideSensorNumericPreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "sensor.office_temperature",
  label: "Office",
  unit: "\u00b0C",
  type: "sensor",
  precision: "1",
  options: "large_numbers",
}, { cardSize: 3 });
assert(!wideSensorNumericPreview.iconHtml.includes("sp-sensor-preview-large"), "sensor wide preview keeps large numbers limited to 2x2");

const sensorTextPreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "sensor.washer_state",
  icon: "Washing Machine",
  type: "sensor",
  precision: "text",
});
assert(sensorTextPreview.iconHtml.includes("mdi-washing-machine"), "sensor text preview uses the selected icon");
assert(sensorTextPreview.labelHtml.includes("mdi-format-text"), "sensor text preview uses the text badge");

const todoPreview = hooks.buttonTypePreviewFor("todo", {
  entity: "todo.shopping",
  label: "Shopping",
  icon: "Check",
  type: "todo",
});
assert(todoPreview.iconHtml.includes("sp-sensor-value"), "todo preview shows an item count");
assert(todoPreview.labelHtml.includes("Shopping"), "todo preview uses the configured label");
assert(todoPreview.labelHtml.includes("mdi-check"), "todo preview uses the check badge");
const todoLargePreview = hooks.buttonTypePreviewFor("todo", {
  entity: "todo.shopping",
  label: "Shopping",
  icon: "Check",
  type: "todo",
  options: "large_numbers",
}, { cardSize: 4 });
assert(todoLargePreview.iconHtml.includes("sp-sensor-preview-large"), "todo item count preview supports large numbers");
const todoIconPreview = hooks.buttonTypePreviewFor("todo", {
  entity: "todo.shopping",
  label: "Shopping",
  icon: "Check",
  type: "todo",
  options: "count_display=icon",
});
assert(todoIconPreview.iconHtml.includes("mdi-check"), "todo preview can show the icon instead of the item count");
assert(!todoIconPreview.iconHtml.includes("sp-sensor-value"), "todo icon preview hides the item count");
const todoLabelCountPreview = hooks.buttonTypePreviewFor("todo", {
  entity: "todo.shopping",
  label: "Shopping",
  icon: "Check",
  type: "todo",
  options: "label_display=count",
});
assert(todoLabelCountPreview.labelHtml.includes(">3<"), "todo preview can show item count in the label");
assert(!todoLabelCountPreview.labelHtml.includes("items"), "todo preview label count does not include item text");
assert(!todoLabelCountPreview.labelHtml.includes("Shopping"), "todo count-label preview hides the static label");
assert.deepStrictEqual(Array.from(hooks.buttonTypeRuntimeSpec("todo").domains), ["todo"], "todo entity field is limited to todo entities");

const legacyForecastPreview = hooks.buttonTypePreviewFor("weather_forecast", {
  entity: "weather.forecast_home",
  type: "weather_forecast",
  precision: "tomorrow",
}, { temperatureUnit: "\u00b0C" });
assert(legacyForecastPreview.iconHtml.includes("sp-forecast-value"), "legacy forecast preview uses forecast styling");
assert(legacyForecastPreview.labelHtml.includes("Temperatures Tomorrow"), "legacy forecast preview keeps its label");

const doorPreview = hooks.buttonTypePreviewFor("door_window", {
  label: "Patio Door",
  icon: "Door",
  icon_on: "Door Open",
  sensor: "binary_sensor.patio_door",
  type: "door_window",
  precision: "door",
});
assert(doorPreview.iconHtml.includes("mdi-door"), "door/window door preview uses the closed door icon");
assert(doorPreview.labelHtml.includes("mdi-door"), "door/window door preview uses the door badge");

const windowPreview = hooks.buttonTypePreviewFor("door_window", {
  label: "Kitchen Window",
  icon: "Window Closed",
  icon_on: "Window Open",
  sensor: "binary_sensor.kitchen_window",
  type: "door_window",
  precision: "window",
});
assert(windowPreview.labelHtml.includes("mdi-window-closed"), "door/window window preview uses the window badge");

const actionPreview = hooks.buttonTypePreviewFor("action", {
  entity: "scene.movie_mode",
  label: "Movie Mode",
  icon: "Flash",
  sensor: "scene.turn_on",
  type: "action",
});
assert(actionPreview.iconHtml.includes("mdi-flash"), "action preview uses the selected action icon");
assert(actionPreview.labelHtml.includes("mdi-flash"), "action preview uses the action badge");
const actionLargePreview = hooks.buttonTypePreviewFor("action", {
  entity: "script.kitchen_lights",
  label: "Kitchen Lights",
  icon: "Flash",
  sensor: "script.turn_on",
  type: "action",
  options: "state_entity=sensor.kitchen_power,state_unit=W,state_precision=1,large_numbers",
}, { cardSize: 4 });
assert(actionLargePreview.iconHtml.includes("sp-sensor-preview-large"), "action numeric state preview supports large numbers");

const actionOptionPreview = hooks.buttonTypePreviewFor("action", {
  entity: "select.wled_preset",
  label: "Preset",
  sensor: "input_select.select_option",
  type: "action",
});
assert(actionOptionPreview.iconHtml.includes("Option"), "action option-select preview uses option text");
assert(actionOptionPreview.labelHtml.includes("mdi-chevron-down"), "action option-select preview uses the dropdown badge");

const alarmActionPreview = hooks.buttonTypePreviewFor("alarm_action", {
  entity: "alarm_control_panel.house",
  label: "Arm Away",
  icon: "Shield Lock",
  sensor: "away",
  type: "alarm_action",
});
assert(alarmActionPreview.iconHtml.includes("mdi-shield-lock"), "alarm action preview uses its action icon");

const fanSpeedPreview = hooks.buttonTypePreviewFor("fan_speed", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan Speed 2",
  type: "fan_speed",
});
assert(fanSpeedPreview.iconHtml.includes("sp-slider-preview"), "fan speed preview keeps the slider preview");
assert(fanSpeedPreview.labelHtml.includes("mdi-fan-speed-2"), "fan speed preview uses the speed badge");

const fanSwitchPreview = hooks.buttonTypePreviewFor("fan_switch", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan Off",
  icon_on: "Fan",
  type: "fan_switch",
});
assert(fanSwitchPreview.labelHtml.includes("mdi-fan"), "fan switch preview uses the fan badge");

const switchPreview = hooks.buttonTypePreviewFor("", {
  entity: "switch.printer",
  label: "Printer",
  icon: "Power Plug",
  icon_on: "Power",
});
assert(switchPreview.labelHtml.includes("mdi-toggle-switch-variant-off"), "switch preview uses the switch badge");

const switchNumericPreview = hooks.buttonTypePreviewFor("", {
  entity: "switch.washing_machine",
  label: "Washer",
  sensor: "sensor.washer_power",
  unit: "W",
});
assert(switchNumericPreview.labelHtml.includes("mdi-gauge"), "switch numeric active display preview uses the gauge badge");
const switchLargePreview = hooks.buttonTypePreviewFor("", {
  entity: "switch.washing_machine",
  label: "Washer",
  sensor: "sensor.washer_power",
  unit: "W",
  options: "large_numbers",
}, { cardSize: 4 });
assert(switchLargePreview.iconHtml.includes("sp-sensor-preview-large"), "switch numeric active display preview supports large numbers");

const switchTextPreview = hooks.buttonTypePreviewFor("", {
  entity: "switch.washing_machine",
  label: "Washer",
  sensor: "sensor.washer_state",
  precision: "text",
});
assert(switchTextPreview.labelHtml.includes("mdi-format-text"), "switch text active display preview uses the text badge");

const sliderPreview = hooks.buttonTypePreviewFor("slider", {
  entity: "light.strip",
  label: "Strip",
  icon: "Lightbulb",
  icon_on: "Lightbulb On",
  type: "slider",
});
assert(sliderPreview.iconHtml.includes("sp-slider-preview"), "slider preview uses the slider track");
assert(sliderPreview.labelHtml.includes("mdi-tune-vertical-variant"), "slider preview uses the tune badge");

const lightBrightnessPreview = hooks.buttonTypePreviewFor("light_brightness", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Lightbulb Outline",
  icon_on: "Lightbulb",
  type: "light_brightness",
});
assert(lightBrightnessPreview.iconHtml.includes("sp-slider-preview"), "light brightness preview uses the slider track");
assert(lightBrightnessPreview.labelHtml.includes("mdi-tune-vertical-variant"), "light brightness preview uses the tune badge");

const lightSwitchPreview = hooks.buttonTypePreviewFor("light_switch", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Lightbulb Outline",
  icon_on: "Lightbulb",
  type: "light_switch",
});
assert(lightSwitchPreview.labelHtml.includes("mdi-lightbulb"), "light switch preview uses the lightbulb badge");

const lightTemperaturePreview = hooks.buttonTypePreviewFor("light_temperature", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Lightbulb",
  type: "light_temperature",
});
assert(lightTemperaturePreview.iconHtml.includes("sp-slider-preview"), "light temperature preview uses the slider track");
assert(lightTemperaturePreview.labelHtml.includes("mdi-lightbulb"), "light temperature preview uses the lightbulb badge");

const coverSliderPreview = hooks.buttonTypePreviewFor("cover", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "tilt",
  type: "cover",
});
assert(coverSliderPreview.iconHtml.includes("sp-slider-preview"), "cover slider preview uses the slider track");
assert(coverSliderPreview.labelHtml.includes("mdi-blinds-horizontal"), "cover slider preview uses the cover badge");

const coverCommandPreview = hooks.buttonTypePreviewFor("cover", {
  entity: "cover.office_blind",
  label: "Open",
  icon: "Blinds Open",
  icon_on: "Auto",
  sensor: "open",
  type: "cover",
});
assert(!coverCommandPreview.iconHtml.includes("sp-slider-preview"), "cover command preview uses icon-only layout");
assert(coverCommandPreview.iconHtml.includes("mdi-blinds-open"), "cover command preview uses the command icon");
assert(coverCommandPreview.labelHtml.includes("mdi-blinds-horizontal"), "cover command preview uses the cover badge");

const coverSetPositionPreview = hooks.buttonTypePreviewFor("cover", {
  entity: "cover.office_blind",
  label: "50%",
  icon: "Blinds",
  icon_on: "Auto",
  sensor: "set_position",
  unit: "50",
  type: "cover",
});
assert(!coverSetPositionPreview.iconHtml.includes("sp-slider-preview"), "cover set-position preview uses command layout");
assert(coverSetPositionPreview.labelHtml.includes("mdi-blinds-horizontal"), "cover set-position preview uses the cover badge");

const lockTogglePreview = hooks.buttonTypePreviewFor("lock", {
  entity: "lock.front_door",
  label: "Front Door",
  icon: "Lock",
  icon_on: "Lock Open",
  type: "lock",
});
assert(lockTogglePreview.iconHtml.includes("mdi-lock"), "lock toggle preview uses the locked icon");
assert(lockTogglePreview.labelHtml.includes("mdi-lock"), "lock toggle preview uses the lock badge");

const lockCommandPreview = hooks.buttonTypePreviewFor("lock", {
  entity: "lock.front_door",
  label: "Lock",
  icon: "Lock",
  icon_on: "Auto",
  sensor: "lock",
  type: "lock",
});
assert(lockCommandPreview.iconHtml.includes("mdi-lock"), "lock command preview uses the lock icon");
assert(lockCommandPreview.labelHtml.includes("Lock"), "lock command preview uses the command label");

const unlockCommandPreview = hooks.buttonTypePreviewFor("lock", {
  entity: "lock.front_door",
  label: "Unlock",
  icon: "Lock Open",
  icon_on: "Auto",
  sensor: "unlock",
  type: "lock",
});
assert(unlockCommandPreview.iconHtml.includes("mdi-lock-open"), "unlock command preview uses the unlock icon");

const garageStatusPreview = hooks.buttonTypePreviewFor("garage", {
  entity: "cover.garage",
  label: "Garage Door",
  icon: "Garage",
  icon_on: "Garage Open",
  type: "garage",
  options: "label_display=status",
});
assert(garageStatusPreview.iconHtml.includes("mdi-garage"), "garage status preview uses the garage icon");
assert(garageStatusPreview.labelHtml.includes("Closed"), "garage status preview uses the status label");
assert(garageStatusPreview.labelHtml.includes("mdi-garage"), "garage status preview uses the garage badge");

const garageOpenPreview = hooks.buttonTypePreviewFor("garage", {
  entity: "cover.garage",
  label: "Open",
  icon: "Garage Open",
  icon_on: "Auto",
  sensor: "open",
  type: "garage",
});
assert(garageOpenPreview.iconHtml.includes("mdi-garage-open"), "garage open command preview uses the open icon");
assert(garageOpenPreview.labelHtml.includes("mdi-garage"), "garage open command preview uses the garage badge");

const garageClosePreview = hooks.buttonTypePreviewFor("garage", {
  entity: "cover.garage",
  label: "Close",
  icon: "Garage",
  icon_on: "Auto",
  sensor: "close",
  type: "garage",
});
assert(garageClosePreview.iconHtml.includes("mdi-garage"), "garage close command preview uses the closed icon");

const pushPreview = hooks.buttonTypePreviewFor("push", {
  label: "Doorbell",
  icon: "Gesture Tap",
  type: "push",
});
assert(pushPreview.iconHtml.includes("mdi-gesture-tap"), "push preview uses the trigger icon");
assert(pushPreview.labelHtml.includes("mdi-gesture-tap"), "push preview uses the trigger badge");

const internalSwitchPreview = hooks.buttonTypePreviewFor("internal", {
  entity: "relay_1",
  label: "Relay",
  icon: "Power Plug",
  icon_on: "Power",
  type: "internal",
});
assert(internalSwitchPreview.iconHtml.includes("mdi-power-plug"), "internal switch preview uses the configured icon");
assert(internalSwitchPreview.labelHtml.includes("mdi-power-plug"), "internal switch preview uses the switch badge");

const internalPushPreview = hooks.buttonTypePreviewFor("internal", {
  entity: "relay_2",
  label: "Bell",
  icon: "Gesture Tap",
  icon_on: "Auto",
  sensor: "push",
  type: "internal",
});
assert(internalPushPreview.iconHtml.includes("mdi-gesture-tap"), "internal push preview uses the push icon");
assert(internalPushPreview.labelHtml.includes("mdi-gesture-tap"), "internal push preview uses the push badge");

const subpagePlainPreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Lighting",
  icon: "Lightbulb",
  type: "subpage",
});
assert(subpagePlainPreview.labelHtml.includes("mdi-chevron-right"), "plain subpage preview uses the chevron badge");

const subpageIconPreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "indicator",
  type: "subpage",
});
assert(subpageIconPreview.iconHtml.includes("mdi-blinds"), "subpage icon-state preview uses the configured icon");
assert(subpageIconPreview.labelHtml.includes("mdi-chevron-right"), "subpage icon-state preview uses the chevron badge");

const subpageNumericPreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Open Windows",
  icon: "Window Closed",
  sensor: "sensor.open_windows",
  unit: "%",
  type: "subpage",
});
assert(subpageNumericPreview.iconHtml.includes("sp-sensor-preview"), "subpage numeric preview uses the shared number preview");
assert(subpageNumericPreview.labelHtml.includes("mdi-chevron-right"), "subpage numeric preview uses the chevron badge");
const subpageLargePreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Open Windows",
  icon: "Window Closed",
  sensor: "sensor.open_windows",
  unit: "%",
  type: "subpage",
  options: "large_numbers",
}, { cardSize: 4 });
assert(subpageLargePreview.iconHtml.includes("sp-sensor-preview-large"), "subpage numeric preview supports large numbers");

const subpageTextPreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Washer",
  icon: "Thermometer",
  sensor: "sensor.washer_state",
  type: "subpage",
  precision: "text",
});
assert(subpageTextPreview.iconHtml.includes("mdi-thermometer"), "subpage text preview uses the configured icon");
assert(subpageTextPreview.labelHtml.includes("State"), "subpage text preview keeps the State label");
assert(subpageTextPreview.labelHtml.includes("mdi-chevron-right"), "subpage text preview uses the chevron badge");

const mediaVolumePreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.kitchen",
  label: "Kitchen",
  sensor: "volume",
  type: "media",
});
assert(mediaVolumePreview.iconHtml.includes("sp-sensor-preview"), "media volume preview uses the shared number preview");
assert(mediaVolumePreview.labelHtml.includes("mdi-speaker"), "media volume preview uses the speaker badge");
const mediaVolumeLargePreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.kitchen",
  label: "Kitchen",
  sensor: "volume",
  type: "media",
  options: "large_numbers",
}, { cardSize: 4 });
assert(mediaVolumeLargePreview.iconHtml.includes("sp-sensor-preview-large"), "media volume preview supports large numbers");

const mediaPositionLargePreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.office",
  label: "Office",
  sensor: "position",
  type: "media",
  options: "large_numbers",
}, { cardSize: 4 });
assert(mediaPositionLargePreview.iconHtml.includes("sp-sensor-preview-large"), "media position preview supports large numbers");

const mediaNowPlayingPreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.office",
  sensor: "now_playing",
  type: "media",
  precision: "progress",
});
assert(mediaNowPlayingPreview.iconHtml.includes("Midnight City"), "media now-playing preview keeps title text");
assert(mediaNowPlayingPreview.labelHtml.includes("sp-media-now-artist"), "media now-playing preview keeps artist styling");

const issue243Backup = {
  version: 1,
  device: "guition-esp32-p4-jc4880p443",
  exported_at: "2026-05-24T08:30:00.103Z",
  button_order: "2,5,1,6,3,4",
  button_on_color: "FF8C00",
  button_off_color: "313131",
  sensor_card_color: "212121",
  buttons: [
    { entity: "light.office", label: "Office", icon: "Lightbulb Outline", icon_on: "Lightbulb", sensor: "", unit: "", type: "light_brightness", precision: "", options: "" },
    { entity: "alarm_control_panel.alarmo", label: "Alarm", icon: "Auto", icon_on: "Auto", sensor: "indicator", unit: "", type: "subpage", precision: "", options: "" },
    { entity: "light.hallway", label: "Hallway", icon: "Lightbulb Outline", icon_on: "Lightbulb", sensor: "", unit: "", type: "light_brightness", precision: "", options: "" },
    { entity: "light.kitchen", label: "Kitchen", icon: "Lightbulb Outline", icon_on: "Lightbulb", sensor: "", unit: "", type: "light_brightness", precision: "", options: "" },
    { entity: "light.office_led", label: "Office LED", icon: "Lightbulb Outline", icon_on: "Lightbulb", sensor: "", unit: "", type: "light_brightness", precision: "", options: "" },
    { entity: "media_player.office", label: "Sonos", icon: "Music", icon_on: "Play Pause", sensor: "indicator", unit: "", type: "subpage", precision: "", options: "" },
  ],
  subpages: {
    2: "~B,,,1,,2|AA,alarm_control_panel.alarmo,Disarm,Shield Off,,disarm|AA,alarm_control_panel.alarmo,Arm Away,Shield Lock,,away,,,pin_arm=0|",
    6: "~B,1,3,2x|M,media_player.office,,,,play_pause,,state|M,media_player.office,,,,now_playing,,progress|M,media_player.office,Volume,,,volume||",
  },
  settings: { temperature_unit: "\u00b0C", timezone: "Australia/Sydney (GMT+10)", clock_format: "24h" },
  screen: { brightness_day: 70, brightness_night: 30 },
};
const issue243Plan = hooks.planBackupImport(issue243Backup, {
  device: "guition-esp32-p4-jc4880p443",
  slots: 6,
});
assert.deepStrictEqual(
  plain(issue243Plan.buttons.map((button) => button.type)),
  ["light_brightness", "subpage", "light_brightness", "light_brightness", "light_brightness", "subpage"],
  "issue #243 import keeps brightness cards as brightness cards"
);
assert.deepStrictEqual(
  plain(issue243Plan.buttons.filter((button) => button.type === "light_brightness").map((button) => button.entity)),
  ["light.office", "light.hallway", "light.kitchen", "light.office_led"],
  "issue #243 import keeps all brightness light entities"
);
assert.strictEqual(issue243Plan.button_order, "2,5,1,6,3,4", "issue #243 import keeps the JC4880 layout");
assert.strictEqual(hooks.serializeButtonConfig(issue243Plan.buttons[0]), "light.office;Office;Lightbulb Outline;Lightbulb;;;light_brightness");
assert.deepStrictEqual(plain(Object.keys(issue243Plan.subpages).sort()), ["2", "6"], "issue #243 import keeps both subpages");
assert.deepStrictEqual(
  plain(issue243Plan.subpages["6"].buttons.filter((button) => button.entity).map((button) => button.type)),
  ["media", "media", "media"],
  "issue #243 media subpage survives import"
);

assert.strictEqual(hooks.normalizeScreensaverAction("Screen Dimmed"), "dim");
assert.strictEqual(hooks.previewHtmlValue({ labelHtml: "" }, "labelHtml", "fallback"), "");
assert.strictEqual(hooks.previewHtmlValue({}, "labelHtml", "fallback"), "fallback");
const backOnlySubpage = hooks.parseSubpageConfig(",,,,B");
hooks.buildSubpageGrid(backOnlySubpage);
assert.deepStrictEqual(plain(backOnlySubpage.buttons), []);
assert.strictEqual(backOnlySubpage.grid[4], -2);
assert.deepStrictEqual(plain(hooks.serializeSubpageGrid(backOnlySubpage)), ["", "", "", "", "B"]);
assert.strictEqual(hooks.serializeSubpageConfig(backOnlySubpage), ",,,,B");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 24), "wifi-strength-1");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 25), "wifi-strength-2");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 50), "wifi-strength-3");
assert.strictEqual(hooks.networkPreviewIconSlug("wifi", 75), "wifi-strength-4");
assert.strictEqual(hooks.networkPreviewIconSlug("ethernet", 0), "ethernet");
assert.strictEqual(hooks.displayFirmwareVersion("v1.11.1"), "v1.11.1");
assert.strictEqual(hooks.displayFirmwareVersion("dev"), "Dev build");
assert.strictEqual(hooks.displayFirmwareVersion("0.0.0"), "Dev build");
assert.strictEqual(hooks.displayFirmwareVersion("main"), "Dev build");
assert.strictEqual(hooks.displayFirmwareVersion(""), "Version unknown");
assert.strictEqual(hooks.firmwareVersionFromMetadata({ firmware_version: "v1.12.0" }), "v1.12.0");
assert.strictEqual(hooks.firmwareVersionFromMetadata({ project_version: "v1.12.1" }), "v1.12.1");
assert.strictEqual(hooks.firmwareVersionFromMetadata({ version: "dev" }), "dev");
const publicManifest = {
  version: "v1.12.0",
  builds: [{
    chipFamily: "ESP32-P4",
    ota: {
      path: "guition-esp32-p4-jc1060p470.ota.bin",
      md5: "0123456789abcdef0123456789abcdef",
      release_url: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.12.0",
    },
  }],
};
assert.deepStrictEqual(plain(hooks.firmwareInfoFromPublicManifest(publicManifest)), {
  latest_version: "v1.12.0",
  release_url: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.12.0",
  ota_url: "https://jtenniswood.github.io/espcontrol/firmware/guition-esp32-p4-jc1060p470/guition-esp32-p4-jc1060p470.ota.bin",
  ota_filename: "guition-esp32-p4-jc1060p470.ota.bin",
  ota_md5: "0123456789abcdef0123456789abcdef",
});
assert.strictEqual(
  hooks.firmwareInfoFromPublicManifest({
    version: "v1.12.0",
    builds: [{ chipFamily: "ESP32-S3", ota: { path: "wrong-device.ota.bin" } }],
  }),
  null
);
assert.strictEqual(hooks.firmwareVersionLabelFor("", true), "Checking version...");
assert.strictEqual(hooks.firmwareVersionLabelFor("", false), "Version unknown");
assert.deepStrictEqual(plain(hooks.entityDetailPaths("text_sensor", hooks.entityLookupNames("firmware_version"))), [
  "/text_sensor/Firmware%3A%20Version?detail=all",
  "/text_sensor/firmware__version?detail=all",
  "/text_sensor/firmware_version?detail=all",
  "/text_sensor/firmware_version_sensor?detail=all",
]);
assert.deepStrictEqual(plain(hooks.entityLookupNames("firmware_version")), [
  "Firmware: Version",
  "firmware__version",
  "firmware_version",
  "firmware_version_sensor",
]);
assert.strictEqual(hooks.firmwareUpdateControlsVisibleFor("wifi", true), true);
assert.strictEqual(hooks.firmwareUpdateControlsVisibleFor("wifi", false), false);
assert.strictEqual(hooks.firmwareUpdateControlsVisibleFor("ethernet", true), true);
assert.strictEqual(
  hooks.firmwareVersionAfterUpdateInfo("Dev", { state: "NO UPDATE", latest_version: "v1.11.1" }).version,
  "v1.11.1"
);
assert.strictEqual(
  hooks.firmwareVersionAfterUpdateInfo("Dev", { state: "UPDATE AVAILABLE", latest_version: "v1.11.1" }).version,
  "Dev build"
);
assert.strictEqual(
  hooks.firmwareVersionAfterUpdateInfo("v1.10.0", { state: "NO UPDATE", latest_version: "v1.11.1" }).version,
  "v1.10.0"
);
assert.deepStrictEqual(plain(hooks.firmwareStateAfterPublicManifest("Dev", publicManifest)), {
  version: "Dev build",
  latest: "v1.12.0",
  updateState: "",
  releaseUrl: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.12.0",
  updateAvailable: false,
  installAvailable: true,
});
assert.strictEqual(
  hooks.firmwareStateAfterPublicManifest("v1.12.0", publicManifest).installAvailable,
  false
);

console.log("Web UI smoke tests passed.");
