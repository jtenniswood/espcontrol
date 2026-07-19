#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const { loadTypeScriptModule } = require("./load_typescript_module");

const ROOT = path.resolve(__dirname, "..");
const MODEL_ENTRY = path.join(ROOT, "src", "webserver", "model", "index.ts");
const PRIMITIVES_ENTRY = path.join(ROOT, "src", "webserver", "model", "config_primitives.ts");
const CARD_CONTRACT_ENTRY = path.join(ROOT, "src", "webserver", "generated", "card_contract.ts");
const COMPAT_FIXTURES = path.join(ROOT, "compatibility", "fixtures", "product_compatibility.json");

const model = loadTypeScriptModule(MODEL_ENTRY);
const primitives = loadTypeScriptModule(PRIMITIVES_ENTRY);
const cardContract = loadTypeScriptModule(CARD_CONTRACT_ENTRY);
const fixtures = JSON.parse(fs.readFileSync(COMPAT_FIXTURES, "utf8"));
const current = fixtures.current;

assert.deepStrictEqual(
  cardContract.CARD_CONFIG_FIELDS,
  ["entity", "label", "icon", "icon_on", "sensor", "unit", "type", "precision", "options"],
  "typed generated card contract exports the saved fields"
);
assert.strictEqual(cardContract.cardContractCardLabel("media"), "Media", "typed card contract is directly testable");
assert.strictEqual(cardContract.cardContractSubpageTypeFromCode("M"), "media", "typed card contract decodes subpage types");
assert.deepStrictEqual(cardContract.CARD_CONFIG_FIELDS, current.generatedContract.fields, "generated card contract preserves saved field order");
assert.strictEqual(primitives.setConfigOption("alpha,beta", "alpha", false), "beta", "option flags are directly testable");
assert.strictEqual(
  primitives.configOptionValue(primitives.setConfigOptionValue("alpha", "message", "Kitchen, Main"), "message"),
  "Kitchen, Main",
  "option values preserve encoded punctuation"
);
assert.strictEqual(model.backOrderToken("B", "Back"), "B", "default back label keeps compact B token");
assert.strictEqual(model.backLabelFromOrder(["1", "B", "2"]), "Back", "missing back label defaults to Back");
for (const token of ["Bw", "Bt", "Bx"]) {
  assert.deepStrictEqual(
    model.parseBackOrderToken(`${token}=Return%20Home`),
    { token, label: "Return Home" },
    `${token} back order token decodes its custom label`
  );
}

function plain(value) {
  return JSON.parse(JSON.stringify(value));
}

const nestedPlaylistOptions = "playlist_content_id=media-source%3A//music/morning%2Cmix=50%25,playlist_player_source=Kitchen%2C Main=Zone 50%25";
assert.strictEqual(
  model.encodeConfigField(nestedPlaylistOptions),
  "playlist_content_id=media-source%253A//music/morning%252Cmix=50%2525%2Cplaylist_player_source=Kitchen%252C Main=Zone 50%2525",
  "config field encoding preserves nested option escaping"
);
assert.strictEqual(
  model.decodeConfigField("Kitchen%2C Main=Zone 50%25"),
  "Kitchen, Main=Zone 50%",
  "config field decoding restores punctuation used inside option values"
);
assert.deepStrictEqual(plain(model.parseRawButtonConfig([
  "~media_player.kitchen",
  "Morning Mix",
  "Music",
  "Auto",
  "playlist",
  "",
  "media",
  "",
  model.encodeConfigField(nestedPlaylistOptions),
].join(","))), {
  entity: "media_player.kitchen",
  label: "Morning Mix",
  icon: "Music",
  icon_on: "Auto",
  sensor: "playlist",
  unit: "",
  type: "media",
  precision: "",
  options: nestedPlaylistOptions,
}, "compact button parsing preserves nested encoded option values");

assert.deepStrictEqual(plain(model.decodeMediaCardConfigV1({
  type: "media",
  entity: "media_player.kitchen",
  sensor: "playlist",
  precision: "state",
  options: nestedPlaylistOptions + ",volume_max=150,large_numbers",
})), {
  version: 1,
  entity: "media_player.kitchen",
  mode: "playlist",
  stateDisplay: "label",
  nowPlayingControl: "none",
  coverArtAction: "play_pause",
  showTrackDetails: false,
  controlLabelDisplay: "status",
  controlNumberDisplay: "icon",
  maxVolumePercent: 100,
  playlist: {
    contentId: "media-source://music/morning,mix=50%",
    contentType: "playlist",
    playerSource: "Kitchen, Main=Zone 50%",
  },
  largeNumbers: true,
}, "Media saved strings cross a versioned typed boundary before application code uses them");
assert.strictEqual(model.decodeMediaCardConfigV1({ type: "sensor" }), null, "Media decoder rejects other card types");
assert.strictEqual(model.decodeMediaCardConfigV1({
  type: "media",
  sensor: "cover_art",
  options: "cover_art_details",
}).showTrackDetails, true, "Media decoder exposes optional cover-art track details");
assert.deepStrictEqual(plain(model.decodeMediaCardConfigV1({
  type: "media",
  sensor: "controls",
  precision: "state",
  options: "media_cover_art,cover_art_action=control_modal,volume_max=0",
})), {
  version: 1,
  entity: "",
  mode: "play_pause",
  stateDisplay: "state",
  nowPlayingControl: "none",
  coverArtAction: "control_modal",
  showTrackDetails: false,
  controlLabelDisplay: "status",
  controlNumberDisplay: "icon",
  maxVolumePercent: 1,
  playlist: { contentId: "", contentType: "playlist", playerSource: "" },
  largeNumbers: false,
}, "Media decoder canonicalises legacy and out-of-range values without changing storage");

assert.deepStrictEqual(plain(model.parseGridOrder("1,2d,3w", 8, 4)), {
  grid: [1, 2, 3, -1, 0, -1, 0, 0],
  sizes: { 2: 2, 3: 3 },
}, "grid order parsing preserves size tokens and spans");
assert.strictEqual(
  model.serializeGridOrder([1, 2, 3, -1, 0, -1, 0, 0], { 2: 2, 3: 3 }),
  "1,2d,3w",
  "grid serialization preserves sparse spanned layout"
);
assert.deepStrictEqual(plain(model.parseGridOrder("1h", 9, 3)), {
  grid: [1, -1, -1, -1, -1, -1, 0, 0, 0],
  sizes: { 1: 8 },
}, "max-wide grid order reserves three columns across two rows");
assert.deepStrictEqual(plain(model.parseGridOrder("1v", 9, 3)), {
  grid: [1, -1, 0, -1, -1, 0, -1, -1, 0],
  sizes: { 1: 9 },
}, "max-tall grid order reserves two columns across three rows");
assert.strictEqual(
  model.serializeGridOrder([1, -1, -1, -1, -1, -1, 0, 0, 0], { 1: 8 }),
  "1h",
  "max-wide grid order serializes with its saved token"
);
assert.strictEqual(
  model.serializeGridOrder([1, -1, 0, -1, -1, 0, -1, -1, 0], { 1: 9 }),
  "1v",
  "max-tall grid order serializes with its saved token"
);
assert.deepStrictEqual(plain(model.parseGridOrder("1p", 20, 5)), {
  grid: [1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0],
  sizes: { 1: 10 },
}, "portrait-large grid order reserves three columns across four rows");
assert.strictEqual(
  model.serializeGridOrder(
    [1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0],
    { 1: model.CARD_SIZE_PORTRAIT_LARGE },
  ),
  "1p",
  "portrait-large grid order serializes with its saved token"
);

const transferCard = {
  entity: "media_player.kitchen",
  label: "Morning, Mix = 50%",
  icon: "Music",
  icon_on: "Auto",
  sensor: "playlist",
  unit: "",
  type: "media",
  precision: "",
  options: nestedPlaylistOptions,
  size: 3,
};
const transferSubpageCard = {
  entity: "",
  label: "Downstairs",
  icon: "Home Floor 0",
  icon_on: "Auto",
  sensor: "generic",
  unit: "",
  type: "subpage",
  precision: "",
  options: "",
  size: 4,
  subpage: {
    order: ["B", "1w"],
    back_label: "Return Home",
    buttons: [{
      entity: "light.kitchen",
      label: "Kitchen",
      icon: "Lightbulb",
      icon_on: "Lightbulb",
      sensor: "",
      unit: "",
      type: "",
      precision: "",
      options: "confirm_on,confirm_message=Turn on%3F",
    }],
  },
};
const transferCode = model.createCardTransferCode(
  { device: "panel-a", firmware: "2026.7.0" },
  [transferCard, transferSubpageCard],
);
assert(!transferCode.includes("\n"), "card transfer code is compact single-line JSON");
const parsedTransfer = plain(model.parseCardTransferCode(transferCode));
assert.strictEqual(parsedTransfer.format, "espcontrol.cards", "card transfer format marker is stable");
assert.strictEqual(parsedTransfer.version, 1, "card transfer format starts at version 1");
assert.deepStrictEqual(parsedTransfer.source, { device: "panel-a", firmware: "2026.7.0" },
  "card transfer keeps source device and firmware");
assert.deepStrictEqual(parsedTransfer.cards[0], transferCard,
  "card transfer preserves punctuation-heavy card configuration and size");
assert.deepStrictEqual(parsedTransfer.cards[1], transferSubpageCard,
  "card transfer preserves a structured subpage");
const extraLargeSubpageCard = {
  ...transferSubpageCard,
  subpage: {
    ...transferSubpageCard.subpage,
    order: ["B", "1q"],
    buttons: [{ ...model.cloneCardConfig(transferCard), options: transferCard.options + ",media_cover_art" }],
  },
};
const extraLargeSubpageCode = model.createCardTransferCode(
  { device: "panel-a", firmware: "2026.7.0" },
  [extraLargeSubpageCard],
);
assert.deepStrictEqual(
  plain(model.parseCardTransferCode(extraLargeSubpageCode).cards[0]),
  plain(extraLargeSubpageCard),
  "card transfer accepts a 3x3 card inside a subpage",
);
const extraLargeTransferCode = model.createCardTransferCode(
  { device: "panel-a", firmware: "2026.7.0" },
  [{ ...transferCard, size: model.CARD_SIZE_EXTRA_LARGE }],
);
assert.strictEqual(
  model.parseCardTransferCode(extraLargeTransferCode).cards[0].size,
  model.CARD_SIZE_EXTRA_LARGE,
  "card transfer accepts the supported 3x3 card size",
);
const maxTallTransferCode = model.createCardTransferCode(
  { device: "panel-a", firmware: "2026.7.0" },
  [{ ...transferCard, type: "camera", size: model.CARD_SIZE_MAX_TALL }],
);
assert.strictEqual(
  model.parseCardTransferCode(maxTallTransferCode).cards[0].size,
  model.CARD_SIZE_MAX_TALL,
  "card transfer accepts the supported 2x3 camera card size",
);
const portraitLargeTransferCode = model.createCardTransferCode(
  { device: "panel-a", firmware: "2026.7.0" },
  [{ ...transferCard, size: model.CARD_SIZE_PORTRAIT_LARGE }],
);
assert.strictEqual(
  model.parseCardTransferCode(portraitLargeTransferCode).cards[0].size,
  model.CARD_SIZE_PORTRAIT_LARGE,
  "card transfer accepts the supported 3x4 card size",
);
const maxWideSubpageCard = {
  ...transferSubpageCard,
  subpage: {
    ...transferSubpageCard.subpage,
    order: ["B", "1h"],
    buttons: [{ ...model.cloneCardConfig(transferCard), type: "camera" }],
  },
};
assert.deepStrictEqual(
  plain(model.parseCardTransferCode(model.createCardTransferCode(
    { device: "panel-a", firmware: "2026.7.0" },
    [maxWideSubpageCard],
  )).cards[0]),
  plain(maxWideSubpageCard),
  "card transfer accepts a 3x2 camera card inside a subpage",
);

function assertTransferError(value, expected) {
  assert.throws(
    () => model.parseCardTransferCode(typeof value === "string" ? value : JSON.stringify(value)),
    (error) => String(error.cardTransferMessage || error.message).includes(expected),
  );
}

assertTransferError("not json", "could not read the JSON");
assertTransferError({ format: "other", version: 1, source: { device: "", firmware: "" }, cards: [transferCard] },
  "unsupported format");
assertTransferError({ format: "espcontrol.cards", version: 2, source: { device: "", firmware: "" }, cards: [transferCard] },
  "newer version");
assertTransferError({ format: "espcontrol.cards", version: 1, source: { device: "", firmware: "" }, cards: [] },
  "no cards");
assertTransferError({ format: "espcontrol.cards", version: 1, source: { device: "", firmware: "" }, cards: [{ ...transferCard, size: model.CARD_SIZE_PORTRAIT_LARGE + 1 }] },
  "invalid size");
assertTransferError({ format: "espcontrol.cards", version: 1, source: { device: "", firmware: "" }, cards: [{ ...transferCard, options: 42 }] },
  "invalid options field");
assertTransferError({ format: "espcontrol.cards", version: 1, source: { device: "", firmware: "" }, cards: [{ ...transferCard, subpage: transferSubpageCard.subpage }] },
  "only a Subpage card");
assertTransferError({
  format: "espcontrol.cards", version: 1, source: { device: "", firmware: "" },
  cards: [{ ...transferSubpageCard, subpage: { ...transferSubpageCard.subpage, order: ["B", "99"] } }],
}, "invalid order");
assertTransferError("x".repeat(model.CARD_TRANSFER_MAX_BYTES + 1), "too large");
const currentLayout = model.parseGridOrder(current.layoutImport.order, 20, 5);
assert.deepStrictEqual(
  plain(currentLayout.grid.slice(0, current.layoutImport.expectedGridPrefix.length)),
  current.layoutImport.expectedGridPrefix,
  "current fixture cross-device layout import grid parses in the model"
);
assert.deepStrictEqual(
  plain(currentLayout.sizes),
  current.layoutImport.expectedSizes,
  "current fixture cross-device layout import sizes parse in the model"
);

assert.deepStrictEqual(plain(model.parseSubpageOrder("1,B=Return%3AHome,2d")), {
  order: ["1", "B", "2d"],
  backLabel: "Return:Home",
}, "subpage order parsing decodes back labels");
const subpageGrid = model.buildSubpageGrid({
  order: ["1", "B", "2w"],
  buttons: [{ label: "One" }, { label: "Two" }],
  sizes: {},
}, 8, 4);
assert.deepStrictEqual(plain(subpageGrid.grid), [1, -2, 2, -1, 0, 0, 0, 0], "subpage grid includes explicit back button");
assert.deepStrictEqual(plain(subpageGrid.sizes), { 2: 3 }, "subpage grid preserves button sizes");
assert.deepStrictEqual(
  plain(
    model.serializeSubpageGrid(subpageGrid.grid, subpageGrid.sizes, "Return"),
  ),
  ["1", "B=Return", "2w"],
  "subpage grid serializes back label and size tokens"
);
assert.deepStrictEqual(plain(model.parseRawSubpageConfig(
  "~1,B|M,media_player.living,Living,Speaker,,play_pause,,,",
  (code) => ({ M: "media" }[code] || code)
)), {
  order: ["1", "B"],
  buttons: [{
    type: "media",
    entity: "media_player.living",
    label: "Living",
    icon: "Speaker",
    icon_on: "Auto",
    sensor: "play_pause",
    unit: "",
    precision: "",
    options: "",
  }],
  backLabel: "Back",
}, "raw compact subpage parsing decodes fields and type codes");
assert.strictEqual(
  model.legacySubpageFieldsSafe([["scene.movie", "Movie"], ["bad|entity"]]),
  false,
  "legacy subpage field safety rejects pipe characters"
);
assert.strictEqual(
  model.serializeLegacySubpageConfig(["1", "B"], [["scene.movie", "Movie", "Flash"]]),
  "1,B|scene.movie:Movie:Flash",
  "legacy subpage serialization assembles field groups"
);
assert.strictEqual(
  model.serializeCompactSubpageConfig(["1", "B"], [["A", "scene.movie", "Movie", "Flash"]]),
  "~1,B|A,scene.movie,Movie,Flash",
  "compact subpage serialization assembles field groups"
);
assert.strictEqual(
  model.chooseSerializedSubpageConfig(["1", "B"], 1, "1,B|scene.movie:Movie", "~1,B|A,scene.movie,Movie"),
  "1,B|scene.movie:Movie",
  "subpage serialization chooses the shorter compatible format"
);
const structuredSubpageSource = {
  order: ["2d", "3w", "Bt", "1"],
  buttons: [
    { entity: "scene.movie", label: "Movie", icon: "Flash", icon_on: "Auto", sensor: "scene.turn_on", unit: "", type: "action", precision: "", options: "state_entity=light.living" },
    { entity: "media_player.living", label: "Living", icon: "Auto", icon_on: "Auto", sensor: "play_pause", unit: "", type: "media", precision: "state", options: "" },
    { entity: "climate.hallway", label: "Hallway", icon: "Thermostat", icon_on: "Radiator", sensor: "", unit: "", type: "climate_control", precision: "1", options: "number_display=icon" },
    { entity: "", label: "Temp", icon: "Thermometer", icon_on: "Auto", sensor: "sensor.hallway_temperature", unit: "\u00B0C", type: "sensor", precision: "1", options: "large_numbers" },
    { entity: "", label: "", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" },
  ],
  backLabel: "Return Home",
};
const structuredSubpageObject = model.structuredSubpageFromParsed(structuredSubpageSource);
assert.deepStrictEqual(plain(structuredSubpageObject), {
  order: ["2d", "3w", "Bt", "1"],
  back_label: "Return Home",
  buttons: structuredSubpageSource.buttons,
}, "structured subpage export preserves readable order, back label, and buttons");
assert.deepStrictEqual(
  plain(model.parseStructuredSubpageConfig(structuredSubpageObject)),
  plain(structuredSubpageSource),
  "structured subpage import round-trips action, media, climate, sensor, and empty buttons"
);
assert.deepStrictEqual(plain(model.parseStructuredSubpageConfig({
  order: ["B=Encoded%20Back", "1"],
  buttons: [{ label: "Partial" }],
})), {
  order: ["B", "1"],
  buttons: [{
    entity: "",
    label: "Partial",
    icon: "Auto",
    icon_on: "Auto",
    sensor: "",
    unit: "",
    type: "",
    precision: "",
    options: "",
  }],
  backLabel: "Encoded Back",
}, "structured subpage import defaults missing card fields and decodes legacy back labels");

const layoutPlan = model.planBackupButtonLayout([
  { entity: "light.kitchen", label: "Kitchen" },
  { entity: "weather.home", label: "Weather", type: "weather" },
  { entity: "climate.hall", label: "Hall", type: "climate_control" },
  {},
], "2w,1,3", 2, 4);
assert.strictEqual(layoutPlan.importedCount, 4, "backup layout records source slot count");
assert.deepStrictEqual(plain(layoutPlan.slotMap), { 1: 2, 2: 1 }, "backup layout maps old slots to new slots");
assert.strictEqual(layoutPlan.button_order, "1w", "backup layout serializes adapted order");
assert.deepStrictEqual(plain(layoutPlan.importedSizes), { 1: 3 }, "backup layout keeps fitting size tokens");
assert.strictEqual(layoutPlan.buttons[0].entity, "weather.home", "backup layout follows saved order first");
assert.strictEqual(layoutPlan.buttons[1].entity, "light.kitchen", "backup layout fills target slots in order");

assert.strictEqual(model.normalizeTemperatureUnit("fahrenheit"), "\u00B0F", "temperature unit normalization");
assert.strictEqual(model.normalizeScheduleWakeTimeout(1), 10, "wake timeout minimum");
assert.strictEqual(model.normalizeScheduleWakeBrightness(1), 1, "wake brightness supports one percent");
assert.strictEqual(model.normalizePowerMode("battery-saver"), "Battery Saver", "battery power mode normalization");
assert.strictEqual(model.normalizePowerMode("unexpected"), "Normal", "unknown power mode defaults to Normal");
assert.strictEqual(model.normalizeCoverArtDelay(0), 3, "legacy immediate cover art delay migrates to three seconds");
assert.strictEqual(model.normalizeCoverArtDelay(60), 60, "valid cover art delay remains unchanged");
assert.strictEqual(model.normalizeCoverArtDelay(900), 300, "cover art delay clamps to the supported maximum");
assert.strictEqual(model.normalizeScheduleClockBrightness(0), 10, "schedule clock brightness fallback");
assert.strictEqual(model.normalizeScheduleSensorActivation("Sensor On"), "on", "schedule sensor activation accepts on");
assert.strictEqual(model.normalizeScheduleSensorActivation("unexpected"), "off", "schedule sensor activation defaults off");
assert.strictEqual(model.normalizeHomeAssistantArtworkPort("80"), 80, "Home Assistant artwork port accepts custom port");
assert.strictEqual(model.normalizeHomeAssistantArtworkPort(""), 8123, "Home Assistant artwork port defaults to 8123");
assert.strictEqual(model.normalizeHomeAssistantArtworkPort(0), 1, "Home Assistant artwork port clamps low values");
assert.strictEqual(model.normalizeHomeAssistantArtworkPort(70000), 65535, "Home Assistant artwork port clamps high values");
assert.deepStrictEqual(
  plain(model.normalizeBackupScreenSettings({
    brightness_day: "88",
    brightness_night: "55",
    automatic_brightness: false,
    brightness_dawn_time: "5:30",
    brightness_dusk_time: "21:05",
    schedule_enabled: true,
    schedule_sensor_activation: "Sensor On",
    schedule_on_hour: 7,
    schedule_off_hour: 22,
    schedule_mode: "clock",
    schedule_wake_timeout: 30,
  }, {
    scheduleWakeBrightness: 70,
    scheduleDimmedBrightness: 12,
    scheduleClockBrightness: 40,
    scheduleClockTextColor: "ABCDEF",
  })),
  {
    powerMode: "Normal",
    brightnessDayVal: 88,
    brightnessNightVal: 55,
    automaticBrightnessEnabled: false,
    brightnessDawnTime: "05:30",
    brightnessDuskTime: "21:05",
    scheduleTrigger: "time",
    scheduleEnabled: true,
    scheduleSensorActivation: "on",
    scheduleOnHour: 7,
    scheduleOffHour: 22,
    scheduleMode: "clock",
    scheduleWakeTimeout: 30,
    scheduleWakeBrightness: 70,
    scheduleDimmedBrightness: 12,
    scheduleClockBrightness: 40,
    scheduleClockTextColor: "ABCDEF",
  },
  "backup screen settings normalize with current-value fallbacks"
);

const panelSettings = model.normalizeBackupPanelSettings({
  temperature_unit: "centigrade",
  outdoor_temp_enable: false,
  clock_bar_temperature_entities: "sensor.porch_temperature",
  clock_bar_time: false,
  network_status_icon: false,
  voice_services: true,
  alarm_delay_audio: true,
  alarm_delay_tts: false,
  alarm_delay_entry_announcement: "  Entry warning  ",
  alarm_delay_exit_announcement: "Exit warning",
  alarm_delay_beep_volume: 0.7,
  alarm_delay_final_countdown: 12,
  language: "it",
  clock_format: "24h",
  ntp_server_1: "pool.ntp.org",
  screensaver_mode: "timer",
  screensaver_action: "Screen Dimmed",
  cover_art_hide_external_input: true,
  home_assistant_artwork_protocol: "https",
  home_assistant_artwork_port: "80",
  firmware_auto_update: false,
  firmware_update_frequency: "Weekly",
  clock_brightness_day: 44,
  clock_brightness_night: 22,
  screen_rotation: "90",
}, {
  timezone: "UTC (GMT+0)",
  language: "en",
  clockFormat: "12h",
  clockFormatOptions: ["12h", "24h"],
  ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"],
  ntpServer1: "0.pool.ntp.org",
  ntpServer2: "1.pool.ntp.org",
  ntpServer3: "2.pool.ntp.org",
  coverArtHomeAssistantProtocol: "http",
  coverArtHomeAssistantPort: 8123,
  autoUpdate: true,
  updateFrequency: "Daily",
  updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  screenRotationOptions: ["0", "90", "180", "270"],
});
assert.strictEqual(panelSettings.temperatureUnit, "\u00B0C", "panel temperature unit normalizes");
assert.strictEqual(panelSettings.outdoorTempEnable, false, "panel clock bar temperature visibility imports");
assert.deepStrictEqual(plain(panelSettings.clockBarTemperatureEntities), ["sensor.porch_temperature"], "panel clock bar temperature entity imports");
assert.strictEqual(panelSettings.clockBarTime, false, "panel clock bar time imports");
assert.strictEqual(panelSettings.networkStatusIcon, false, "panel clock bar network status imports");
assert.strictEqual(panelSettings.voiceServices, true, "panel voice services imports");
assert.strictEqual(panelSettings.alarmDelayAudio, true, "panel alarm delay audio imports");
assert.strictEqual(panelSettings.alarmDelayTts, false, "panel alarm delay TTS imports");
assert.strictEqual(panelSettings.alarmDelayEntryAnnouncement, "Entry warning", "panel entry announcement normalizes");
assert.strictEqual(panelSettings.alarmDelayExitAnnouncement, "Exit warning", "panel exit announcement imports");
assert.strictEqual(panelSettings.alarmDelayBeepVolume, 0.7, "panel alarm beep volume imports");
assert.strictEqual(panelSettings.alarmDelayFinalCountdown, 12, "panel final countdown imports");
assert.strictEqual(panelSettings.language, "it", "panel language imports");
assert.strictEqual(panelSettings.clockFormat, "24h", "panel clock format validates against options");
assert.strictEqual(panelSettings.ntpServer1, "pool.ntp.org", "panel NTP server imports");
assert.strictEqual(panelSettings.screensaverMode, "timer", "panel screensaver mode imports");
assert.strictEqual(panelSettings.screensaverAction, "dim", "panel screensaver action imports");
assert.strictEqual(panelSettings.coverArtHideExternalInput, true, "panel cover art external-input setting imports");
assert.strictEqual(panelSettings.coverArtHomeAssistantProtocol, "https", "panel Home Assistant artwork protocol imports");
assert.strictEqual(panelSettings.coverArtHomeAssistantPort, 80, "panel Home Assistant artwork port imports");
assert.strictEqual(panelSettings.autoUpdate, false, "panel firmware auto-update imports");
assert.strictEqual(panelSettings.updateFrequency, "Weekly", "panel firmware update frequency imports");
assert.strictEqual(
  model.normalizeBackupPanelSettings({}, {
    timezone: "UTC (GMT+0)", language: "en", clockFormat: "12h", clockFormatOptions: ["12h", "24h"],
    ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"], ntpServer1: "0.pool.ntp.org",
    ntpServer2: "1.pool.ntp.org", ntpServer3: "2.pool.ntp.org", coverArtHomeAssistantProtocol: "http",
    coverArtHomeAssistantPort: 8123, autoUpdate: true, updateFrequency: "Daily",
    updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"], screenRotationOptions: ["0", "90", "180", "270"],
  }).mediaPlayerSleepPrevention,
  true,
  "missing media sleep prevention setting defaults on",
);
assert.strictEqual(
  model.normalizeBackupPanelSettings({ media_player_sleep_prevention: false, cover_art_delay: 0 }, {
    timezone: "UTC (GMT+0)", language: "en", clockFormat: "12h", clockFormatOptions: ["12h", "24h"],
    ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"], ntpServer1: "0.pool.ntp.org",
    ntpServer2: "1.pool.ntp.org", ntpServer3: "2.pool.ntp.org", coverArtHomeAssistantProtocol: "http",
    coverArtHomeAssistantPort: 8123, autoUpdate: true, updateFrequency: "Daily",
    updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"], screenRotationOptions: ["0", "90", "180", "270"],
  }).mediaPlayerSleepPrevention,
  false,
  "explicit media sleep prevention setting remains off",
);
assert.strictEqual(
  model.normalizeBackupPanelSettings({ cover_art_delay: 0 }, {
    timezone: "UTC (GMT+0)", language: "en", clockFormat: "12h", clockFormatOptions: ["12h", "24h"],
    ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"], ntpServer1: "0.pool.ntp.org",
    ntpServer2: "1.pool.ntp.org", ntpServer3: "2.pool.ntp.org", coverArtHomeAssistantProtocol: "http",
    coverArtHomeAssistantPort: 8123, autoUpdate: true, updateFrequency: "Daily",
    updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"], screenRotationOptions: ["0", "90", "180", "270"],
  }).coverArtDelay,
  3,
  "legacy backup cover art delay migrates to three seconds",
);
assert.strictEqual(
  model.normalizeBackupPanelSettings({
    media_player_sleep_prevention_entity: "media_player.living",
  }, {
    timezone: "UTC (GMT+0)",
    language: "en",
    clockFormat: "12h",
    clockFormatOptions: ["12h", "24h"],
    ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"],
    ntpServer1: "0.pool.ntp.org",
    ntpServer2: "1.pool.ntp.org",
    ntpServer3: "2.pool.ntp.org",
    coverArtHomeAssistantProtocol: "http",
    coverArtHomeAssistantPort: 8123,
    autoUpdate: true,
    updateFrequency: "Daily",
    updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
    screenRotationOptions: ["0", "90", "180", "270"],
  }).coverArtMediaPlayerEntity,
  "media_player.living",
  "legacy sleep prevention media player imports into cover art media player"
);
assert.strictEqual(panelSettings.clockBrightnessDay, 44, "panel day clock brightness imports");
assert.strictEqual(panelSettings.clockBrightnessNight, 22, "panel night clock brightness imports");
assert.strictEqual(panelSettings.subpageChevron, true, "panel subpage chevron defaults on");
assert.strictEqual(panelSettings.screenRotation, "90", "panel rotation validates against options");

const invalidPanelOptionSettings = model.normalizeBackupPanelSettings({
  clock_format: "24h",
  firmware_update_frequency: "Yearly",
  screen_rotation: "270",
}, {
  timezone: "UTC (GMT+0)",
  language: "en",
  clockFormat: "12h",
  clockFormatOptions: ["12h"],
  ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"],
  ntpServer1: "0.pool.ntp.org",
  ntpServer2: "1.pool.ntp.org",
  ntpServer3: "2.pool.ntp.org",
  coverArtHomeAssistantProtocol: "http",
  coverArtHomeAssistantPort: 8123,
  autoUpdate: true,
  updateFrequency: "Daily",
  updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  screenRotationOptions: ["0", "90", "180"],
});
assert.strictEqual(invalidPanelOptionSettings.clockFormat, "12h", "invalid backup clock format falls back to current setting");
assert.strictEqual(invalidPanelOptionSettings.updateFrequency, "Daily", "invalid backup update frequency falls back to current setting");
assert.strictEqual(invalidPanelOptionSettings.screenRotation, "0", "invalid backup rotation falls back to a safe default");

const legacyPanelSettings = model.normalizeBackupPanelSettings({}, {
  timezone: "UTC (GMT+0)",
  language: "en",
  clockFormat: "12h",
  clockFormatOptions: ["12h", "24h"],
  ntpDefaults: ["0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org"],
  ntpServer1: "0.pool.ntp.org",
  ntpServer2: "1.pool.ntp.org",
  ntpServer3: "2.pool.ntp.org",
  coverArtHomeAssistantProtocol: "https",
  coverArtHomeAssistantPort: 80,
  autoUpdate: false,
  updateFrequency: "Monthly",
  updateFrequencyOptions: ["Hourly", "Daily", "Weekly", "Monthly"],
  screenRotationOptions: ["0", "90", "180", "270"],
});
assert.strictEqual(legacyPanelSettings.clockBarTime, true, "legacy panel settings default clock bar time on");
assert.strictEqual(legacyPanelSettings.voiceServices, false, "legacy panel settings default voice services off");
assert.strictEqual(legacyPanelSettings.alarmDelayAudio, false, "legacy panel settings default alarm audio off");
assert.strictEqual(legacyPanelSettings.alarmDelayTts, true, "legacy panel settings default alarm TTS on");
assert.strictEqual(legacyPanelSettings.alarmDelayEntryAnnouncement, "Please disarm the alarm", "legacy panel settings default entry announcement");
assert.strictEqual(legacyPanelSettings.alarmDelayExitAnnouncement, "Alarm arming, please leave the house", "legacy panel settings default exit announcement");
assert.strictEqual(legacyPanelSettings.alarmDelayBeepVolume, 0.45, "legacy panel settings default moderate beep volume");
assert.strictEqual(legacyPanelSettings.alarmDelayFinalCountdown, 10, "legacy panel settings default final countdown");
assert.strictEqual(model.normalizeAlarmDelayBeepVolume(5), 1, "alarm beep volume clamps high values");
assert.strictEqual(model.normalizeAlarmDelayFinalCountdown(-4), 0, "alarm final countdown clamps low values");
assert.strictEqual(legacyPanelSettings.coverArtHideExternalInput, true, "legacy panel settings default cover art external-input setting on");
assert.strictEqual(legacyPanelSettings.coverArtHomeAssistantProtocol, "https", "legacy panel settings keep current Home Assistant artwork protocol");
assert.strictEqual(legacyPanelSettings.coverArtHomeAssistantPort, 80, "legacy panel settings keep current Home Assistant artwork port");
assert.strictEqual(legacyPanelSettings.autoUpdate, false, "legacy panel settings keep current firmware auto-update setting");
assert.strictEqual(legacyPanelSettings.updateFrequency, "Monthly", "legacy panel settings keep current firmware update frequency");

console.log("Model contract tests passed.");
