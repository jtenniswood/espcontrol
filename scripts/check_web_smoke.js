#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { freshWebOutputDir, loadBuiltWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "entry.ts");
const DEVICE_MANIFEST = path.join(ROOT, "devices", "manifest.json");
const WEB_OUTPUT_DIR = path.join(ROOT, "docs", "public", "webserver");
const ALL_ROTATIONS = ["0", "90", "180", "270"];
const REQUIRED_HOOK_GROUPS = ["config", "preview", "backup", "settings"];

function createWebSandbox() {
  const domEvents = [];
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    URL,
    location: { href: "http://espcontrol.test/" },
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
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: SOURCE });
  assertRequiredHookGroups(sandbox.__ESPCONTROL_TEST_HOOKS__.groups);
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

function assertRequiredHookGroups(groups, prefix = "web test hooks") {
  assert(groups, `${prefix} must expose grouped hook registrations`);
  for (const group of REQUIRED_HOOK_GROUPS) {
    assert(groups[group], `${prefix} must include the ${group} hook group`);
  }
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

function assertGeneratedConfigValue(slug, generated, key, value) {
  assert(
    generated.includes(`${key}:${JSON.stringify(value)}`),
    `${slug}: generated web UI must include ${key} ${JSON.stringify(value)}`
  );
}

const hooks = loadHooks();
assert(hooks, "web test hooks were not exported");
assert.strictEqual(
  hooks.backupExportFileName(new Date(2026, 5, 9)),
  "espcontrol-7-inch-2026-06-09.json",
  "backup export filename includes screen size and date"
);
assert.deepStrictEqual(Array.from(hooks.buttonTypesMissingCardMetadata()), [], "all registered card types define card metadata");
assert.deepStrictEqual(
  plain(hooks.cardSizeMenuOptions({ type: "image" })).slice(-2),
  [
    { size: 8, label: "Max wide (3x2)" },
    { size: 9, label: "Max tall (2x3)" },
  ],
  "camera card size menu exposes the two max shapes"
);
assert(
  !plain(hooks.cardSizeMenuOptions({ type: "sensor" })).some((option) => option.size === 8 || option.size === 9),
  "non-camera card size menus do not expose max shapes"
);
assert(
  Array.from(hooks.entityLookupNames("screen_saver_hide_cover_art_external_input")).includes("screen_saver__hide_cover_art_on_external_input"),
  "cover art external-input post aliases include the full generated object id"
);
assert(
  Array.from(hooks.entityLookupNames("screen_saver_hide_cover_art_external_input")).includes("screen_saver__hide_for_external_sources"),
  "cover art external-input post aliases include the legacy external-sources object id"
);
assert.deepStrictEqual(Array.from(hooks.coverArtHideExternalInputPostUrls(false)), [
  "/switch/screen_saver__hide_cover_art_on_external_input/turn_off",
  "/switch/screen_saver_hide_cover_art_on_external_input/turn_off",
  "/switch/hide_cover_art_on_external_input/turn_off",
  "/switch/cover_art_hide_external_input/turn_off",
  "/switch/screen_saver__hide_for_external_sources/turn_off",
  "/switch/Screen%20Saver%3A%20Hide%20for%20external%20sources/turn_off",
], "cover art external-input posts include all firmware object id aliases");
assert.deepStrictEqual(Array.from(hooks.coverArtDelayPostUrls(30)), [
  "/number/screen_saver__cover_art_delay/set?value=30",
  "/number/screen_saver_cover_art_delay/set?value=30",
  "/number/cover_art_delay/set?value=30",
  "/number/Screen%20Saver%3A%20Cover%20Art%20Delay/set?value=30",
], "cover art delay posts include all firmware object id aliases");
assert.deepStrictEqual(Array.from(hooks.coverArtDelayPostUrls(0)), [
  "/number/screen_saver__cover_art_delay/set?value=3",
  "/number/screen_saver_cover_art_delay/set?value=3",
  "/number/cover_art_delay/set?value=3",
  "/number/Screen%20Saver%3A%20Cover%20Art%20Delay/set?value=3",
], "legacy immediate cover art delay posts as three seconds");
assert.strictEqual(hooks.normalizeCoverArtDelay(0), 3, "cover art delay UI normalizes legacy immediate values");
assert(
  Array.from(hooks.entityLookupNames("screen_saver_track_overlay_duration")).includes("screen_saver__show_track_overlay"),
  "cover art track-overlay post aliases include the legacy show-track-overlay object id"
);
assert.deepStrictEqual(Array.from(hooks.coverArtTrackOverlayDurationPostUrls(15)), [
  "/number/screen_saver__track_overlay_duration/set?value=15",
  "/number/screen_saver_track_overlay_duration/set?value=15",
  "/number/track_overlay_duration/set?value=15",
  "/number/screen_saver__show_track_overlay/set?value=15",
  "/number/Screen%20Saver%3A%20Show%20Track%20Overlay/set?value=15",
], "cover art track-overlay posts include all firmware object id aliases");
assert.deepStrictEqual(Array.from(hooks.homeAssistantArtworkPortPostUrls(80)), [
  "/number/home_assistant_artwork_port/set?value=80",
  "/number/Home%20Assistant%20Artwork%20Port/set?value=80",
], "Home Assistant artwork port posts include object id and entity name fallbacks");
assert.deepStrictEqual(Array.from(hooks.voiceServicesPostUrls(true)), [
  "/switch/voice_services/turn_on",
  "/switch/voice_services_enabled/turn_on",
  "/switch/Voice%20Services/turn_on",
], "voice services posts include object id aliases and entity name fallback");
assert.strictEqual(hooks.clockBarVisibleInPreviewFor(true, "off"), true, "clock bar preview is visible when enabled");
assert.strictEqual(hooks.clockBarVisibleInPreviewFor(true, "dim"), true, "clock bar preview stays visible for dimmed screen saver");
assert.strictEqual(hooks.clockBarVisibleInPreviewFor(true, "clock"), true, "clock bar preview stays visible when clock screen saver is configured");
assert.strictEqual(hooks.clockBarVisibleInPreviewFor(false, "off"), false, "clock bar preview is hidden when disabled");
assert.deepStrictEqual(plain(hooks.firmwareFailureStatusFor("Could not download firmware file (404).")), {
  error: "Firmware update failed: Could not download firmware file (404).",
  updateState: "",
  installStatus: "",
}, "firmware update failures leave a visible status reason");

const manifest = JSON.parse(fs.readFileSync(DEVICE_MANIFEST, "utf8"));
const freshOutput = freshWebOutputDir();
const webOutput = path.join(freshOutput, "www.js");
const generated = fs.readFileSync(webOutput, "utf8");

const hostedSandbox = createWebSandbox();
hostedSandbox.document.currentScript = {
  getAttribute() { return "/webserver/www.js?device=guition-esp32-s3-4848s040"; },
};
vm.createContext(hostedSandbox);
vm.runInContext(generated, hostedSandbox, { filename: webOutput });
assert.strictEqual(
  hostedSandbox.__ESPCONTROL_TEST_HOOKS__.config.imageSlotCapacity(),
  manifest.devices["guition-esp32-s3-4848s040"].capabilities.imageSlots,
  "shared hosted bundle selects the device profile from its script URL",
);

for (const [slug, device] of Object.entries(manifest.devices || {})) {
  assertGeneratedConfigValue(slug, generated, "slots", device.slots);
  assertGeneratedConfigValue(slug, generated, "cols", device.layout.cols);
  assertGeneratedConfigValue(slug, generated, "rows", device.layout.rows);
  assertGeneratedConfigValue(slug, generated, "screenSize", device.public.screenSize);
  assertGeneratedConfigValue(slug, generated, "slots", device.slots);
  assertGeneratedConfigValue(slug, generated, "cols", device.layout.cols);
  assertGeneratedConfigValue(slug, generated, "rows", device.layout.rows);
  assertGeneratedConfigValue(slug, generated, "screenSize", device.public.screenSize);
  const sandbox = createWebSandbox();
  sandbox.__ESPCONTROL_DEVICE_PROFILE__ = slug;
  vm.createContext(sandbox);
  vm.runInContext(generated, sandbox, { filename: webOutput });
  assert(
    sandbox.__ESPCONTROL_TEST_HOOKS__.config,
    `${slug}: generated web UI must export the same test hooks used by local checks`
  );
  assertRequiredHookGroups(sandbox.__ESPCONTROL_TEST_HOOKS__.groups, `${slug}: generated web UI`);
  const generatedHooks = sandbox.__ESPCONTROL_TEST_HOOKS__.config;
  const expectedScreenSize = String(device.public.screenSize)
    .toLowerCase()
    .replace(/\binches\b/g, "inch")
    .replace(/\bin\b/g, "inch")
    .replace(/[^a-z0-9.]+/g, "-")
    .replace(/^-+|-+$/g, "");
  assert.strictEqual(
    generatedHooks.backupExportFileName(new Date(2026, 5, 9)),
    `espcontrol-${expectedScreenSize}-2026-06-09.json`,
    `${slug}: backup export filename includes screen size and date`
  );
  const generatedTimezones = Array.from(generatedHooks.defaultTimezoneOptions());
  assert(
    generatedTimezones.includes("UTC (GMT+0)") && generatedTimezones.includes("Europe/London (GMT+0)"),
    `${slug}: generated web UI must include fallback timezone choices`
  );
  assert.strictEqual(
    generatedTimezones[0],
    "Pacific/Midway (GMT-11)",
    `${slug}: Auto timezone must not shift restored timezone indices on OTA`
  );
  assert.strictEqual(
    generatedTimezones[generatedTimezones.length - 1],
    "Auto (Home Assistant)",
    `${slug}: Auto timezone remains available as the new-install default option`
  );
  assert(
    Array.from(generatedHooks.timezoneOptionsWithFallback([], "Custom/Zone (GMT+0)")).includes("Custom/Zone (GMT+0)"),
    `${slug}: timezone fallback must preserve the selected value`
  );
  assert(
    !Array.from(generatedHooks.timezoneOptionsWithFallback(["UTC (GMT+0)"], "Auto (Home Assistant)")).includes("Auto (Home Assistant)"),
    `${slug}: timezone fallback must not add Auto when firmware options do not advertise it`
  );
  assert(
    Array.from(generatedHooks.timezoneOptionsWithFallback(["UTC (GMT+0)"], "Auto (Home Assistant)", true)).includes("Auto (Home Assistant)"),
    `${slug}: timezone fallback must preserve restored Auto timezone selections`
  );
  if (((device.web || {}).disabledCardTypes || []).includes("weather_forecast")) {
    assert.deepStrictEqual(
      Array.from(generatedHooks.weatherModeOptionValues()),
      [""],
      `${slug}: generated web UI must hide weather forecast modes when forecast cards are disabled`
    );
    assert.strictEqual(
      generatedHooks.normalizeWeatherCardMode("today"),
      "",
      `${slug}: generated web UI must normalize forecast weather cards back to current conditions`
    );
    assert.strictEqual(
      generatedHooks.weatherCardIsForecastMode({ precision: "today" }),
      false,
      `${slug}: generated web UI must not preview disabled weather forecast modes`
    );
  }
  const disabledCardTypes = new Set((device.web || {}).disabledCardTypes || []);
  const imageCardsDisabled = device.capabilities.imageSlots === 0 || disabledCardTypes.has("image");
  const mediaCoverArtDisabled = device.capabilities.imageSlots === 0 || disabledCardTypes.has("media_cover_art");
  if (mediaCoverArtDisabled) {
    assert(
      !Array.from(generatedHooks.mediaModeOptionValues()).includes("cover_art"),
      `${slug}: generated web UI must hide Media Cover Art from the Media card type list`
    );
    assert.strictEqual(
      generatedHooks.mediaEditorMode("cover_art"),
      "play_pause",
      `${slug}: generated web UI must normalize unsupported Media Cover Art cards`
    );
    assert.strictEqual(
      generatedHooks.buttonTypeVisibleInPickerFor("media_cover_art", false),
      false,
      `${slug}: generated web UI must hide Media Cover Art from the main card picker`
    );
  }
  assert.strictEqual(
    generatedHooks.buttonTypeVisibleInPickerFor("image", false),
    !imageCardsDisabled,
    `${slug}: generated web UI must match the Camera Card capability`
  );
  if (slug === "guition-esp32-s3-4848s040") {
    assert.strictEqual(
      generatedHooks.imageSlotCapacityMessage(),
      "This display supports up to 1 Camera Card across the main page and subpages.",
      `${slug}: generated web UI must describe the camera-only limit accurately`
    );
    assert.strictEqual(
      generatedHooks.imageCardCandidateAllowedForTest(
        { grid: [1], buttons: [{ type: "image" }], subpages: {} },
        { isSub: false, slot: 2, button: { type: "image" } }
      ),
      false,
      `${slug}: generated web UI must reject a second Camera Card`
    );
  }
  assert(
    sandbox.__domEvents.some((event) => event.type === "DOMContentLoaded" && typeof event.listener === "function"),
    `${slug}: generated web UI must register DOMContentLoaded startup wiring`
  );
}

for (const [slug, device] of Object.entries(manifest.devices || {})) {
  if (!device.rotation || !device.rotation.enabled) continue;
  const featureConfig = generated.match(/features:\{[^}]*\}/)?.[0] || "";
  assert(
    /features:\{[^}]*screenRotation:!0/.test(generated),
    `${slug}: generated web UI must expose screen rotation when rotation is enabled`
  );
  assert.deepStrictEqual(device.rotation.options, ALL_ROTATIONS, `${slug}: normal rotation options`);
  assert.strictEqual(device.rotation.experimentalOptions, undefined, `${slug}: no hidden rotation options`);
  assertGeneratedRotationOptions(slug, featureConfig, "screenRotationOptions", ALL_ROTATIONS);
  if (Object.prototype.hasOwnProperty.call(device.rotation, "displayOffset")) {
    assert(
      featureConfig.includes(`screenRotationDisplayOffset:${device.rotation.displayOffset}`),
      `${slug}: generated web UI must include screen rotation display offset ${device.rotation.displayOffset}`
    );
  }
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
const scriptConfirmationButton = {
  entity: "script.goodnight",
  label: "Goodnight",
  icon: "Script Text Play",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "confirm_on,confirm_message=Run bedtime?,confirm_yes=Run,confirm_no=Cancel",
};
const scriptConfirmationRoundTrip = hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptConfirmationButton));
assert.deepStrictEqual(plain(scriptConfirmationRoundTrip), scriptConfirmationButton);
assert.strictEqual(hooks.actionScriptConfirmationEnabled(scriptConfirmationRoundTrip), true);
assert.strictEqual(hooks.actionScriptConfirmationMessage(scriptConfirmationRoundTrip), "Run bedtime?");
assert.strictEqual(hooks.actionScriptConfirmationYesText(scriptConfirmationRoundTrip), "Run");
assert.strictEqual(hooks.actionScriptConfirmationNoText(scriptConfirmationRoundTrip), "Cancel");
const scriptConfirmationDefaultRoundTrip = hooks.parseButtonConfig(hooks.serializeButtonConfig({
  entity: "script.goodnight",
  label: "Goodnight",
  icon: "Script Text Play",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "confirm_on",
}));
assert.strictEqual(hooks.actionScriptConfirmationMessage(scriptConfirmationDefaultRoundTrip), "Run this script?");
const sceneWithStaleConfirmation = hooks.parseButtonConfig(hooks.serializeButtonConfig({
  entity: "scene.goodnight",
  label: "Goodnight",
  icon: "Movie Open",
  icon_on: "Auto",
  sensor: "scene.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "confirm_on,confirm_message=Run bedtime?",
}));
assert.strictEqual(sceneWithStaleConfirmation.options, "", "non-script action cards drop script confirmation options");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm", false), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm", true), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm_action", false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm_action", true), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("local_sensor", false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("local_sensor", true), false);
const infoOnlyPickerKeys = Array.from(hooks.buttonTypePickerKeysForInfoOnly(true));
assert(infoOnlyPickerKeys.includes("sensor"), "info-only displays can still add sensor cards");
assert(infoOnlyPickerKeys.includes("weather"), "info-only displays can still add weather cards");
assert(!infoOnlyPickerKeys.includes(""), "info-only displays hide switch controls");
assert(!infoOnlyPickerKeys.includes("subpage"), "info-only displays hide subpage cards");
assert(!infoOnlyPickerKeys.includes("media"), "info-only displays hide media controls");
const pickerOptions = Array.from(hooks.buttonTypePickerOptionsFor(false, null));
assert(pickerOptions.length > 8, "main card picker exposes the visible card choices");
for (const option of pickerOptions) {
  assert.strictEqual(typeof option.icon, "string", `${option.key}: picker option has an icon`);
  assert(option.icon.length > 0, `${option.key}: picker option icon is not empty`);
  assert.strictEqual(typeof option.description, "string", `${option.key}: picker option has a description`);
  assert(option.description.length > 0, `${option.key}: picker option description is not empty`);
}
const switchPickerOption = pickerOptions.find((option) => option.key === "");
assert(switchPickerOption, "switch card appears in the main card picker");
assert.strictEqual(switchPickerOption.icon, "toggle-switch", "switch picker option uses the expected icon");
assert(/Toggle lights/.test(switchPickerOption.description), "switch picker option includes concise help text");
assert(
  hooks.buttonTypePreviewFor("alarm", { label: "Alarm", icon: "Security", type: "alarm" }).iconHtml.includes("mdi-shield-off"),
  "alarm preview defaults to the status icon"
);
assert(
  !pickerOptions.some((option) => option.key === "media_control"),
  "media all controls is not shown as a top-level card picker shortcut"
);
assert(
  !pickerOptions.some((option) => option.label === "All Controls"),
  "all controls subtypes stay out of the top-level card picker"
);
assert(
  Array.from(hooks.mediaModeOptionValues()).includes("control_modal"),
  "media mode options include the media control modal subtype"
);
assert.strictEqual(
  Array.from(hooks.mediaModeOptionValues())[0],
  "control_modal",
  "all media controls appears first in the media mode list"
);
const mediaControlIconPreview = hooks.buttonTypePreviewFor("media", {
  label: "All Controls",
  icon: "Music",
  sensor: "control_modal",
  type: "media",
});
assert(
  mediaControlIconPreview.iconHtml.includes("mdi-music"),
  "all controls preview uses the selected custom icon"
);
const mediaControlConfig = hooks.parseButtonConfig(hooks.serializeButtonConfig({
  entity: "media_player.living_room",
  label: "Speaker",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "control_modal",
  unit: "",
  type: "media",
  precision: "",
  options: "label_display=status,number_display=volume",
}));
assert.strictEqual(
  mediaControlConfig.options,
  "number_display=volume",
  "media control parent card display options survive normalization"
);
assert.strictEqual(hooks.mediaLabelDisplayMode(mediaControlConfig), "status");
assert.strictEqual(hooks.mediaNumberDisplayMode(mediaControlConfig), "volume");
const mediaControlLabelConfig = hooks.parseButtonConfig(hooks.serializeButtonConfig({
  entity: "media_player.living_room",
  label: "Speaker",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "control_modal",
  unit: "",
  type: "media",
  precision: "",
  options: "label_display=label",
}));
assert.strictEqual(mediaControlLabelConfig.options, "label_display=label");
assert.strictEqual(hooks.mediaLabelDisplayMode(mediaControlLabelConfig), "label");
const mediaControlPreview = hooks.buttonTypePreviewFor("media", mediaControlConfig);
assert(
  mediaControlPreview.iconHtml.includes("sp-sensor-preview"),
  "media control volume display previews as a top-left number"
);
assert(
  mediaControlPreview.labelHtml.includes("Playing"),
  "media control status label preview uses player state text"
);
assert(
  hooks.buttonTypePreviewFor("alarm", { label: "Alarm", icon: "Alarm", type: "alarm", options: "icon_display=static" }).iconHtml.includes("mdi-bell-ring"),
  "alarm preview uses the selected Alarm icon"
);
assert(
  hooks.buttonTypePreviewFor("sensor", { type: "sensor", sensor: "local", entity: "room_temp", unit: "°C", precision: "1" }).iconHtml.includes("0.0"),
  "sensor preview renders the local sensor subtype"
);
assert.deepStrictEqual(Array.from(hooks.alarmCardTypeOptionValues(false)), ["control_panel", "away", "home", "night", "vacation", "disarm"]);
assert.deepStrictEqual(Array.from(hooks.alarmCardTypeOptionValues(true)), ["control_panel", "away", "home", "night", "vacation", "disarm"]);
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(hooks.parseButtonConfig(
  "alarm_control_panel.house;House;Security;Auto;;;alarm;;actions=away%7Cdisarm"
))), ["away", "disarm"]);
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(hooks.parseButtonConfig(
  "alarm_control_panel.house;House;Security;Auto;;;alarm;;actions=night%7Cvacation"
))), ["night", "vacation"]);
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(hooks.parseButtonConfig(
  "alarm_control_panel.house;House;Security;Auto;;;alarm;;actions=away%7Chome%7Cnight%7Cvacation%7Cdisarm"
))), ["away", "home", "night"]);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_speed", false), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_speed", true), true);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_control", false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_control", true), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_switch", false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_oscillate", true), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("option_select", false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("option_select", true), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("todo", false), false);
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("todo", true), false);
assert(
  hooks.buttonTypePickerKeysFor(false, "fan_speed").includes("fan_speed"),
  "fan cards are available"
);
assert(!hooks.buttonTypeRuntimeSpec("todo"), "todo card type is not registered");

assert.strictEqual(hooks.normalizeTemperatureUnit("fahrenheit"), "\u00b0F");
assert.strictEqual(hooks.normalizeTemperatureUnit("centigrade"), "\u00b0C");
assert.strictEqual(hooks.normalizeHomeAssistantArtworkPort("80"), 80);
assert.strictEqual(hooks.normalizeHomeAssistantArtworkPort(""), 8123);
assert.strictEqual(hooks.normalizeHomeAssistantArtworkPort(0), 1);
assert.strictEqual(hooks.normalizeHomeAssistantArtworkPort(70000), 65535);
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
  options: "",
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
assert.strictEqual(
  hooks.temperatureUnitSymbolFor("Auto (Home Assistant)", "Auto", "America/New_York"),
  "\u00b0F",
  "Auto temperature unit follows the published active timezone"
);
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
const frenchMonth = new Intl.DateTimeFormat("fr", { month: "long" }).format(hooks.webserverMockNow());
const frenchDatePreview = hooks.buttonTypePreviewForMockNow("calendar", {
  type: "calendar",
  precision: "",
  options: "",
}, {
  language: "fr",
});
assert(frenchDatePreview.labelHtml.includes(frenchMonth), "date preview follows selected language month names");
assert.strictEqual(hooks.dateTimeLargeNumbersLabel({ type: "calendar", precision: "" }), "Large Date");
assert.strictEqual(hooks.dateTimeLargeNumbersLabel({ type: "calendar", precision: "datetime" }), "Large Time");
assert.strictEqual(hooks.dateTimeLargeNumbersLabel({ type: "clock" }), "Large Clock");
assert.strictEqual(hooks.dateTimeLargeNumbersLabel({ type: "timezone" }), "Large World Clock");

const largeDatePreview = hooks.buttonTypePreviewFor("calendar", {
  type: "calendar",
  precision: "",
  options: "",
}, {
  cardSize: 4,
});
assert(largeDatePreview.iconHtml.includes("sp-sensor-preview-large"), "date 2x2 preview defaults to large numbers");

const dateTimePreview = hooks.buttonTypePreviewFor("calendar", {
  type: "calendar",
  precision: "datetime",
  options: "",
}, {
  cardSize: 4,
  clockFormat: "24h",
});
assert(dateTimePreview.iconHtml.includes("sp-sensor-preview-large"), "date/time 2x2 preview defaults to large numbers");
assert(previewSensorValue(dateTimePreview).includes(":"), "date/time preview renders a time value");

const wideDateTimePreview = hooks.buttonTypePreviewFor("calendar", {
  type: "calendar",
  precision: "datetime",
  options: "large_numbers",
}, {
  cardSize: 3,
  clockFormat: "24h",
});
assert(!wideDateTimePreview.iconHtml.includes("sp-sensor-preview-large"), "date/time wide preview does not support large numbers");
assert.strictEqual(wideDateTimePreview.buttonClass, undefined, "date/time wide preview uses the standard wrapper");
assert(wideDateTimePreview.labelHtml.includes("mdi-calendar-month"), "date/time wide preview keeps the date label");

const clockPreview = hooks.buttonTypePreviewFor("clock", {
  type: "clock",
  options: "",
}, {
  cardSize: 4,
  clockFormat: "24h",
});
assert(previewSensorValue(clockPreview).includes(":"), "clock preview renders a time value");
assert(clockPreview.iconHtml.includes("sp-sensor-preview-large"), "clock 2x2 preview defaults to large numbers");
assert.strictEqual(clockPreview.labelHtml, "", "clock preview does not render a date label");
assert.strictEqual(clockPreview.buttonClass, undefined, "clock 2x2 preview uses the standard wrapper");

const wideClockPreview = hooks.buttonTypePreviewFor("clock", {
  type: "clock",
  options: "large_numbers",
}, {
  cardSize: 3,
  clockFormat: "24h",
});
assert(wideClockPreview.iconHtml.includes("sp-sensor-preview-large"), "clock wide preview supports large numbers");
assert.strictEqual(wideClockPreview.buttonClass, "sp-clock-wide-large", "clock wide large preview is left aligned");
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

const autoTimezonePreview = hooks.buttonTypePreviewForMockNow("timezone", {
  entity: "Auto (Home Assistant)",
  type: "timezone",
  options: "",
}, {
  activeTimezone: "America/New_York",
  clockFormat: "24h",
});
assert(autoTimezonePreview.labelHtml.includes("New York"), "Auto world clock preview uses the published active timezone city");
assert.strictEqual(previewSensorValue(autoTimezonePreview), "04:00", "Auto world clock preview uses the published active timezone time");

const wideTimezonePreview = hooks.buttonTypePreviewFor("timezone", {
  entity: "America/New_York (GMT-5)",
  type: "timezone",
  options: "large_numbers",
}, {
  cardSize: 3,
  clockFormat: "24h",
});
assert(!wideTimezonePreview.iconHtml.includes("sp-sensor-preview-large"), "world clock wide preview does not support large numbers");
assert.strictEqual(wideTimezonePreview.buttonClass, undefined, "world clock wide preview uses the standard wrapper");
assert(wideTimezonePreview.labelHtml.includes("New York"), "world clock wide preview keeps the city label");

const largeTimezonePreview = hooks.buttonTypePreviewFor("timezone", {
  entity: "America/New_York (GMT-5)",
  type: "timezone",
  options: "large_numbers",
}, {
  cardSize: 4,
  clockFormat: "24h",
});
assert(largeTimezonePreview.iconHtml.includes("sp-sensor-preview-large"), "world clock 2x2 preview supports large numbers");

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
  options: "",
}, {
  cardSize: 4,
  temperatureUnit: "\u00b0F",
});
assert(weatherForecastPreview.iconHtml.includes("sp-forecast-value"), "weather forecast preview uses forecast value styling");
assert(weatherForecastPreview.iconHtml.includes("sp-sensor-preview-large"), "weather forecast preview supports large numbers");
assert(weatherForecastPreview.iconHtml.includes("\u00b0F"), "weather forecast preview uses the selected temperature unit");
assert(weatherForecastPreview.labelHtml.includes("Garden"), "weather forecast preview uses the custom label");

const imagePreview = hooks.buttonTypePreviewFor("image", {
  entity: "camera.seaside",
  label: "Seaside",
  type: "image",
  options: "image_label",
});
assert(!imagePreview.iconHtml.includes("sp-image-preview-icon"), "image preview hides the top-left icon by default");
assert(!imagePreview.iconHtml.includes("sp-image-preview-text"), "image preview does not show centered placeholder text");
assert(!imagePreview.iconHtml.includes(">Image<"), "image preview does not show centered Image copy");
assert(imagePreview.labelHtml.includes("Seaside"), "image preview keeps the configured label");

const imageIconPreview = hooks.buttonTypePreviewFor("image", {
  entity: "camera.seaside",
  label: "Seaside",
  type: "image",
  options: "image_icon",
});
assert(imageIconPreview.iconHtml.includes("sp-image-preview-icon mdi mdi-camera"), "image preview shows a top-left camera icon when enabled");
assert(!imageIconPreview.labelHtml.includes("Seaside"), "image preview hides the label when label option is off");
const customImageIconPreview = hooks.buttonTypePreviewFor("image", {
  entity: "image.seaside",
  icon: "Image",
  type: "image",
  options: "image_icon",
});
assert(customImageIconPreview.iconHtml.includes("sp-image-preview-icon mdi mdi-image"), "image preview can use a selected image icon when enabled");

const sensorNumericPreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "sensor.office_temperature",
  label: "Office",
  unit: "\u00b0C",
  type: "sensor",
  precision: "1",
  options: "",
}, { cardSize: 4 });
assert(sensorNumericPreview.iconHtml.includes("sp-sensor-preview-large"), "sensor numeric 2x2 preview defaults to large numbers");
assert(sensorNumericPreview.labelHtml.includes("mdi-gauge"), "sensor numeric preview uses the gauge badge");
assert(sensorNumericPreview.iconHtml.includes("\u00b0C"), "sensor numeric preview includes the unit");

const sensorLargeDisabledPreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "sensor.office_temperature",
  label: "Office",
  unit: "\u00b0C",
  type: "sensor",
  precision: "1",
  options: "large_numbers=off",
}, { cardSize: 4 });
assert(!sensorLargeDisabledPreview.iconHtml.includes("sp-sensor-preview-large"), "sensor numeric 2x2 preview respects disabled large numbers");

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

const sensorIconPreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "binary_sensor.patio_door",
  icon: "Door Closed",
  icon_on: "Door Open",
  type: "sensor",
  precision: "icon",
});
assert(sensorIconPreview.iconHtml.includes("mdi-door"), "sensor icon preview uses the selected icon");
assert(sensorIconPreview.labelHtml.includes("mdi-toggle-switch"), "sensor icon preview uses the icon badge");

const sensorTimePreview = hooks.buttonTypePreviewFor("sensor", {
  sensor: "sensor.ups_runtime",
  label: "UPS Runtime",
  type: "sensor",
  precision: "time",
  options: "time_unit=hours,large_numbers",
}, { cardSize: 4 });
assert.strictEqual(previewSensorValue(sensorTimePreview), "1h 30m", "sensor Time preview shows compact duration formatting");
assert(!sensorTimePreview.iconHtml.includes("sp-sensor-preview-large"), "sensor Time preview remains on the normal responsive layout");

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
  options: "state_entity=sensor.kitchen_power,state_unit=W,state_precision=1",
}, { cardSize: 4 });
assert(actionLargePreview.iconHtml.includes("sp-sensor-preview-large"), "action numeric state 2x2 preview defaults to large numbers");

const actionIconStatePreview = hooks.buttonTypePreviewFor("action", {
  entity: "script.goodnight",
  label: "Goodnight",
  icon: "Flash",
  icon_on: "Check Circle",
  sensor: "script.turn_on",
  type: "action",
  options: "state_entity=input_boolean.goodnight_ready,state_precision=icon",
});
assert(actionIconStatePreview.iconHtml.includes("mdi-toggle-switch"), "action icon state preview uses the icon-state badge");

const actionOptionPreview = hooks.buttonTypePreviewFor("action", {
  entity: "select.wled_preset",
  label: "Preset",
  sensor: "input_select.select_option",
  type: "action",
});
assert(actionOptionPreview.iconHtml.includes("Option"), "action option-select preview uses option text");
assert(actionOptionPreview.labelHtml.includes("mdi-chevron-down"), "action option-select preview uses the dropdown badge");

const localActionPreview = hooks.buttonTypePreviewFor("action", {
  entity: "zoom_mute",
  label: "Zoom Mute",
  icon: "Gesture Tap",
  sensor: "local",
  type: "action",
});
assert(localActionPreview.iconHtml.includes("mdi-gesture-tap"), "local action subtype preview uses the local action icon");
assert(localActionPreview.labelHtml.includes("mdi-chip"), "local action subtype preview uses the local action badge");

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

const fanControlPreview = hooks.buttonTypePreviewFor("fan_control", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan",
  type: "fan_control",
});
assert(!fanControlPreview.iconHtml.includes("sp-slider-preview"), "fan control preview is not an inline slider");
assert(fanControlPreview.labelHtml.includes("mdi-fan"), "fan control preview uses the fan badge");

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
  options: "",
}, { cardSize: 4 });
assert(switchLargePreview.iconHtml.includes("sp-sensor-preview-large"), "switch numeric active display 2x2 preview defaults to large numbers");

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

const coverModalPreview = hooks.buttonTypePreviewFor("cover", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "modal",
  type: "cover",
});
assert(coverModalPreview.iconHtml.includes("sp-slider-preview"), "cover modal preview shows read-only position track");
assert(coverModalPreview.iconHtml.includes("mdi-blinds"), "cover modal preview uses the cover icon");
assert(coverModalPreview.labelHtml.includes("mdi-blinds-horizontal"), "cover modal preview uses the cover badge");

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
assert(subpagePlainPreview.labelHtml.includes("mdi-chevron-right"), "plain subpage preview shows the chevron badge");

const subpageIconPreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "indicator",
  type: "subpage",
});
assert(subpageIconPreview.iconHtml.includes("mdi-blinds"), "subpage icon-state preview uses the configured icon");
assert(subpageIconPreview.labelHtml.includes("mdi-chevron-right"), "subpage icon-state preview shows the chevron badge");

const subpageLightsPreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "light.living_room",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=lights",
});
assert(subpageLightsPreview.iconHtml.includes("mdi-lightbulb"), "lights subpage preset preview uses the lightbulb icon");
assert(subpageLightsPreview.labelHtml.includes("Lighting"), "lights subpage preset preview uses the Lighting label");
assert(subpageLightsPreview.labelHtml.includes("mdi-chevron-right"), "lights subpage preset preview shows the chevron badge");

const subpageMediaPreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "media_player.living_room",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=media",
});
assert(subpageMediaPreview.iconHtml.includes("mdi-speaker"), "media subpage preset preview uses the speaker icon");
assert(subpageMediaPreview.labelHtml.includes("Media"), "media subpage preset preview uses the Media label");
assert(subpageMediaPreview.labelHtml.includes("mdi-chevron-right"), "media subpage preset preview shows the chevron badge");

const subpageClimatePreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "climate.living_room",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=climate",
});
assert(subpageClimatePreview.iconHtml.includes("mdi-thermostat"), "climate subpage preset preview uses the thermostat icon");
assert(subpageClimatePreview.labelHtml.includes("Climate"), "climate subpage preset preview uses the Climate label");
assert(subpageClimatePreview.labelHtml.includes("mdi-chevron-right"), "climate subpage preset preview shows the chevron badge");

const subpagePresencePreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "person.jane",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=presence",
});
assert(subpagePresencePreview.iconHtml.includes("mdi-account"), "presence subpage preset preview uses the account icon");
assert(subpagePresencePreview.labelHtml.includes("Presence"), "presence subpage preset preview uses the Presence label");
assert(subpagePresencePreview.labelHtml.includes("mdi-chevron-right"), "presence subpage preset preview shows the chevron badge");

[
  ["alarm", "alarm_control_panel.home", "mdi-shield-home", "Alarm"],
  ["vacuum", "vacuum.downstairs", "mdi-robot-vacuum", "Vacuum"],
  ["lawn_mower", "lawn_mower.backyard", "mdi-robot-mower", "Lawn Mower"],
  ["weather", "weather.home", "mdi-weather-partly-cloudy", "Weather"],
].forEach(([kind, entity, iconClass, label]) => {
  const preview = hooks.buttonTypePreviewFor("subpage", {
    entity,
    sensor: "indicator",
    type: "subpage",
    options: `subpage_kind=${kind}`,
  });
  assert(preview.iconHtml.includes(iconClass), `${label} subpage preset preview uses the expected icon`);
  assert(preview.labelHtml.includes(label), `${label} subpage preset preview uses the expected label`);
  assert(preview.labelHtml.includes("mdi-chevron-right"), `${label} subpage preset preview shows the chevron badge`);
});

assert(hooks.buttonTypePickerKeysFor(false).includes("lawn_mower"), "lawn mower cards are available in the main picker");
assert(hooks.buttonTypePickerKeysFor(true).includes("lawn_mower"), "lawn mower cards are available in subpages");
assert.deepStrictEqual(plain(hooks.buttonTypeDefaultConfig("lawn_mower")), {
  entity: "",
  label: "",
  icon: "Robot Mower",
  icon_on: "Auto",
  sensor: "start_mowing",
  unit: "",
  type: "lawn_mower",
  precision: "",
  options: "",
}, "lawn mower default config matches the shared contract");
assert.deepStrictEqual(
  Array.from(hooks.cardContractOptions("lawn_mower").find((option) => option.name === "lawn_mower_mode").values),
  ["status", "start_mowing", "dock", "pause_resume"],
  "lawn mower mode values match the scoped service set"
);
const lawnMowerPreview = hooks.buttonTypePreviewFor("lawn_mower", {
  entity: "lawn_mower.backyard",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "bad_mode",
  unit: "ignored",
  type: "lawn_mower",
  precision: "2",
  options: "ignored",
});
assert(lawnMowerPreview.iconHtml.includes("mdi-robot-mower"), "lawn mower preview uses the robot mower icon");
assert(lawnMowerPreview.labelHtml.includes("mdi-robot-mower"), "lawn mower preview badge uses the robot mower icon");

const subpageCustomPresetPreview = hooks.buttonTypePreviewFor("subpage", {
  entity: "climate.living_room",
  label: "Downstairs",
  icon: "Home",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=climate",
});
assert(subpageCustomPresetPreview.iconHtml.includes("mdi-home"), "custom subpage preset preview uses the configured icon");
assert(subpageCustomPresetPreview.labelHtml.includes("Downstairs"), "custom subpage preset preview uses the configured label");

const subpageNumericPreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Open Windows",
  icon: "Window Closed",
  sensor: "sensor.open_windows",
  unit: "%",
  type: "subpage",
});
assert(subpageNumericPreview.iconHtml.includes("sp-sensor-preview"), "subpage numeric preview uses the shared number preview");
assert(subpageNumericPreview.labelHtml.includes("mdi-chevron-right"), "subpage numeric preview shows the chevron badge");
const subpageLargePreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Open Windows",
  icon: "Window Closed",
  sensor: "sensor.open_windows",
  unit: "%",
  type: "subpage",
  options: "",
}, { cardSize: 4 });
assert(subpageLargePreview.iconHtml.includes("sp-sensor-preview-large"), "subpage numeric 2x2 preview defaults to large numbers");

const subpageTextPreview = hooks.buttonTypePreviewFor("subpage", {
  label: "Washer",
  icon: "Thermometer",
  sensor: "sensor.washer_state",
  type: "subpage",
  precision: "text",
});
assert(subpageTextPreview.iconHtml.includes("mdi-thermometer"), "subpage text preview uses the configured icon");
assert(subpageTextPreview.labelHtml.includes("State"), "subpage text preview keeps the State label");
assert(subpageTextPreview.labelHtml.includes("mdi-chevron-right"), "subpage text preview shows the chevron badge");

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
  options: "",
}, { cardSize: 4 });
assert(mediaVolumeLargePreview.iconHtml.includes("sp-sensor-preview-large"), "media volume 2x2 preview defaults to large numbers");

const mediaPositionLargePreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.office",
  label: "Office",
  sensor: "position",
  type: "media",
  options: "",
}, { cardSize: 4 });
assert(mediaPositionLargePreview.iconHtml.includes("sp-sensor-preview-large"), "media position 2x2 preview defaults to large numbers");

const mediaNowPlayingPreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.office",
  sensor: "now_playing",
  type: "media",
  precision: "progress",
});
assert(mediaNowPlayingPreview.iconHtml.includes("Midnight City"), "media now-playing preview keeps title text");
assert(mediaNowPlayingPreview.labelHtml.includes("sp-media-now-artist"), "media now-playing preview keeps artist styling");

const mediaCoverArtPreview = hooks.buttonTypePreviewFor("media", {
  entity: "media_player.office",
  sensor: "cover_art",
  type: "media",
});
assert.strictEqual(mediaCoverArtPreview.buttonClass, "sp-image-card", "media cover art preview uses the image-card wrapper");
assert(mediaCoverArtPreview.iconHtml.includes("sp-image-preview"), "media cover art preview uses the shared camera-card surface");
assert(!mediaCoverArtPreview.iconHtml.includes("sp-media-cover-preview"), "media cover art preview omits the old decorative mock");
assert(mediaCoverArtPreview.labelHtml.includes("sp-image-label"), "media cover art preview uses the shared padded image label");
assert(mediaCoverArtPreview.labelHtml.includes("Cover Art"), "media cover art preview shows the Cover Art label");
assert(!mediaCoverArtPreview.labelHtml.includes("Now Playing"), "media cover art preview does not show the now-playing label");

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
assert.strictEqual(hooks.webserverMockNow().toISOString(), "2026-01-01T09:00:00.000Z");
assert.notStrictEqual(
  hooks.webserverNow().toISOString(),
  "2026-01-01T09:00:00.000Z",
  "production web UI clock uses the real current time"
);
assert(
  hooks.buttonTypePreviewForMockNow("clock", { type: "clock" }, { clockFormat: "24h" }).iconHtml.includes("09:00"),
  "mock webserver clock preview uses fixed 09:00 time"
);
assert(
  hooks.buttonTypePreviewForMockNow("clock", { type: "clock" }, { clockFormat: "12h" }).iconHtml.includes("9:00"),
  "mock webserver clock preview uses fixed 9:00 time in 12h mode"
);
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
assert.strictEqual(hooks.displayFirmwareVersion("dev-jc8012p4a1-20260611-livecheck"), "Dev build");
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
const publicVersionIndex = {
  device: "guition-esp32-p4-jc1060p470",
  versions: [{
    version: "v1.12.0",
    release_url: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.12.0",
    ota: {
      path: "guition-esp32-p4-jc1060p470.ota.bin",
      md5: "0123456789abcdef0123456789abcdef",
    },
  }, {
    version: "v1.11.0",
    release_url: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.11.0",
    ota: {
      path: "versions/v1.11.0/guition-esp32-p4-jc1060p470.ota.bin",
      md5: "abcdef0123456789abcdef0123456789",
    },
  }],
};
assert.deepStrictEqual(plain(hooks.firmwareInfosFromPublicVersions(publicVersionIndex)), [{
  latest_version: "v1.12.0",
  release_url: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.12.0",
  ota_url: "https://jtenniswood.github.io/espcontrol/firmware/guition-esp32-p4-jc1060p470/guition-esp32-p4-jc1060p470.ota.bin",
  ota_filename: "guition-esp32-p4-jc1060p470.ota.bin",
  ota_md5: "0123456789abcdef0123456789abcdef",
}, {
  latest_version: "v1.11.0",
  release_url: "https://github.com/jtenniswood/espcontrol/releases/tag/v1.11.0",
  ota_url: "https://jtenniswood.github.io/espcontrol/firmware/guition-esp32-p4-jc1060p470/versions/v1.11.0/guition-esp32-p4-jc1060p470.ota.bin",
  ota_filename: "guition-esp32-p4-jc1060p470.ota.bin",
  ota_md5: "abcdef0123456789abcdef0123456789",
}]);
assert.deepStrictEqual(plain(hooks.firmwareStateAfterVersionIndex("v1.12.0", publicVersionIndex)), {
  latest: "v1.12.0",
  selected: "v1.11.0",
  installAvailable: true,
  selectorVisible: true,
  installedSelected: false,
  previous: ["v1.11.0"],
});
assert.deepStrictEqual(plain(hooks.firmwareStateAfterVersionIndex("v1.12.0", publicVersionIndex, "v1.11.0")), {
  latest: "v1.12.0",
  selected: "v1.11.0",
  installAvailable: true,
  selectorVisible: true,
  installedSelected: false,
  previous: ["v1.11.0"],
});
assert.strictEqual(
  hooks.firmwareOtaUrlAfterVersionIndex("v1.12.0", publicVersionIndex, "v1.11.0"),
  "https://jtenniswood.github.io/espcontrol/firmware/guition-esp32-p4-jc1060p470/guition-esp32-p4-jc1060p470.ota.bin",
  "latest firmware OTA resolution must not follow the selected previous version"
);
assert.strictEqual(hooks.firmwareVersionLabelFor("", true), "Checking version...");
assert.strictEqual(hooks.firmwareVersionLabelFor("", false), "Version unknown");
assert.deepStrictEqual(plain(hooks.entityDetailPaths("text_sensor", hooks.entityLookupNames("firmware_version"))), [
  "/text_sensor/Firmware%3A%20Version?detail=all",
  "/text_sensor/firmware__version?detail=all",
  "/text_sensor/firmware_version?detail=all",
  "/text_sensor/firmware_version_sensor?detail=all",
]);
assert.strictEqual(hooks.entityInitialDetail("select"), "state");
assert.strictEqual(hooks.entityInitialDetail("text"), "all");
assert.strictEqual(hooks.entityDetailPath("select", "Screen: Timezone", hooks.entityInitialDetail("select")), "/select/Screen%3A%20Timezone");
assert.strictEqual(hooks.entityDetailPath("text", "Button Order", hooks.entityInitialDetail("text")), "/text/Button%20Order?detail=all");
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
  hooks.firmwareUpToDateStatusFor("v1.10.0", "v1.11.1", "NO UPDATE", false),
  false,
  "check-only firmware must not report up to date when the public release is newer"
);
assert.strictEqual(
  hooks.firmwareUpToDateStatusFor("v1.11.1", "v1.11.1", "NO UPDATE", false),
  true,
  "matching installed and public versions may report up to date without install controls"
);
assert.strictEqual(
  hooks.firmwareVersionAfterUpdateInfo("Dev", { state: "NO UPDATE", latest_version: "v1.11.1" }).version,
  "v1.11.1"
);
assert.strictEqual(
  hooks.firmwareVersionAfterUpdateInfo("v1.10.0", { state: "NO UPDATE", latest_version: "v1.11.1" }).installAction,
  "check_then_install",
  "public firmware discovered before the update entity should check and then install"
);
assert.strictEqual(
  hooks.firmwareVersionAfterUpdateInfo("v1.10.0", { state: "UPDATE AVAILABLE", latest_version: "v1.11.1" }).installAction,
  "install",
  "a confirmed firmware update should install immediately"
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

async function verifyLocalFirmwareProfileSelection() {
  const productionOutput = freshWebOutputDir({ testHooks: false });
  const productionBundle = fs.readFileSync(path.join(productionOutput, "www.js"), "utf8");
  const sandbox = createWebSandbox();
  const requested = [];
  sandbox.document.currentScript = null;
  sandbox.document.querySelector = () => ({ getAttribute() { return "/0.js"; } });
  sandbox.fetch = (url) => {
    requested.push(url);
    return Promise.resolve({
      ok: true,
      json: () => Promise.resolve({ device_slug: "guition-esp32-s3-4848s040" }),
    });
  };
  vm.createContext(sandbox);
  vm.runInContext(productionBundle, sandbox, { filename: "shared-local-www.js" });
  await new Promise((resolve) => setImmediate(resolve));
  assert.deepStrictEqual(requested, ["/espcontrol/version.json"]);
  assert(
    sandbox.__domEvents.some((event) => event.type === "DOMContentLoaded"),
    "shared local bundle starts after resolving the firmware device profile",
  );
}

verifyLocalFirmwareProfileSelection()
  .then(() => console.log("Web UI smoke tests passed."))
  .catch((error) => {
    console.error(error);
    process.exitCode = 1;
  });
