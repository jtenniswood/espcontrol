#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { loadBuiltWebSource } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const SOURCE = path.join(ROOT, "src", "webserver", "entry.js");
const COMPAT_FIXTURES = path.join(ROOT, "compatibility", "fixtures", "product_compatibility.json");
const CONFIG_DIR = path.join(ROOT, "common", "config");
const CARD_NORMALIZATION_FIXTURES = path.join(ROOT, "common", "config", "card_normalization_fixtures.json");
const IMAGE_CARD_NORMALIZATION_FIXTURES = path.join(ROOT, "common", "config", "image_card_normalization_fixtures.json");

function loadHooks(search) {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    location: { search: search || "" },
    URLSearchParams,
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
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: SOURCE });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
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

function fixtureLabelFromFile(fileName) {
  return fileName
    .replace(/_card_normalization_fixtures\.json$/, "")
    .replace(/_/g, " ");
}

function loadNormalizationFixtureGroups() {
  const groups = [];
  const shared = JSON.parse(fs.readFileSync(CARD_NORMALIZATION_FIXTURES, "utf8"));
  for (const [label, fixtures] of Object.entries(shared)) {
    groups.push({ label, fixtures });
  }
  for (const fileName of fs.readdirSync(CONFIG_DIR).sort()) {
    if (!fileName.endsWith("_card_normalization_fixtures.json")) continue;
    const fixtures = JSON.parse(fs.readFileSync(path.join(CONFIG_DIR, fileName), "utf8"));
    groups.push({ label: fixtureLabelFromFile(fileName), fixtures });
  }
  return groups;
}

function assertNormalizationFixtures(hooks, groups) {
  for (const group of groups) {
    for (const fixture of group.fixtures) {
      const parsed = buttonShape(hooks.parseButtonConfig(fixture.input));
      assert.deepStrictEqual(parsed, buttonShape(fixture.expected), `${group.label} fixture ${fixture.name}: web parse`);
      const canonical = hooks.serializeButtonConfig(parsed);
      if (Object.prototype.hasOwnProperty.call(fixture, "canonical")) {
        assert.strictEqual(canonical, fixture.canonical, `${group.label} fixture ${fixture.name}: web canonical string`);
      }
      assert.deepStrictEqual(
        buttonShape(hooks.parseButtonConfig(canonical)),
        buttonShape(fixture.expected),
        `${group.label} fixture ${fixture.name}: web canonical round-trip`
      );
    }
  }
}

const hooks = loadHooks();
const fixtures = JSON.parse(fs.readFileSync(COMPAT_FIXTURES, "utf8"));
const cardNormalizationFixtures = JSON.parse(fs.readFileSync(CARD_NORMALIZATION_FIXTURES, "utf8"));
const imageCardNormalizationFixtures = JSON.parse(fs.readFileSync(IMAGE_CARD_NORMALIZATION_FIXTURES, "utf8"));
const normalizationFixtureGroups = loadNormalizationFixtureGroups();
const current = fixtures.current;
const legacyV1 = fixtures["legacy-v1"];
assert(hooks, "web config helpers were not exported");
assert.deepStrictEqual(
  Array.from(hooks.CARD_CONFIG_FIELDS),
  current.generatedContract.fields,
  "generated card contract preserves saved field order"
);
assert.strictEqual(
  hooks.cardContractSubpageTypeCode("climate"),
  current.generatedContract.subpageTypeCodes.climate,
  "generated contract exposes compact type codes"
);
assert.strictEqual(hooks.cardContractSubpageTypeFromCode("H"), "climate", "generated contract exposes compact type decode");
assert.strictEqual(hooks.cardContractLargeNumbersSupported("sensor", "text"), false, "generated contract blocks text sensor large numbers");
assert.strictEqual(hooks.cardContractLargeNumbersSupported("weather", "tomorrow"), true, "generated contract allows weather forecast large numbers");
current.generatedContract.requiredCards.forEach((type) => {
  assert(hooks.cardContractCardKeys().includes(type), `generated contract exposes ${type || "switch"} card identity`);
});
assertNormalizationFixtures(hooks, normalizationFixtureGroups);
assert.strictEqual(hooks.cardContractCardLabel("media"), "Media", "generated contract exposes card labels");
assert.strictEqual(hooks.cardContractAllowInSubpage("subpage"), false, "generated contract exposes subpage placement rules");
const subpageKindOption = Array.from(hooks.cardContractOptions("subpage"))
  .find((option) => option.name === "subpage_kind");
assert.deepStrictEqual(Array.from(subpageKindOption.values), [
  "",
  "switch",
  "lights",
  "climate",
  "presence",
  "media",
  "alarm",
  "cover",
  "garage",
  "gate",
  "lock",
  "vacuum",
  "lawn_mower",
  "weather",
  "sensor",
  "image",
], "subpage type options include status presets for newer card styles");
assert.strictEqual(
  hooks.subpageKind({ options: "subpage_kind=vacuum" }),
  "vacuum",
  "vacuum subpage type is accepted by the web config normalizer"
);
assert.strictEqual(
  hooks.subpageKind({ options: "subpage_kind=lawn_mower" }),
  "lawn_mower",
  "lawn mower subpage type is accepted by the web config normalizer"
);
assert.deepStrictEqual(Array.from(hooks.cardContractDomains("climate")), ["climate"], "generated contract exposes card domains");
assert.deepStrictEqual(buttonShape(hooks.cardContractDefaultConfig("climate")), buttonShape({
  entity: "",
  label: "Climate",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "",
  options: "",
}), "generated contract exposes card defaults");
assert.deepStrictEqual(
  buttonShape(hooks.buttonTypeDefaultConfig("")),
  buttonShape(hooks.cardContractDefaultConfig("")),
  "switch card type default is spec-backed"
);
assert.deepStrictEqual(
  buttonShape(hooks.buttonTypeDefaultConfig("sensor")),
  buttonShape(hooks.cardContractDefaultConfig("sensor")),
  "sensor card type default is spec-backed"
);
function assertButtonTypeSpecBacked(type, description) {
  const spec = hooks.buttonTypeRuntimeSpec(type);
  assert(spec, `${description} runtime spec exists`);
  assert.strictEqual(spec.label, hooks.cardContractCardLabel(type), `${description} picker label is spec-backed`);
  assert.strictEqual(spec.allowInSubpage, hooks.cardContractAllowInSubpage(type), `${description} subpage visibility is spec-backed`);
  assert.strictEqual(spec.pickerKey, hooks.cardContractPickerKey(type), `${description} picker key is spec-backed`);
  assert.strictEqual(spec.hidden, hooks.cardContractHidden(type), `${description} hidden flag is spec-backed`);
  assert.deepStrictEqual(Array.from(spec.domains), Array.from(hooks.cardContractDomains(type)), `${description} entity domains are spec-backed`);
  assert.deepStrictEqual(
    buttonShape(hooks.buttonTypeDefaultConfig(type)),
    buttonShape(hooks.cardContractDefaultConfig(type)),
    `${description} default config is spec-backed`
  );
}
assertButtonTypeSpecBacked("", "switch card");
assertButtonTypeSpecBacked("sensor", "sensor card");
assertButtonTypeSpecBacked("slider", "slider card");
assertButtonTypeSpecBacked("cover", "cover card");
assertButtonTypeSpecBacked("light_brightness", "light brightness card");
assertButtonTypeSpecBacked("light_switch", "light switch card");
assertButtonTypeSpecBacked("light_temperature", "light temperature card");
assertButtonTypeSpecBacked("light_control", "full light control card");
assertButtonTypeSpecBacked("calendar", "calendar card");
assertButtonTypeSpecBacked("clock", "clock card");
assertButtonTypeSpecBacked("timezone", "timezone card");
assertButtonTypeSpecBacked("weather", "weather card");
assertButtonTypeSpecBacked("push", "push card");
assertButtonTypeSpecBacked("screen_lock", "screen lock card");
assertButtonTypeSpecBacked("webhook", "webhook card");
assertButtonTypeSpecBacked("internal", "internal relay card");
assertButtonTypeSpecBacked("garage", "garage card");
assertButtonTypeSpecBacked("gate", "gate card");
assertButtonTypeSpecBacked("lock", "lock card");
assertButtonTypeSpecBacked("media", "media card");
assertButtonTypeSpecBacked("alarm", "alarm card");
assertButtonTypeSpecBacked("alarm_action", "alarm action card");
assertButtonTypeSpecBacked("climate", "climate card");
assert.deepStrictEqual(
  Array.from(hooks.dateTimeModeOptionValues()),
  ["clock", "datetime", "", "timezone"],
  "date/time mode options are spec-backed"
);
assert.strictEqual(hooks.normalizeDateTimeCardMode("clock"), "clock", "date/time clock mode is allowed by spec");
assert.strictEqual(hooks.normalizeDateTimeCardMode("timezone"), "timezone", "date/time world clock mode is allowed by spec");
assert.strictEqual(hooks.normalizeDateTimeCardMode("bad"), "", "date/time invalid mode falls back to date mode");
assert.deepStrictEqual(
  Array.from(hooks.weatherModeOptionValues()),
  ["", "today", "tomorrow"],
  "weather mode options are spec-backed"
);
assert.strictEqual(hooks.normalizeWeatherCardMode("today"), "today", "weather today mode is allowed by spec");
assert.strictEqual(hooks.normalizeWeatherCardMode("bad"), "", "weather invalid mode falls back to current conditions");
assert.strictEqual(
  hooks.cardContractOptionSupportedFor("weather", "large_numbers", { precision: "today" }),
  true,
  "weather large-number option supports forecast modes"
);
assert.strictEqual(
  hooks.cardContractOptionSupportedFor("weather", "large_numbers", { precision: "" }),
  false,
  "weather large-number option blocks current conditions"
);
assert.deepStrictEqual(
  Array.from(hooks.garageModeOptionValues()),
  ["", "open", "close"],
  "garage mode options are spec-backed"
);
assert.strictEqual(hooks.normalizeGarageMode("open"), "open", "garage open mode is allowed by spec");
assert.strictEqual(hooks.normalizeGarageMode("bad"), "", "garage invalid mode falls back to toggle");
assert.strictEqual(hooks.normalizeGarageLabelDisplayMode("status"), "status", "garage status display is allowed by spec");
assert.strictEqual(hooks.normalizeGarageLabelDisplayMode("bad"), "label", "garage invalid display falls back to label");
assert.deepStrictEqual(
  Array.from(hooks.gateModeOptionValues()),
  ["", "open", "close", "stop"],
  "gate mode options are spec-backed"
);
assert.strictEqual(hooks.normalizeGateMode("open"), "open", "gate open mode is allowed by spec");
assert.strictEqual(hooks.normalizeGateMode("stop"), "stop", "gate stop mode is allowed by spec");
assert.strictEqual(hooks.normalizeGateMode("bad"), "", "gate invalid mode falls back to toggle");
assert.strictEqual(hooks.normalizeGateLabelDisplayMode("status"), "status", "gate status display is allowed by spec");
assert.strictEqual(hooks.normalizeGateLabelDisplayMode("bad"), "label", "gate invalid display falls back to label");
assert.deepStrictEqual(
  Array.from(hooks.lockModeOptionValues()),
  ["", "lock", "unlock"],
  "lock mode options are spec-backed"
);
assert.strictEqual(hooks.normalizeLockMode("unlock"), "unlock", "lock unlock mode is allowed by spec");
assert.strictEqual(hooks.normalizeLockMode("bad"), "", "lock invalid mode falls back to toggle");
assert.strictEqual(hooks.pushDefaultIcon(), "Gesture Tap", "push default icon is spec-backed");
assert.strictEqual(hooks.pushDefaultIconOn(), "Auto", "push on icon cleanup is spec-backed");
assert.strictEqual(hooks.webhookMethod("post"), "POST", "webhook method normalizes to uppercase");
assert.strictEqual(hooks.webhookMethod("bad"), "GET", "webhook invalid method falls back to GET");
assert.deepStrictEqual(
  Array.from(hooks.internalRelayModeOptionValues()),
  ["switch", "push"],
  "internal relay mode options are spec-backed"
);
assert.strictEqual(hooks.normalizeInternalRelayMode("push"), "push", "internal relay push mode is allowed by spec");
assert.strictEqual(hooks.normalizeInternalRelayMode("bad"), "switch", "internal relay invalid mode falls back to switch");
assert.strictEqual(hooks.internalRelayDefaultIcon("switch"), "Lightbulb Outline", "internal relay switch icon is spec-backed");
assert.strictEqual(hooks.internalRelayDefaultIcon("push"), "Gesture Tap", "internal relay push icon is spec-backed");
assert.strictEqual(hooks.internalRelayDefaultOnIcon(), "Lightbulb", "internal relay on icon is spec-backed");
assert.deepStrictEqual(
  Array.from(hooks.mediaModeOptionValues()),
  ["control_modal", "play_pause", "previous", "next", "volume", "position", "now_playing", "playlist"],
  "media mode options are spec-backed"
);
assert.strictEqual(hooks.mediaEditorMode("controls"), "play_pause", "legacy media controls mode maps through spec");
assert.strictEqual(hooks.mediaEditorMode("bad"), "play_pause", "invalid media mode falls back through spec");
assert.deepStrictEqual(
  Array.from(hooks.mediaNowPlayingControlValues()),
  ["", "progress", "play_pause"],
  "media now-playing control options are spec-backed"
);
assert.strictEqual(
  hooks.mediaNowPlayingControls({ sensor: "now_playing", precision: "progress" }),
  "progress",
  "media now-playing progress mode is spec-backed"
);
assert.strictEqual(
  hooks.mediaNowPlayingControls({ sensor: "now_playing", precision: "state" }),
  "",
  "media now-playing invalid precision is cleared"
);
assert.strictEqual(hooks.mediaStateDisplayModeSupported("position"), true, "media state display supports position mode");
assert.strictEqual(hooks.mediaStateDisplayModeSupported("volume"), false, "media state display rejects volume mode");
assert.deepStrictEqual(
  Array.from(hooks.mediaPlaylistContentTypeOptions()).map((option) => option[0]),
  ["playlist", "music", "album", "artist", "track", "channel", "episode", "podcast", "tvshow", "video", "movie", "app", "url", "__custom"],
  "media playlist content type dropdown includes common Home Assistant media types"
);
assert.deepStrictEqual(
  Array.from(hooks.mediaPlaylistSourceOptions()).map((option) => option[0]),
  ["spotify", "apple_music", "youtube_music", "plex", "jellyfin", "media_source", "url", "__custom"],
  "media playlist source dropdown includes common source presets"
);
assert.strictEqual(hooks.mediaPlaylistContentTypeKnown("playlist"), true, "media playlist recognizes known content types");
assert.strictEqual(hooks.mediaPlaylistContentTypeKnown("favorite"), false, "media playlist custom content types use the custom field");
const spotifyPlaylistParts = hooks.parseMediaPlaylistContentId("spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO", "playlist");
assert.strictEqual(spotifyPlaylistParts.source, "spotify", "media playlist editor detects Spotify URIs");
assert.strictEqual(spotifyPlaylistParts.contentType, "playlist", "media playlist editor detects Spotify URI media type");
assert.strictEqual(spotifyPlaylistParts.id, "1LG2Lnt9EDQS1DqoE8E2uO", "media playlist editor extracts the Spotify ID");
assert.strictEqual(
  hooks.buildMediaPlaylistContentId("spotify", "playlist", "1LG2Lnt9EDQS1DqoE8E2uO"),
  "spotify:playlist:1LG2Lnt9EDQS1DqoE8E2uO",
  "media playlist editor rebuilds Spotify URIs from separate fields"
);
const mediaSourcePlaylistParts = hooks.parseMediaPlaylistContentId("media-source://music/morning-mix", "playlist");
assert.strictEqual(mediaSourcePlaylistParts.source, "media_source", "media playlist editor detects media source URIs");
assert.strictEqual(mediaSourcePlaylistParts.id, "music/morning-mix", "media playlist editor extracts media source paths");
const playlistOptions = { sensor: "playlist" };
hooks.setMediaPlaylistContentId(playlistOptions, "media-source://music/morning,mix");
hooks.setMediaPlaylistContentType(playlistOptions, "music");
hooks.setMediaPlaylistPlayerSource(playlistOptions, "Kitchen Speaker");
assert.strictEqual(
  hooks.mediaPlaylistContentId(playlistOptions),
  "media-source://music/morning,mix",
  "media playlist content IDs are compact-option encoded"
);
assert.strictEqual(hooks.mediaPlaylistContentType(playlistOptions), "music", "media playlist content type is preserved");
assert.strictEqual(hooks.mediaPlaylistPlayerSource(playlistOptions), "Kitchen Speaker", "media playlist playback source is preserved");
hooks.setMediaPlaylistContentType(playlistOptions, "playlist");
assert.strictEqual(
  playlistOptions.options,
  "playlist_content_id=media-source%3A//music/morning%2Cmix,playlist_player_source=Kitchen Speaker",
  "media playlist default content type is omitted while playback source is preserved"
);
const encodedPlaylistOptions = { sensor: "playlist" };
hooks.setMediaPlaylistContentId(encodedPlaylistOptions, "media-source://music/morning,mix=50%");
hooks.setMediaPlaylistContentType(encodedPlaylistOptions, "music");
hooks.setMediaPlaylistPlayerSource(encodedPlaylistOptions, "Kitchen, Main=Zone 50%");
const encodedPlaylistButton = {
  entity: "media_player.kitchen",
  label: "Morning Mix",
  icon: "Music",
  icon_on: "Auto",
  sensor: "playlist",
  unit: "",
  type: "media",
  precision: "",
  options: encodedPlaylistOptions.options,
};
assertButtonRoundTrip(hooks, "media playlist encoded option values", encodedPlaylistButton, false);
const parsedEncodedPlaylistButton = hooks.parseButtonConfig(hooks.serializeButtonConfig(encodedPlaylistButton));
assert.strictEqual(
  hooks.mediaPlaylistContentId(parsedEncodedPlaylistButton),
  "media-source://music/morning,mix=50%",
  "media playlist content ID keeps punctuation after parse and serialize"
);
assert.strictEqual(
  hooks.mediaPlaylistPlayerSource(parsedEncodedPlaylistButton),
  "Kitchen, Main=Zone 50%",
  "media playlist player source keeps punctuation after parse and serialize"
);
assert.strictEqual(
  hooks.normalizeMediaOptions("volume_max=40", "volume"),
  "volume_max=40",
  "media volume max option is preserved for volume mode"
);
assert.strictEqual(
  hooks.normalizeMediaOptions("volume_max=150", "volume"),
  "",
  "media volume max defaults back to no cap above 100"
);
assert.strictEqual(
  hooks.normalizeMediaOptions("volume_max=40", "play_pause"),
  "",
  "media volume max option is removed outside volume mode"
);
assert.strictEqual(hooks.alarmControlPanelValue(), "control_panel", "alarm combined-control value is spec-backed");
assert.deepStrictEqual(Array.from(hooks.alarmActionValues()), ["away", "home", "disarm"], "alarm default actions are spec-backed");
assert.strictEqual(hooks.normalizeAlarmIconDisplayMode("static"), "static", "alarm static icon mode is spec-backed");
assert.strictEqual(hooks.normalizeAlarmIconDisplayMode("bad"), "status", "alarm invalid icon mode falls back to status");
assert.strictEqual(hooks.normalizeAlarmLabelDisplayMode("name"), "name", "alarm name label mode is spec-backed");
assert.strictEqual(hooks.normalizeAlarmLabelDisplayMode("bad"), "status", "alarm invalid label mode falls back to status");
for (const fixture of cardNormalizationFixtures.alarm) {
  const parsed = buttonShape(hooks.parseButtonConfig(fixture.input));
  assert.deepStrictEqual(parsed, buttonShape(fixture.expected), `alarm fixture ${fixture.name}: web parse`);
  const canonical = hooks.serializeButtonConfig(parsed);
  assert.deepStrictEqual(
    buttonShape(hooks.parseButtonConfig(canonical)),
    buttonShape(fixture.expected),
    `alarm fixture ${fixture.name}: web canonical round-trip`
  );
}
assert.strictEqual(hooks.climateDefaultLabelDisplayMode(), "label", "climate default label display is spec-backed");
assert.strictEqual(hooks.climateDefaultNumberDisplayMode(), "target", "climate default number display is spec-backed");
assert.strictEqual(hooks.climateDefaultTemperatureStep(), "1", "climate default temperature step is spec-backed");
assert.strictEqual(hooks.normalizeClimateLabelDisplayMode("actual"), "actual", "climate actual label display is spec-backed");
assert.strictEqual(hooks.normalizeClimateLabelDisplayMode("bad"), "label", "climate invalid label display falls back to spec default");
assert.strictEqual(hooks.normalizeClimateNumberDisplayMode("icon"), "icon", "climate icon number display is spec-backed");
assert.strictEqual(hooks.normalizeClimateNumberDisplayMode("bad"), "target", "climate invalid number display falls back to spec default");
assert.strictEqual(hooks.normalizeClimateTemperatureStep("0.5"), "0.5", "climate half-degree step is spec-backed");
assert.strictEqual(hooks.normalizeClimateTemperatureStep("bad"), "1", "climate invalid temperature step falls back to spec default");
assert.deepStrictEqual(Array.from(hooks.climatePrecisionValues()), ["", "1", "2", "3"], "climate precision values are spec-backed");
assert.strictEqual(hooks.normalizeClimatePrecisionConfig("3:-1.24:5.05"), "3:-1.2:5.1", "climate precision range cleanup is spec-backed");
assert.strictEqual(hooks.normalizeClimatePrecisionConfig("bad:-25:5"), "0:-25:5", "climate invalid precision preserves custom range with fallback precision");
assert.deepStrictEqual(
  Array.from(hooks.climateControlTabs({ options: "climate_tabs=fan%7Ctemperature%7Cmode" })),
  ["fan", "temperature", "mode"],
  "climate control tabs preserve custom order"
);
assert.strictEqual(
  hooks.normalizeClimateOptions("climate_tabs=temperature%7Cmode%7Cpreset%7Cfan%7Cswing"),
  "",
  "plain climate cards omit climate control tabs"
);
assert.strictEqual(
  hooks.normalizeClimateOptions("climate_tabs=temperature%7Cmode%7Cpreset%7Cfan%7Cswing", true),
  "",
  "default climate control tab order is omitted"
);
assert.strictEqual(
  hooks.normalizeClimateOptions("climate_tabs=bad%7Cfan%7Cfan", true),
  "climate_tabs=fan",
  "invalid and duplicate climate control tabs are removed"
);
const coverOptionSpecs = hooks.cardContractOptions("cover");
const coverOptionByName = Object.fromEntries(coverOptionSpecs.map((option) => [option.name, option]));
assert.deepStrictEqual(
  Array.from(coverOptionByName.cover_mode.values),
  ["modal", "", "tilt", "toggle", "open", "close", "stop", "set_position"],
  "cover mode spec exposes modal, slider, tilt, toggle, and command modes"
);
assert.deepStrictEqual(
  Array.from(hooks.coverModeOptionValues(false)),
  ["modal", "", "tilt", "toggle"],
  "cover mode helper hides command modes when commands are not allowed"
);
assert.deepStrictEqual(
  Array.from(hooks.coverModeOptionValues(true)),
  ["modal", "", "tilt", "toggle", "open", "close", "stop", "set_position"],
  "cover mode helper exposes command modes when commands are allowed"
);
assert.strictEqual(hooks.normalizeCoverMode("modal", true), "modal", "cover modal mode normalizes from spec");
assert.strictEqual(hooks.normalizeCoverMode("set_position", true), "set_position", "cover command mode normalizes from spec");
assert.deepStrictEqual(
  Array.from(hooks.coverModeOptionLabels("")),
  ["modal:All Controls", ":Slider: Position", "tilt:Slider: Tilt", "toggle:Toggle", "open:Open", "close:Close", "stop:Stop", "set_position:Set Position"],
  "cover modal option is visible"
);
assert.deepStrictEqual(
  Array.from(hooks.coverModeOptionLabels("modal")),
  ["modal:All Controls", ":Slider: Position", "tilt:Slider: Tilt", "toggle:Toggle", "open:Open", "close:Close", "stop:Stop", "set_position:Set Position"],
  "saved cover modal cards use the normal modal label"
);
assert.strictEqual(hooks.normalizeCoverMode("set_position", false), "", "cover command mode is rejected when commands are disabled");
assert.strictEqual(hooks.normalizeCoverPosition("-1"), "0", "cover position spec clamps minimum");
assert.strictEqual(hooks.normalizeCoverPosition("101"), "100", "cover position spec clamps maximum");
assert.strictEqual(hooks.normalizeCoverPosition("bad"), "50", "cover position spec provides fallback");
assert.deepStrictEqual(
  Array.from(hooks.coverControlTabs({ options: "cover_tabs=controls%7Cposition" })),
  ["controls", "position"],
  "cover control tabs preserve custom order"
);
assert.strictEqual(
  hooks.normalizeCoverOptions("cover_tabs=position%7Ccontrols%7Ctilt%7Cpresets"),
  "",
  "default cover control tab order is omitted"
);
assert.strictEqual(
  hooks.normalizeCoverOptions("cover_tabs=bad%7Cposition%7Cposition"),
  "cover_tabs=position",
  "invalid and duplicate cover control tabs are removed"
);
assert.strictEqual(hooks.lightTempDefaultRange(), "2000-6500", "light temperature spec exposes default range");
assert.deepStrictEqual(Array.from(hooks.lightTempParseRange("")), [2000, 6500], "light temperature default range is spec-backed");
assert.deepStrictEqual(Array.from(hooks.lightTempParseRange("500-900")), [2000, 6500], "light temperature invalid range falls back to spec defaults");
assert.strictEqual(hooks.lightTempClampMin("999", 1000), 1000, "light temperature spec clamps minimum input");
assert.strictEqual(hooks.lightTempClampMin("10000", 1000), 9900, "light temperature spec clamps minimum below max input");
assert.strictEqual(hooks.lightTempClampMax("10001", 9900), 10000, "light temperature spec clamps maximum input");
assert.deepStrictEqual(Array.from(hooks.lightTempLegacySensorValues()), ["kelvin"], "light temperature spec exposes legacy sensor values");
assert.strictEqual(hooks.lightTempSensorNeedsCleanup("kelvin"), true, "light temperature legacy sensor cleanup is spec-backed");
assert.strictEqual(hooks.lightTempSensorNeedsCleanup(""), false, "light temperature empty sensor remains unchanged");
const switchOptionSpecs = hooks.cardContractOptions("");
const switchOptionByName = Object.fromEntries(switchOptionSpecs.map((option) => [option.name, option]));
assert.deepStrictEqual(
  Array.from(switchOptionSpecs, (option) => option.name),
  ["large_numbers", "confirmation_mode", "on_pattern", "confirm_message", "confirm_yes", "confirm_no"],
  "switch option specs preserve current option order"
);
assert.deepStrictEqual(
  Array.from(switchOptionByName.confirmation_mode.values),
  ["", "off", "on", "both"],
  "switch confirmation mode spec exposes disabled/off/on/both modes"
);
assert.deepStrictEqual(
  Array.from(switchOptionByName.confirmation_mode.storage),
  ["confirm_off", "confirm_on"],
  "switch confirmation mode spec exposes saved option flags"
);
assert.strictEqual(
  switchOptionByName.confirm_message.defaultValueByMode.on,
  "Turn on this device?",
  "switch confirmation message spec exposes mode-specific defaults"
);
assert.strictEqual(switchOptionByName.large_numbers.kind, "flag", "switch large-number option is a flag");
assert.deepStrictEqual(Array.from(switchOptionByName.on_pattern.values), ["", "stripes"], "switch on pattern spec exposes stripes option");
assert.strictEqual(switchOptionByName.confirm_yes.defaultValue, "Yes", "switch confirm text spec exposes current default");
assert.strictEqual(switchOptionByName.confirm_no.defaultValue, "No", "switch cancel text spec exposes current default");
const sensorOptionSpecs = hooks.cardContractOptions("sensor");
const sensorOptionByName = Object.fromEntries(sensorOptionSpecs.map((option) => [option.name, option]));
assert.deepStrictEqual(
  Array.from(sensorOptionSpecs, (option) => option.name),
  ["large_numbers", "active_color", "state_labels", "state_input", "state_output", "state_input_2", "state_output_2"],
  "sensor option specs preserve current option order"
);
assert.deepStrictEqual(
  Array.from(sensorOptionByName.large_numbers.supportedWhen.precisionNot),
  ["icon", "text"],
  "sensor large-number option spec excludes icon and text sensor modes"
);
assert.strictEqual(
  hooks.cardContractOptionSupportedFor("sensor", "large_numbers", { precision: "" }),
  true,
  "sensor large-number option supports numeric mode"
);
assert.strictEqual(
  hooks.cardContractOptionSupportedFor("sensor", "large_numbers", { precision: "text" }),
  false,
  "sensor large-number option blocks text mode"
);
assert.strictEqual(
  hooks.cardContractOptionSupportedFor("sensor", "large_numbers", { precision: "icon" }),
  false,
  "sensor large-number option blocks icon mode"
);
assert.strictEqual(sensorOptionByName.active_color.hidden, true, "sensor active-colour option spec remains hidden");
assert.strictEqual(sensorOptionByName.active_color.migration, "drop", "sensor active-colour option spec documents cleanup");
assert.strictEqual(
  hooks.cardContractOptionSupportedFor("sensor", "active_color", { precision: "text" }),
  false,
  "sensor active-colour option cleanup is spec-backed"
);
assert.deepStrictEqual(Object.assign({}, hooks.cardContractMigrationAlias("weather_forecast")), {
  type: "weather",
  precision: "tomorrow",
}, "generated contract exposes migration aliases");
assert.deepStrictEqual(Object.assign({}, hooks.cardContractMigrationAlias("text_sensor")), {
  type: "sensor",
  precision: "text",
}, "generated contract exposes legacy text sensor migration alias");
Object.entries(current.generatedContract.migrationAliases).forEach(([alias, expected]) => {
  assert.deepStrictEqual(
    Object.assign({}, hooks.cardContractMigrationAlias(alias)),
    expected,
    `generated contract exposes ${alias} migration alias`
  );
});
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
  grid: [1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
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

Object.entries(current.buttons).forEach(([name, button]) => {
  assertButtonRoundTrip(hooks, `current ${name}`, button, false);
});
Object.entries(legacyV1.oldButtonStrings).forEach(([name, value]) => {
  if (value.needsMigration === false) {
    assert.deepStrictEqual(
      buttonShape(hooks.parseButtonConfig(value.input)),
      buttonShape(value.expected),
      `legacy-v1 old button ${name} parses`
    );
  } else {
    assertButtonMigration(hooks, `legacy-v1 old button ${name}`, value.input, value.expected);
  }
});
assertSubpageRoundTrip(hooks, "current subpage", current.subpage, true);
Object.entries(current.compactSubpageStrings).forEach(([name, value]) => {
  assertSubpageRoundTrip(hooks, `current compact subpage ${name}`, value.expected, true);
  assert.deepStrictEqual(
    subpageShape(hooks.parseSubpageConfig(value.input)),
    subpageShape(value.expected),
    `current compact subpage ${name} parses`
  );
});
const currentLayout = hooks.importedButtonOrderFor(current.layoutImport.order, {});
assert.deepStrictEqual(
  Array.from(currentLayout.grid.slice(0, current.layoutImport.expectedGridPrefix.length)),
  current.layoutImport.expectedGridPrefix,
  "current cross-device layout import grid"
);
assert.deepStrictEqual(
  Object.assign({}, currentLayout.sizes),
  current.layoutImport.expectedSizes,
  "current cross-device layout import sizes"
);
const legacyV1BackupPlan = hooks.planBackupImport(legacyV1.backup, { device: "small-panel", slots: 2 });
assert(legacyV1BackupPlan.warnings.some((msg) => msg.includes("different panel")), "legacy-v1 backup warns on device mismatch");
assert.deepStrictEqual(buttonShape(legacyV1BackupPlan.buttons[0]), buttonShape({
  entity: "weather.home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  type: "weather",
  precision: "tomorrow",
}), "legacy-v1 backup migrates weather forecast card");

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
assert.strictEqual(hooks.switchConfirmationMode(parsedConfirmSwitch), "off", "switch confirmation defaults to off mode");
const defaultConfirmSwitch = hooks.parseButtonConfig("switch.printer;Printer;Printer 3D;Auto;;;;;confirm_off");
assert.strictEqual(hooks.switchConfirmationYesText(defaultConfirmSwitch), "Yes", "switch confirmation default yes text");
assert.strictEqual(hooks.switchConfirmationNoText(defaultConfirmSwitch), "No", "switch confirmation default no text");
const confirmOnSwitch = hooks.parseButtonConfig("switch.printer;Printer;Printer 3D;Auto;;;;;confirm_on");
assert.strictEqual(hooks.switchConfirmationEnabled(confirmOnSwitch), true, "switch on confirmation enabled");
assert.strictEqual(hooks.switchConfirmationMode(confirmOnSwitch), "on", "switch on confirmation mode");
assert.strictEqual(hooks.switchConfirmationMessage(confirmOnSwitch), "Turn on this device?", "switch on confirmation default message");
assert.strictEqual(hooks.serializeButtonConfig(confirmOnSwitch), "switch.printer;Printer;Printer 3D;Auto;;;;;confirm_on", "switch on confirmation round-trip");
const patternedSwitch = hooks.parseButtonConfig("switch.printer;Printer;Printer 3D;Auto;;;;;on_pattern=stripes");
assert.strictEqual(hooks.cardOnPattern(patternedSwitch), "stripes", "switch on pattern is decoded");
assert.strictEqual(hooks.serializeButtonConfig(patternedSwitch), "switch.printer;Printer;Printer 3D;Auto;;;;;on_pattern=stripes", "switch on pattern round-trip");
hooks.setCardOnPattern(patternedSwitch, "");
assert.strictEqual(patternedSwitch.options, "", "switch on pattern can be cleared");
const confirmBothSwitch = hooks.parseButtonConfig("switch.printer;Printer;Printer 3D;Auto;;;;;confirm_off,confirm_on");
assert.strictEqual(hooks.switchConfirmationMode(confirmBothSwitch), "both", "switch both confirmation mode");
assert.strictEqual(hooks.switchConfirmationMessage(confirmBothSwitch), "Toggle this device?", "switch both confirmation default message");

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

const localSensorSubtype = {
  entity: "room_temp",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "local",
  unit: "°C",
  type: "sensor",
  precision: "1",
  options: "",
};
assertButtonRoundTrip(hooks, "local sensor subtype", localSensorSubtype, false);
assert.strictEqual(hooks.sensorCardIsLocal(localSensorSubtype), true, "local sensor subtype is detected");
assert.strictEqual(hooks.cardLargeNumbersEnabled({
  type: "sensor",
  sensor: "local",
  precision: "1",
  options: "large_numbers",
}), false, "local sensor subtype does not use large sensor numbers");

const iconSensor = hooks.parseButtonConfig(";;;;binary_sensor.patio_door;;sensor;icon;");
iconSensor.icon = "Door Closed";
iconSensor.icon_on = "Door Open";
assertButtonRoundTrip(hooks, "sensor icon display", iconSensor, false);
assert.strictEqual(
  hooks.cardLargeNumbersEnabled({ type: "sensor", precision: "icon", options: "large_numbers" }),
  false,
  "sensor icon display does not use large numbers"
);

const parsedActiveSensor = hooks.parseButtonConfig(";;;;binary_sensor.patio_door;;sensor;text;active_color");
assert.strictEqual(hooks.sensorActiveColorEnabled(parsedActiveSensor), false, "sensor active colour removed");

const stateLabelSensor = hooks.parseButtonConfig(";;;;sensor.bin_level;;sensor;text;state_labels,state_input=high,state_output=Please%20empty,state_input_2=low,state_output_2=Full");
assert.strictEqual(hooks.sensorStateLabelsEnabled(stateLabelSensor), true, "sensor text state labels enabled");
assert.strictEqual(hooks.sensorStateInput(stateLabelSensor), "high", "sensor state input is decoded");
assert.strictEqual(hooks.sensorStateOutput(stateLabelSensor), "Please empty", "sensor state output is decoded");
assert.strictEqual(hooks.sensorStateInput2(stateLabelSensor), "low", "second sensor state input is decoded");
assert.strictEqual(hooks.sensorStateOutput2(stateLabelSensor), "Full", "second sensor state output is decoded");
assertButtonRoundTrip(hooks, "sensor status translation", stateLabelSensor, false);
const legacyStateLabelSensor = hooks.parseButtonConfig(";;;;sensor.bin_level;;sensor;text;state_labels,state_high_label=Please%20empty");
assert.strictEqual(legacyStateLabelSensor.options, "state_labels,state_input=high,state_output=Please empty", "legacy high label migrates to status translation");
const numericStateLabelSensor = hooks.parseButtonConfig(";;;;sensor.bin_level;;sensor;0;state_labels,state_high_label=Please%20empty");
assert.strictEqual(numericStateLabelSensor.options, "", "sensor state labels are text-mode only");

assertButtonMigration(
  hooks,
  "text sensor drops hidden active colour option",
  "sensor.patio_door;Patio Door;Door Closed;Auto;binary_sensor.patio_door;;sensor;text;large_numbers,active_color",
  {
    entity: "sensor.patio_door",
    label: "Patio Door",
    icon: "Door Closed",
    icon_on: "Auto",
    sensor: "binary_sensor.patio_door",
    unit: "",
    type: "sensor",
    precision: "text",
    options: "",
  }
);

assertButtonRoundTrip(hooks, "door window card door subtype", {
  entity: "",
  label: "Patio Door",
  icon: "Door",
  icon_on: "Door Open",
  sensor: "binary_sensor.patio_door",
  unit: "",
  type: "door_window",
  precision: "door",
  options: "active_color",
}, false);
assert.strictEqual(
  hooks.doorWindowActiveColorEnabled(hooks.parseButtonConfig(";;Auto;Auto;binary_sensor.patio_door;;door_window;window;active_color")),
  true,
  "door/window active colour enabled");
assertButtonMigration(
  hooks,
  "door window defaults icons and subtype",
  ";;Auto;Auto;binary_sensor.kitchen_window;;door_window;;large_numbers,active_color",
  {
    icon: "Door",
    icon_on: "Door Open",
    sensor: "binary_sensor.kitchen_window",
    type: "door_window",
    precision: "door",
    options: "active_color",
  }
);

assertButtonRoundTrip(hooks, "presence card", {
  entity: "",
  label: "Living Room",
  icon: "Motion Sensor Off",
  icon_on: "Motion Sensor",
  sensor: "binary_sensor.living_room_presence",
  unit: "",
  type: "presence",
  precision: "",
  options: "active_color",
}, false);
assert.strictEqual(
  hooks.presenceActiveColorEnabled(hooks.parseButtonConfig(";;Motion Sensor Off;Motion Sensor;binary_sensor.living_room_presence;;presence;;active_color")),
  true,
  "presence active colour enabled");
assertButtonMigration(
  hooks,
  "presence defaults icons",
  ";;Auto;Auto;text_sensor.mmwave_presence;;presence;;large_numbers,active_color",
  {
    icon: "Motion Sensor Off",
    icon_on: "Motion Sensor",
    sensor: "text_sensor.mmwave_presence",
    type: "presence",
    precision: "",
    options: "active_color",
  }
);

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

const screenLockCard = {
  entity: "",
  label: "",
  icon: "Lock",
  icon_on: "Lock Open",
  sensor: "",
  unit: "",
  type: "screen_lock",
  precision: "",
  options: "",
};
assertButtonRoundTrip(hooks, "screen lock local card", screenLockCard, false);
assert.deepStrictEqual(
  buttonShape(hooks.parseButtonConfig(hooks.serializeButtonConfig({
    entity: "switch.should_not_save",
    label: "Screen Lock",
    icon: "Auto",
    icon_on: "Auto",
    sensor: "sensor.should_not_save",
    unit: "%",
    type: "screen_lock",
    precision: "2",
    options: "large_numbers",
  }))),
  buttonShape(screenLockCard),
  "screen lock card strips non-local config fields"
);

assertButtonRoundTrip(hooks, "webhook post json", {
  entity: "https://maker.ifttt.com/trigger/door/json/with/key/test",
  label: "Door Alert",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "POST",
  unit: "{\"value1\":\"Front door\"}",
  type: "webhook",
  precision: "",
  options: "webhook_headers=Content-Type%3A application/json%3B Authorization%3A Bearer token",
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

const garageStatusCard = {
  entity: "cover.garage",
  label: "Garage Door",
  icon: "Garage",
  icon_on: "Garage Open",
  sensor: "",
  unit: "",
  type: "garage",
  precision: "",
  options: "label_display=status",
};
assertButtonRoundTrip(hooks, "garage status button", garageStatusCard, false);
assert.strictEqual(hooks.garageLabelDisplayMode(garageStatusCard), "status", "garage status display option");

assertButtonRoundTrip(hooks, "garage open command button", {
  entity: "cover.garage",
  label: "Open",
  icon: "Garage Open",
  icon_on: "Auto",
  sensor: "open",
  unit: "",
  type: "garage",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "garage open command status button", {
  entity: "cover.garage",
  label: "Open",
  icon: "Garage Open",
  icon_on: "Auto",
  sensor: "open",
  unit: "",
  type: "garage",
  precision: "",
  options: "label_display=status",
}, false);

assert.strictEqual(
  hooks.garageLabelDisplayMode({
    type: "garage",
    sensor: "open",
    options: "label_display=status",
  }),
  "status",
  "garage open command status display option"
);

assertButtonRoundTrip(hooks, "garage close command button", {
  entity: "cover.garage",
  label: "Close",
  icon: "Garage",
  icon_on: "Auto",
  sensor: "close",
  unit: "",
  type: "garage",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "garage close command status button", {
  entity: "cover.garage",
  label: "Close",
  icon: "Garage",
  icon_on: "Auto",
  sensor: "close",
  unit: "",
  type: "garage",
  precision: "",
  options: "label_display=status",
}, false);

assertButtonRoundTrip(hooks, "gate label button", {
  entity: "cover.gate",
  label: "Gate",
  icon: "Gate",
  icon_on: "Gate Open",
  sensor: "",
  unit: "",
  type: "gate",
  precision: "",
}, false);

const gateStatusCard = {
  entity: "cover.gate",
  label: "Gate",
  icon: "Gate",
  icon_on: "Gate Open",
  sensor: "",
  unit: "",
  type: "gate",
  precision: "",
  options: "label_display=status",
};
assertButtonRoundTrip(hooks, "gate status button", gateStatusCard, false);
assert.strictEqual(hooks.gateLabelDisplayMode(gateStatusCard), "status", "gate status display option");

assertButtonRoundTrip(hooks, "gate open command button", {
  entity: "cover.gate",
  label: "Open",
  icon: "Gate Open",
  icon_on: "Auto",
  sensor: "open",
  unit: "",
  type: "gate",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "gate open command status button", {
  entity: "cover.gate",
  label: "Open",
  icon: "Gate Open",
  icon_on: "Auto",
  sensor: "open",
  unit: "",
  type: "gate",
  precision: "",
  options: "label_display=status",
}, false);

assert.strictEqual(
  hooks.gateLabelDisplayMode({
    type: "gate",
    sensor: "open",
    options: "label_display=status",
  }),
  "status",
  "gate open command status display option"
);

assertButtonRoundTrip(hooks, "gate close command button", {
  entity: "cover.gate",
  label: "Close",
  icon: "Gate",
  icon_on: "Auto",
  sensor: "close",
  unit: "",
  type: "gate",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "gate close command status button", {
  entity: "cover.gate",
  label: "Close",
  icon: "Gate",
  icon_on: "Auto",
  sensor: "close",
  unit: "",
  type: "gate",
  precision: "",
  options: "label_display=status",
}, false);

assertButtonRoundTrip(hooks, "gate stop command button", {
  entity: "cover.gate",
  label: "Stop",
  icon: "Stop",
  icon_on: "Auto",
  sensor: "stop",
  unit: "",
  type: "gate",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "gate stop command status button", {
  entity: "cover.gate",
  label: "Stop",
  icon: "Stop",
  icon_on: "Auto",
  sensor: "stop",
  unit: "",
  type: "gate",
  precision: "",
  options: "label_display=status",
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

assertButtonRoundTrip(hooks, "lock command button", {
  entity: "lock.front_door",
  label: "Lock",
  icon: "Lock",
  icon_on: "Auto",
  sensor: "lock",
  unit: "",
  type: "lock",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "unlock command button", {
  entity: "lock.front_door",
  label: "Unlock",
  icon: "Lock Open",
  icon_on: "Auto",
  sensor: "unlock",
  unit: "",
  type: "lock",
  precision: "",
}, false);

const defaultAlarmCard = {
  entity: "alarm_control_panel.house",
  label: "House Alarm",
  icon: "Security",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "",
};
assertButtonRoundTrip(hooks, "alarm card default options", defaultAlarmCard, false);
assert.strictEqual(hooks.alarmPinRequired(defaultAlarmCard, "arm"), true, "alarm arm PIN default");
assert.strictEqual(hooks.alarmPinRequired(defaultAlarmCard, "disarm"), true, "alarm disarm PIN default");
assert.strictEqual(hooks.alarmIconDisplayMode(defaultAlarmCard), "status", "alarm icon display default");
assert.strictEqual(hooks.alarmLabelDisplayMode(defaultAlarmCard), "status", "alarm label display default");
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(defaultAlarmCard)), ["away", "home", "disarm"], "alarm default visible actions");

const customAlarmCard = {
  entity: "alarm_control_panel.house",
  label: "House Alarm",
  icon: "Alarm",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "pin_arm=0,actions=away%7Cdisarm",
};
assertButtonRoundTrip(hooks, "alarm card custom options", customAlarmCard, false);
const parsedCustomAlarm = hooks.parseButtonConfig(hooks.serializeButtonConfig(customAlarmCard));
assert.strictEqual(hooks.alarmPinRequired(parsedCustomAlarm, "arm"), false, "alarm arm PIN override");
assert.strictEqual(hooks.alarmPinRequired(parsedCustomAlarm, "disarm"), true, "alarm disarm PIN remains default");
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(parsedCustomAlarm)), ["away", "disarm"], "alarm visible action subset");

const nightVacationAlarmCard = {
  entity: "alarm_control_panel.house",
  label: "House Alarm",
  icon: "Alarm",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "actions=night%7Cvacation",
};
assertButtonRoundTrip(hooks, "alarm card night vacation options", nightVacationAlarmCard, false);
const parsedNightVacationAlarm = hooks.parseButtonConfig(hooks.serializeButtonConfig(nightVacationAlarmCard));
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(parsedNightVacationAlarm)), ["night", "vacation"], "alarm visible night and vacation actions");

const tooManyAlarmCard = {
  entity: "alarm_control_panel.house",
  label: "House Alarm",
  icon: "Alarm",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "actions=away%7Chome%7Cnight%7Cvacation%7Cdisarm",
};
const parsedTooManyAlarm = hooks.parseButtonConfig(hooks.serializeButtonConfig(tooManyAlarmCard));
assert.deepStrictEqual(Array.from(hooks.alarmVisibleActions(parsedTooManyAlarm)), ["away", "home", "night"], "alarm visible actions are limited to three");
assert.strictEqual(parsedTooManyAlarm.options, "actions=away%7Chome%7Cnight", "alarm visible action overflow is trimmed when saved");

const statusAlarmCard = {
  entity: "alarm_control_panel.house",
  label: "House Alarm",
  icon: "Security",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "",
};
assertButtonRoundTrip(hooks, "alarm card status icon and label options", statusAlarmCard, false);
const parsedStatusAlarm = hooks.parseButtonConfig(hooks.serializeButtonConfig(statusAlarmCard));
assert.strictEqual(hooks.alarmIconDisplayMode(parsedStatusAlarm), "status", "alarm status icon option");
assert.strictEqual(hooks.alarmLabelDisplayMode(parsedStatusAlarm), "status", "alarm status label option");

const staticAlarmCard = {
  entity: "alarm_control_panel.house",
  label: "House Alarm",
  icon: "Security",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "icon_display=static,label_display=name",
};
assertButtonRoundTrip(hooks, "alarm card static icon and name label options", staticAlarmCard, false);
const parsedStaticAlarm = hooks.parseButtonConfig(hooks.serializeButtonConfig(staticAlarmCard));
assert.strictEqual(hooks.alarmIconDisplayMode(parsedStaticAlarm), "static", "alarm static icon option");
assert.strictEqual(hooks.alarmLabelDisplayMode(parsedStaticAlarm), "name", "alarm name label option");

assertButtonMigration(hooks, "alarm clears ignored fields", "alarm_control_panel.house;House;Auto;Alarm;sensor.temp;W;alarm;2;pin_disarm=0,actions=home%7Cnight", {
  entity: "alarm_control_panel.house",
  label: "House",
  icon: "Security",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "alarm",
  precision: "",
  options: "pin_disarm=0,actions=home%7Cnight",
});

assertButtonRoundTrip(hooks, "alarm action button", {
  entity: "alarm_control_panel.house",
  label: "Arm Home",
  icon: "Shield Home",
  icon_on: "Auto",
  sensor: "home",
  unit: "",
  type: "alarm_action",
  precision: "",
  options: "pin_arm=0",
}, false);

assertButtonRoundTrip(hooks, "alarm night action button", {
  entity: "alarm_control_panel.house",
  label: "Arm Night",
  icon: "Weather Night",
  icon_on: "Auto",
  sensor: "night",
  unit: "",
  type: "alarm_action",
  precision: "",
  options: "pin_arm=0",
}, false);

assertButtonRoundTrip(hooks, "alarm vacation action button", {
  entity: "alarm_control_panel.house",
  label: "Arm Vacation",
  icon: "Airplane",
  icon_on: "Auto",
  sensor: "vacation",
  unit: "",
  type: "alarm_action",
  precision: "",
  options: "pin_arm=0",
}, false);

assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm", false), true, "alarm modal picker visible on parent page");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm", true), true, "alarm card family visible in subpages");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm_action", false), false, "alarm actions hidden as a separate picker item");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("alarm_action", true), false, "alarm actions hidden as a separate subpage picker item");
assert.strictEqual(
  hooks.buttonTypePickerKeysFor(false, "alarm").indexOf("alarm") >= 0,
  true,
  "saved alarm modal type remains selectable");
assert.strictEqual(
  hooks.buttonTypePickerKeysFor(true, "alarm").indexOf("alarm") >= 0,
  true,
  "saved alarm action subtype remains represented in subpages");

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

assertButtonRoundTrip(hooks, "cover modal custom tabs", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "modal",
  unit: "",
  type: "cover",
  precision: "",
  options: "cover_tabs=controls%7Cposition",
}, false);

assertButtonMigration(hooks, "cover non-modal clears modal tabs", "cover.office_blind;Office Blind;Blinds;Blinds Open;toggle;;cover;;cover_tabs=controls%7Cposition", {
  entity: "cover.office_blind",
  label: "Office Blind",
  icon: "Blinds",
  icon_on: "Blinds Open",
  sensor: "toggle",
  unit: "",
  type: "cover",
  precision: "",
  options: "",
});

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

assertButtonRoundTrip(hooks, "calendar date card", {
  entity: "sensor.date",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "calendar",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "calendar date and time card", {
  entity: "sensor.date",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "calendar",
  precision: "datetime",
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

assertButtonRoundTrip(hooks, "clock card", {
  entity: "",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "clock",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "clock large numbers option", {
  entity: "",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "clock",
  precision: "",
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

assertButtonRoundTrip(hooks, "weather current conditions card", {
  entity: "weather.forecast_home",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "weather",
  precision: "",
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

assertButtonRoundTrip(hooks, "media control modal card", {
  entity: "media_player.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "control_modal",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "media control modal card label display", {
  entity: "media_player.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "control_modal",
  unit: "",
  type: "media",
  precision: "",
  options: "label_display=label",
}, false);

assertButtonRoundTrip(hooks, "media control modal card custom icon", {
  entity: "media_player.living_room",
  label: "Living Room",
  icon: "Music",
  icon_on: "Auto",
  sensor: "control_modal",
  unit: "",
  type: "media",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "climate card", {
  entity: "climate.living_room",
  label: "Living Room",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
}, false);

assertButtonRoundTrip(hooks, "climate card precision 2", {
  entity: "climate.bedroom",
  label: "Bedroom",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "2",
}, false);

assertButtonRoundTrip(hooks, "climate card firmware precision 3", {
  entity: "climate.office",
  label: "Office",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "3",
}, false);

assertButtonRoundTrip(hooks, "climate card custom range", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1:16:30",
}, false);

assertButtonRoundTrip(hooks, "climate card negative custom range", {
  entity: "climate.freezer",
  label: "Freezer",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1:-25:5",
}, false);

assertButtonRoundTrip(hooks, "climate card display options", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
  options: "label_display=status,number_display=actual",
}, false);

assertButtonRoundTrip(hooks, "climate card half-degree step", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
  options: "temperature_step=0.5",
}, false);

assertButtonRoundTrip(hooks, "climate card icon display", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Thermostat",
  icon_on: "Radiator",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
  options: "number_display=icon",
}, false);

assertButtonMigration(hooks, "legacy climate keeps all-controls tabs", "climate.hallway;Hallway;Thermostat;Auto;;;climate;1;climate_tabs=mode%7Ctemperature", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
  options: "climate_tabs=mode%7Ctemperature",
});

assertButtonRoundTrip(hooks, "climate all controls custom tabs", {
  entity: "climate.hallway",
  label: "Hallway",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
  options: "climate_tabs=mode%7Ctemperature",
}, false);

assertButtonMigration(hooks, "climate clears ignored fields", "climate.living_room;Living;Thermostat;Radiator;sensor.temp;deg C;climate;bad", {
  entity: "climate.living_room",
  label: "Living",
  icon: "Thermostat",
  icon_on: "Radiator",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "",
});

assertButtonMigration(hooks, "climate clears legacy options", "climate.living_room;Living;Thermostat;Auto;;;climate;1;large_numbers,off_target", {
  entity: "climate.living_room",
  label: "Living",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "climate_control",
  precision: "1",
  options: "large_numbers",
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

assertButtonRoundTrip(hooks, "light brightness card", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "light_brightness",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "light switch card", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "light_switch",
  precision: "",
  options: "",
}, false);
assertButtonRoundTrip(hooks, "full light control card", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "light_control",
  precision: "",
  options: "",
}, false);
assertButtonRoundTrip(hooks, "full light control custom tabs", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "light_control",
  precision: "",
  options: "light_tabs=brightness%7Cpower",
}, false);
assert.deepStrictEqual(
  Array.from(hooks.lightControlTabs({ options: "light_tabs=brightness%7Cpower" })),
  ["brightness", "power"],
  "light control tabs preserve custom order");
assert.strictEqual(
  hooks.normalizeLightControlOptions("light_tabs=power%7Cbrightness%7Ctemperature%7Ccolor"),
  "",
  "default light control tab order is omitted");
assert.strictEqual(
  hooks.normalizeLightControlOptions("light_tabs=bad%7Cpower%7Cpower"),
  "light_tabs=power",
  "invalid and duplicate light control tabs are removed");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_brightness", false), true, "lights picker visible on parent page");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_brightness", true), true, "lights picker visible in subpages");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_switch", false), false, "light switch subtype hidden from top-level picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_switch", true), false, "light switch subtype hidden from subpage picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_temperature", false), false, "light temperature subtype hidden from top-level picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_temperature", true), false, "light temperature subtype hidden from subpage picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("light_control", false), false, "full light control subtype hidden from top-level picker");
assert.strictEqual(hooks.buttonTypeRuntimeSpec("light_control").hidden, true, "full light control is grouped under Lights");
assert.strictEqual(hooks.defaultButtonTypeForPicker("light_brightness"), "light_control", "lights picker defaults to all controls");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("climate", false), true, "climate picker visible on parent page");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("climate_control", false), false, "all controls climate subtype hidden from top-level picker");
assert.strictEqual(hooks.buttonTypeRuntimeSpec("climate_control").label, "All Controls", "all controls climate subtype has its own label");
assert.strictEqual(hooks.buttonTypeRuntimeSpec("climate_control").pickerKey, "climate", "all controls climate subtype is grouped under Climate");
assert.strictEqual(hooks.defaultButtonTypeForPicker("climate"), "climate_control", "climate picker defaults to all controls");
assert.strictEqual(hooks.defaultButtonTypeForPicker("cover"), "cover", "ungrouped picker entries keep their own type");
assert.strictEqual(
  hooks.buttonTypePickerKeysFor(false, "light_brightness").indexOf("light_brightness") >= 0,
  true,
  "saved light subtypes remain represented by the lights picker");
assert.strictEqual(
  hooks.buttonTypePickerKeysFor(true, "light_brightness").indexOf("light_brightness") >= 0,
  true,
  "saved light subtypes remain represented in subpages");

assertButtonMigration(hooks, "light switch clears ignored fields", "light.living_room;Living Room;Auto;Auto;sensor.living_room_power;W;light_switch;1;confirm_off", {
  entity: "light.living_room",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  type: "light_switch",
});

assertButtonRoundTrip(hooks, "fan switch card", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan Off",
  icon_on: "Fan",
  sensor: "",
  unit: "",
  type: "fan_switch",
  precision: "",
  options: "",
}, false);

assertButtonRoundTrip(hooks, "fan speed card", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan Speed 2",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "fan_speed",
  precision: "",
  options: "",
}, false);

assertButtonRoundTrip(hooks, "fan control modal card", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "fan_control",
  precision: "",
  options: "",
}, false);

assertButtonRoundTrip(hooks, "fan control modal custom tabs", {
  entity: "fan.bedroom",
  label: "Bedroom Fan",
  icon: "Fan",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "fan_control",
  precision: "",
  options: "fan_tabs=speed%7Cpower%7Cdirection",
}, false);

assert.strictEqual(
  hooks.normalizeFanControlOptions("fan_tabs=bad%7Cspeed%7Cpower%7Cspeed"),
  "fan_tabs=speed%7Cpower",
  "fan control tabs normalize invalid and duplicate values"
);

assertButtonRoundTrip(hooks, "fan oscillation card", {
  entity: "fan.bedroom",
  label: "Oscillation",
  icon: "Fan",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "fan_oscillate",
  precision: "",
  options: "",
}, false);

assertButtonRoundTrip(hooks, "fan direction card", {
  entity: "fan.bedroom",
  label: "Direction",
  icon: "Swap Horizontal",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "fan_direction",
  precision: "",
  options: "",
}, false);

assertButtonRoundTrip(hooks, "fan preset card", {
  entity: "fan.bedroom",
  label: "Preset",
  icon: "Fan Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "fan_preset",
  precision: "",
  options: "",
}, false);

assertButtonMigration(hooks, "fan card clears ignored fields", "fan.bedroom;Bedroom;Auto;Fan;sensor.temp;W;fan_direction;2;large_numbers", {
  entity: "fan.bedroom",
  label: "Bedroom",
  icon: "Swap Horizontal",
  icon_on: "Auto",
  type: "fan_direction",
});

assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_speed", false), true, "fan picker visible on parent page");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_speed", true), true, "fan picker visible in subpages");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("image", false), true, "image picker visible on parent page");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("image", true), true, "image picker visible in subpages");
assert.deepStrictEqual(Array.from(hooks.imageModalModeValues()), ["fill", "fit"], "image modal mode values are contract-backed");
assert.deepStrictEqual(Array.from(hooks.cardContractDomains("image")), ["camera", "image"], "image cards accept camera and image entities");
assert.strictEqual(hooks.normalizeImageOptions("image_refresh=30,image_refresh_mode=timer,unknown=1"), "", "legacy image refresh options are dropped");
assert.strictEqual(hooks.normalizeImageOptions("image_label,image_refresh=30,image_refresh_mode=timer,unknown=1"), "image_label", "image label option is preserved while legacy refresh values are dropped");
assert.strictEqual(hooks.normalizeImageOptions("image_modal_mode=fit,image_refresh=30,image_refresh_mode=timer,unknown=1"), "image_modal_mode=fit", "image modal fit option is preserved while legacy refresh values are dropped");
assert.strictEqual(hooks.normalizeImageOptions("image_modal_mode=fill,image_refresh=30"), "", "image modal fill and legacy refresh options are omitted");
assert.strictEqual(hooks.normalizeImageOptions("image_modal_mode=bad,image_refresh=30"), "", "invalid image modal mode and legacy refresh options are dropped");
assert.strictEqual(hooks.normalizeImageOptions("image_label,image_refresh=5,image_refresh_mode=bad"), "image_label", "image label option survives invalid legacy refresh values");
for (const fixture of imageCardNormalizationFixtures) {
  const parsed = buttonShape(hooks.parseButtonConfig(fixture.input));
  assert.deepStrictEqual(parsed, buttonShape(fixture.expected), `image fixture ${fixture.name}: web parse`);
  const canonical = hooks.serializeButtonConfig(parsed);
  assert.deepStrictEqual(
    buttonShape(hooks.parseButtonConfig(canonical)),
    buttonShape(fixture.expected),
    `image fixture ${fixture.name}: web canonical round-trip`
  );
}
const imageCardForLimit = {
  entity: "camera.front_door",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "image",
  precision: "",
  options: "",
};
const switchCardForImageLimit = {
  entity: "switch.kitchen",
  label: "Kitchen",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "",
  precision: "",
  options: "",
};
const imageLimitSnapshot = {
  grid: [1, 2, 3, 0],
  buttons: [imageCardForLimit, imageCardForLimit, switchCardForImageLimit, imageCardForLimit],
  subpages: {
    4: {
      grid: [1, 2, 3, 4, 0],
      buttons: [imageCardForLimit, imageCardForLimit, imageCardForLimit, imageCardForLimit],
    },
  },
};
assert.strictEqual(hooks.imageCardLimit(), 6, "image card editor limit matches the built device profile");
assert.strictEqual(hooks.imageCardCountForTest(imageLimitSnapshot), 6, "image card count spans main page and subpages");
assert.strictEqual(hooks.imageCardCandidateAllowedForTest(imageLimitSnapshot, {
  isSub: false,
  slot: 5,
  button: imageCardForLimit,
}), false, "saving a seventh image card on the main page is blocked");
assert.strictEqual(hooks.imageCardCandidateAllowedForTest(imageLimitSnapshot, {
  isSub: true,
  homeSlot: 4,
  slot: 5,
  button: imageCardForLimit,
}), false, "saving a seventh image card on a subpage is blocked");
assert.strictEqual(hooks.imageCardCandidateAllowedForTest(imageLimitSnapshot, {
  isSub: false,
  slot: 2,
  button: switchCardForImageLimit,
}), true, "saving a non-image card does not consume a firmware image slot");
assertButtonRoundTrip(hooks, "image card default options", {
  entity: "camera.front_door",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "image",
  precision: "",
  options: "",
}, false);
assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig(
  "~camera.front_door,,Auto,Auto,,,image,,image_refresh=30%2Cimage_refresh_mode=timer"
)), buttonShape({
  entity: "camera.front_door",
  type: "image",
}), "image card legacy refresh options are cleaned up by web parser");
assertButtonRoundTrip(hooks, "image card label option", {
  entity: "camera.front_door",
  label: "Front Door",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "image",
  precision: "",
  options: "image_label",
}, false);
assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig(
  "~camera.front_door,Front%20Door,Auto,Auto,,,image,,image_label%2Cimage_refresh=30%2Cimage_refresh_mode=timer"
)), buttonShape({
  entity: "camera.front_door",
  label: "Front Door",
  type: "image",
  options: "image_label",
}), "image card label survives legacy refresh cleanup");
assertButtonRoundTrip(hooks, "image card icon option", {
  entity: "camera.front_door",
  label: "",
  icon: "Camera",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "image",
  precision: "",
  options: "image_icon",
}, false);
assertButtonRoundTrip(hooks, "image card label and icon options", {
  entity: "camera.front_door",
  label: "Front Door",
  icon: "Camera",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "image",
  precision: "",
  options: "image_label,image_icon",
}, false);
assertButtonMigration(hooks, "image card clears label without overlay option", "camera.front_door;Front Door;Auto;Auto;;;image;;", {
  entity: "camera.front_door",
  label: "",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "",
  unit: "",
  type: "image",
  precision: "",
  options: "",
});
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_switch", false), false, "fan subtype hidden from top-level picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_control", false), false, "fan modal subtype hidden from top-level picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_control", true), false, "fan modal subtype hidden from subpage picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_switch", true), false, "fan switch subtype hidden from subpage picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_oscillate", true), false, "fan oscillation subtype hidden from subpage picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_direction", true), false, "fan direction subtype hidden from subpage picker");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("fan_preset", true), false, "fan preset subtype hidden from subpage picker");
assert.strictEqual(
  hooks.buttonTypePickerKeysFor(false, "fan_speed").indexOf("fan_speed") >= 0,
  true,
  "fan type remains selectable");
assert.strictEqual(hooks.buttonTypeRuntimeSpec("todo"), null, "todo card type is removed from the webserver");
assert.strictEqual(hooks.buttonTypeVisibleInPickerFor("todo", false), false, "todo picker is removed");
assert.deepStrictEqual(Array.from(hooks.cardContractDomains("todo")), [], "todo card has no webserver entity contract");
assert.strictEqual(hooks.cardLargeNumbersEnabled({ type: "todo", options: "large_numbers" }), false, "todo no longer supports webserver large numbers");

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
const subpageLightsPreset = buttonShape({
  entity: "light.living_room",
  label: "Lighting",
  icon: "Lightbulb",
  icon_on: "Auto",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=lights",
});
const subpageMediaPreset = buttonShape({
  entity: "media_player.living_room",
  label: "Media",
  icon: "Speaker",
  icon_on: "Auto",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=media",
});
const subpageClimatePreset = buttonShape({
  entity: "climate.living_room",
  label: "Climate",
  icon: "Thermostat",
  icon_on: "Auto",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=climate",
});
const subpagePresencePreset = buttonShape({
  entity: "person.jane",
  label: "Presence",
  icon: "Account",
  icon_on: "Auto",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=presence",
});
const subpageCustomPreset = buttonShape({
  entity: "climate.living_room",
  label: "Downstairs",
  icon: "Home",
  icon_on: "Auto",
  sensor: "indicator",
  type: "subpage",
  options: "subpage_kind=climate",
});

assertButtonRoundTrip(hooks, "subpage state off", subpageStateOff, false);
assertButtonRoundTrip(hooks, "subpage state icon", subpageStateIcon, false);
assertButtonRoundTrip(hooks, "subpage state icon entity", subpageStateIconEntity, false);
assertButtonRoundTrip(hooks, "subpage state numeric", subpageStateNumeric, false);
assertButtonRoundTrip(hooks, "subpage state numeric precision", subpageStateNumericPrecision, false);
assertButtonRoundTrip(hooks, "subpage state text", subpageStateText, false);
assertButtonRoundTrip(hooks, "subpage lights preset", subpageLightsPreset, false);
assertButtonRoundTrip(hooks, "subpage media preset", subpageMediaPreset, false);
assertButtonRoundTrip(hooks, "subpage climate preset", subpageClimatePreset, false);
assertButtonRoundTrip(hooks, "subpage presence preset", subpagePresencePreset, false);
assertButtonRoundTrip(hooks, "subpage custom preset label and icon", subpageCustomPreset, false);

assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateOff), "off", "subpage state off");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateIcon), "icon", "subpage icon state");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateIconEntity), "icon", "subpage icon entity state");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateNumeric), "numeric", "subpage numeric state");
assert.strictEqual(hooks.subpageStateDisplayMode(subpageStateText), "text", "subpage text state");
assert.strictEqual(hooks.subpageKind(subpageStateOff), "", "generic subpage has no preset kind");
assert.strictEqual(hooks.subpageKind(subpageLightsPreset), "lights", "lights subpage preset kind");
assert.strictEqual(hooks.subpageKind(subpageMediaPreset), "media", "media subpage preset kind");
assert.strictEqual(hooks.subpageKind(subpageClimatePreset), "climate", "climate subpage preset kind");
assert.strictEqual(hooks.subpageKind(subpagePresencePreset), "presence", "presence subpage preset kind");
assert.deepStrictEqual(buttonShape(hooks.parseButtonConfig(
  "media_player.bad;Bad;Speaker;Auto;indicator;;subpage;;subpage_kind=audio"
)), buttonShape({
  entity: "media_player.bad",
  label: "Bad",
  icon: "Speaker",
  icon_on: "Auto",
  sensor: "indicator",
  type: "subpage",
}), "invalid subpage preset kind normalizes back to generic");

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

assertButtonMigration(hooks, "legacy local sensor card", "room_temp;Living Room;Auto;Thermometer;;°C;local_sensor;1;large_numbers", {
  entity: "room_temp",
  label: "Living Room",
  icon: "Auto",
  icon_on: "Auto",
  sensor: "local",
  unit: "°C",
  type: "sensor",
  precision: "1",
  options: "",
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

const scriptActionStateCard = {
  entity: "script.kitchen_lights",
  label: "Kitchen Lights",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "state_entity=light.kitchen",
};
assertButtonRoundTrip(hooks, "script action card with state entity", scriptActionStateCard, false);
assert.strictEqual(
  hooks.actionCardStateEntity(hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionStateCard))),
  "light.kitchen",
  "action card state entity option"
);
assert.strictEqual(
  hooks.actionCardStateDisplayMode(hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionStateCard))),
  "text",
  "legacy action card state entity defaults to text display in editor"
);

const scriptActionNumericStateCard = {
  entity: "script.kitchen_lights",
  label: "Kitchen Lights",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "state_entity=sensor.kitchen_power,state_unit=W,state_precision=1",
};
assertButtonRoundTrip(hooks, "script action card with numeric state display", scriptActionNumericStateCard, false);
const parsedActionNumericState = hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionNumericStateCard));
assert.strictEqual(hooks.actionCardStateDisplayMode(parsedActionNumericState), "numeric", "action card numeric state mode");
assert.strictEqual(hooks.actionCardStateUnit(parsedActionNumericState), "W", "action card numeric state unit");
assert.strictEqual(hooks.actionCardStatePrecision(parsedActionNumericState), "1", "action card numeric state precision");

const scriptActionZeroPrecisionStateCard = {
  entity: "script.kitchen_lights",
  label: "Kitchen Lights",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "state_entity=sensor.kitchen_power,state_precision=0",
};
assertButtonRoundTrip(hooks, "script action card with zero-precision numeric state display", scriptActionZeroPrecisionStateCard, false);
const parsedActionZeroPrecisionState = hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionZeroPrecisionStateCard));
assert.strictEqual(hooks.actionCardStateDisplayMode(parsedActionZeroPrecisionState), "numeric", "action card zero-precision numeric state mode");
assert.strictEqual(hooks.actionCardStatePrecision(parsedActionZeroPrecisionState), "0", "action card zero-precision numeric state precision");

const scriptActionTextStateCard = {
  entity: "script.washer",
  label: "Washer",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "state_entity=text_sensor.washer_state,state_precision=text",
};
assertButtonRoundTrip(hooks, "script action card with text state display", scriptActionTextStateCard, false);
const parsedActionTextState = hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionTextStateCard));
assert.strictEqual(hooks.actionCardStateDisplayMode(parsedActionTextState), "text", "action card text state mode");
assert.strictEqual(hooks.actionCardStatePrecision(parsedActionTextState), "text", "action card text state precision");

const scriptActionIconStateCard = {
  entity: "script.goodnight",
  label: "Goodnight",
  icon: "Flash",
  icon_on: "Check Circle",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "state_entity=input_boolean.goodnight_ready,state_precision=icon",
};
assertButtonRoundTrip(hooks, "script action card with icon state display", scriptActionIconStateCard, false);
const parsedActionIconState = hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionIconStateCard));
assert.strictEqual(hooks.actionCardStateDisplayMode(parsedActionIconState), "icon", "action card icon state mode");
assert.strictEqual(hooks.actionCardStatePrecision(parsedActionIconState), "icon", "action card icon state precision");

const scriptActionFieldsCard = {
  entity: "script.goodnight",
  label: "Goodnight",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "script.turn_on",
  unit: "",
  type: "action",
  precision: "",
  options: "script_fields=room%3A kitchen\nmode=night",
};
assertButtonRoundTrip(hooks, "script action card with fields", scriptActionFieldsCard, false);
const parsedScriptActionFields = hooks.parseButtonConfig(hooks.serializeButtonConfig(scriptActionFieldsCard));
assert.strictEqual(
  hooks.actionScriptFields(parsedScriptActionFields),
  "room: kitchen\nmode=night",
  "script action fields round-trip"
);
hooks.setActionScriptFields(parsedScriptActionFields, "level: 4");
assert.strictEqual(hooks.actionScriptFields(parsedScriptActionFields), "level: 4", "script action fields update");
parsedScriptActionFields.options = "script_fields=level: 4,confirm_on,confirm_message=Run level?";
parsedScriptActionFields.sensor = "scene.turn_on";
const parsedNonScriptActionFields = hooks.parseButtonConfig(hooks.serializeButtonConfig(parsedScriptActionFields));
assert.strictEqual(parsedNonScriptActionFields.options || "", "", "script-only options are removed from non-script actions");

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

assertButtonRoundTrip(hooks, "local action subtype card", {
  entity: "zoom_mute",
  label: "Zoom Mute",
  icon: "Gesture Tap",
  icon_on: "Auto",
  sensor: "local",
  unit: "",
  type: "action",
  precision: "",
}, false);

const parsedLocalActionSubtype = hooks.parseButtonConfig("zoom_mute;Zoom Mute;Gesture Tap;Auto;local;;action");
assert.strictEqual(parsedLocalActionSubtype.type, "action", "local action subtype remains an action card");
assert.strictEqual(hooks.actionCardIsLocal(parsedLocalActionSubtype), true, "local action subtype is detected");

assertButtonMigration(hooks, "legacy local action card becomes action subtype", "zoom_mute;Zoom Mute;Auto;Auto;;;local;;state_entity=sensor.stale", {
  entity: "zoom_mute",
  label: "Zoom Mute",
  icon: "Gesture Tap",
  icon_on: "Auto",
  sensor: "local",
  unit: "",
  type: "action",
  precision: "",
});

assertButtonMigration(hooks, "legacy vacuum start action card", "vacuum.k11_vacuum_784c;Vacuum Bath;Robot Vacuum;Auto;vacuum.start;;action", {
  entity: "vacuum.k11_vacuum_784c",
  label: "Vacuum Bath",
  icon: "Robot Vacuum",
  icon_on: "Auto",
  sensor: "start_stop",
  unit: "",
  type: "vacuum",
  precision: "",
});

assertButtonMigration(hooks, "legacy vacuum return to base action card", "vacuum.k11_vacuum_784c;Dock Vacuum;Robot Vacuum;Auto;vacuum.return_to_base;;action", {
  entity: "vacuum.k11_vacuum_784c",
  label: "Dock Vacuum",
  icon: "Robot Vacuum",
  icon_on: "Auto",
  sensor: "dock",
  unit: "",
  type: "vacuum",
  precision: "",
});

[
  ["status", ""],
  ["start_stop", ""],
  ["dock", ""],
  ["pause_resume", ""],
  ["clean_spot", ""],
  ["locate", ""],
  ["clean_area", "kitchen"],
].forEach(([mode, unit]) => {
  assertButtonRoundTrip(hooks, `vacuum ${mode} card`, {
    entity: "vacuum.k11_vacuum_784c",
    label: "Vacuum",
    icon: "Robot Vacuum",
    icon_on: "Auto",
    sensor: mode,
    unit,
    type: "vacuum",
    precision: "",
  }, false);
});

[
  "status",
  "start_mowing",
  "dock",
  "pause_resume",
].forEach((mode) => {
  assertButtonRoundTrip(hooks, `lawn mower ${mode} card`, {
    entity: "lawn_mower.backyard",
    label: "Backyard Mower",
    icon: "Robot Mower",
    icon_on: "Auto",
    sensor: mode,
    unit: "",
    type: "lawn_mower",
    precision: "",
  }, false);
});

assertButtonMigration(hooks, "invalid lawn mower mode normalizes to start mowing", "lawn_mower.backyard;Backyard Mower;Auto;Auto;bad_mode;;lawn_mower", {
  entity: "lawn_mower.backyard",
  label: "Backyard Mower",
  icon: "Robot Mower",
  icon_on: "Auto",
  sensor: "start_mowing",
  unit: "",
  type: "lawn_mower",
  precision: "",
});

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

assertButtonRoundTrip(hooks, "input select option action card", {
  entity: "input_select.house_mode",
  label: "House Mode",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_select.select_option",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonRoundTrip(hooks, "select option action card", {
  entity: "select.wled_preset",
  label: "WLED Preset",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_select.select_option",
  unit: "",
  type: "action",
  precision: "",
}, false);

assertButtonMigration(hooks, "legacy option select becomes action subtype", "select.wled_preset;WLED Preset;Chevron Down;Lightbulb;sensor.stale;%;option_select;2;large_numbers", {
  entity: "select.wled_preset",
  label: "WLED Preset",
  icon: "Flash",
  icon_on: "Auto",
  sensor: "input_select.select_option",
  unit: "",
  type: "action",
  precision: "",
});

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

const backOnlySubpageEncoded = assertSubpageRoundTrip(hooks, "back-only subpage", {
  order: ["B"],
  buttons: [],
}, false);
assert.strictEqual(backOnlySubpageEncoded, "B", "back-only subpage keeps a non-empty config");

assertSubpageRoundTrip(hooks, "moved back-only subpage", {
  order: ["", "", "", "B"],
  buttons: [],
}, false);

const backOnlyFromGrid = hooks.serializeSubpageConfig({
  order: [],
  grid: [-2].concat(Array(19).fill(0)),
  sizes: {},
  buttons: [],
});
assert.strictEqual(backOnlyFromGrid, "B", "new back-only subpage serializes from its grid");

assertSubpageRoundTrip(hooks, "date time large numbers subpage", {
  order: ["1", "B", "2", "3"],
  buttons: [
    buttonShape({ type: "calendar", precision: "datetime", options: "large_numbers" }),
    buttonShape({ type: "clock", options: "large_numbers" }),
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

assertSubpageRoundTrip(hooks, "garage command subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "cover.garage", label: "Open", icon: "Garage Open", icon_on: "Auto", sensor: "open", type: "garage" }),
    buttonShape({ entity: "cover.garage", label: "Close", icon: "Garage", icon_on: "Auto", sensor: "close", type: "garage" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "garage status subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.garage", label: "Garage Door", icon: "Garage", icon_on: "Garage Open", type: "garage", options: "label_display=status" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "gate command subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "cover.gate", label: "Open", icon: "Gate Open", icon_on: "Auto", sensor: "open", type: "gate" }),
    buttonShape({ entity: "cover.gate", label: "Stop", icon: "Stop", icon_on: "Auto", sensor: "stop", type: "gate" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "gate status subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.gate", label: "Gate", icon: "Gate", icon_on: "Gate Open", type: "gate", options: "label_display=status" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "action subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "scene.movie_mode", label: "Movie Mode", icon: "Flash", sensor: "scene.turn_on", type: "action", options: "state_entity=light.living_room" }),
    buttonShape({ entity: "input_select.house_mode", label: "House Mode", icon: "Flash", sensor: "input_select.select_option", type: "action" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "option select action subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "input_select.house_mode", label: "House Mode", icon: "Flash", icon_on: "Auto", sensor: "input_select.select_option", type: "action" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "lock subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "lock.front_door", label: "Front Door", icon: "Lock", icon_on: "Lock Open", type: "lock" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "lock command subpage", {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "lock.front_door", label: "Lock", icon: "Lock", icon_on: "Auto", sensor: "lock", type: "lock" }),
    buttonShape({ entity: "lock.front_door", label: "Unlock", icon: "Lock Open", icon_on: "Auto", sensor: "unlock", type: "lock" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "alarm action subpage", {
  order: ["B", "1", "2"],
  buttons: [
    buttonShape({ entity: "alarm_control_panel.house", label: "Arm Away", icon: "Shield Lock", icon_on: "Auto", sensor: "away", type: "alarm_action", options: "pin_arm=0" }),
    buttonShape({ entity: "alarm_control_panel.house", label: "Disarm", icon: "Shield Off", icon_on: "Auto", sensor: "disarm", type: "alarm_action" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "media subpage", {
  order: ["1", "B", "2", "3", "4", "5", "6", "7"],
  buttons: [
    buttonShape({ entity: "media_player.living_room", label: "Play/Pause", icon: "Auto", sensor: "play_pause", type: "media" }),
    buttonShape({ entity: "media_player.living_room", label: "Previous", icon: "Auto", sensor: "previous", type: "media" }),
    buttonShape({ entity: "media_player.living_room", label: "Next", icon: "Auto", sensor: "next", type: "media" }),
    buttonShape({ entity: "media_player.kitchen", label: "Kitchen", icon: "Auto", sensor: "volume", type: "media", options: "volume_max=40" }),
    buttonShape({ entity: "media_player.office", label: "Office", icon: "Progress Clock", sensor: "position", type: "media" }),
    buttonShape({ entity: "media_player.office", label: "", icon: "Auto", sensor: "now_playing", type: "media" }),
    buttonShape({ entity: "media_player.office", label: "Morning Mix", icon: "Music", sensor: "playlist", type: "media", options: "playlist_content_id=spotify%3Aplaylist%3A12345" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.living_room", label: "Living Room", type: "climate_control", precision: "1" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage custom range", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", type: "climate_control", precision: "0:16:30" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage negative custom range", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.freezer", label: "Freezer", type: "climate_control", precision: "1:-25:5" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage display options", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", type: "climate_control", precision: "1", options: "label_display=target,number_display=actual" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "climate subpage icon display", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", icon: "Thermostat", icon_on: "Radiator", type: "climate_control", precision: "1", options: "number_display=icon" }),
  ],
}, true);

const climateIconSubpage = hooks.serializeSubpageConfig({
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", icon: "Thermostat", icon_on: "Radiator", type: "climate_control", precision: "1", options: "number_display=icon" }),
  ],
});
assert.deepStrictEqual(
  Array.from(hooks.subpageChunkPostKeysFor(climateIconSubpage, { ext: "|A,scene.old,Old,Flash,,scene.turn_on" }, "")),
  ["subpage_config", "subpage_config_ext"],
  "saving a shorter climate subpage clears stale extension chunks"
);
assert.deepStrictEqual(
  Array.from(hooks.subpageChunkPostKeysFor(climateIconSubpage, {}, climateIconSubpage + "|A," + "scene.old_action_with_long_tail_".repeat(8))),
  ["subpage_config", "subpage_config_ext"],
  "saving over a pending longer subpage clears stale extension chunks"
);

assertSubpageRoundTrip(hooks, "light temperature subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "light.living_room", label: "Living Room", icon: "Auto", sensor: "kelvin", unit: "2000-6500", type: "light_temperature", precision: "color" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "light brightness subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "light.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", type: "light_brightness" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "light switch subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "light.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", type: "light_switch" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "fan switch subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Bedroom Fan", icon: "Fan Off", icon_on: "Fan", type: "fan_switch" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "fan speed subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Bedroom Fan", icon: "Fan Speed 2", icon_on: "Auto", type: "fan_speed" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "fan control modal subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Bedroom Fan", icon: "Fan", icon_on: "Auto", type: "fan_control", options: "fan_tabs=speed%7Cpower" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "fan oscillation subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Oscillation", icon: "Fan", icon_on: "Auto", type: "fan_oscillate" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "fan direction subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Direction", icon: "Swap Horizontal", icon_on: "Auto", type: "fan_direction" }),
  ],
}, true);

assertSubpageRoundTrip(hooks, "fan preset subpage", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Preset", icon: "Fan Auto", icon_on: "Auto", type: "fan_preset" }),
  ],
}, true);

assertSubpageMigration(hooks, "fan subpage clears ignored fields", "~1,B|E,fan.bedroom,Bedroom,Auto,Fan,sensor.temp,W,2,large_numbers", {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Bedroom", icon: "Swap Horizontal", icon_on: "Auto", type: "fan_direction" }),
  ],
});

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

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|X,,Kitchen%20Window,Window%20Closed,Window%20Open,binary_sensor.kitchen_window,,window,active_color")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ label: "Kitchen Window", icon: "Window Closed", icon_on: "Window Open", sensor: "binary_sensor.kitchen_window", type: "door_window", precision: "window", options: "active_color" }),
  ],
}, "compact door/window subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|D")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ type: "calendar" }),
  ],
}, "compact calendar subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|CK")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ type: "clock" }),
  ],
}, "compact clock subpage parse");

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

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B,2|R,cover.garage,Open,Garage%20Open,,open|R,cover.garage,Close,Garage,,close")), {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "cover.garage", label: "Open", icon: "Garage Open", icon_on: "Auto", sensor: "open", type: "garage" }),
    buttonShape({ entity: "cover.garage", label: "Close", icon: "Garage", icon_on: "Auto", sensor: "close", type: "garage" }),
  ],
}, "compact garage command subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|GT,cover.gate,,Gate,Gate%20Open")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.gate", icon: "Gate", icon_on: "Gate Open", type: "gate" }),
  ],
}, "compact gate subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|GT,cover.gate,Gate,Gate,Gate%20Open")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "cover.gate", label: "Gate", icon: "Gate", icon_on: "Gate Open", type: "gate" }),
  ],
}, "compact gate label subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B,2|GT,cover.gate,Open,Gate%20Open,,open|GT,cover.gate,Stop,Stop,,stop")), {
  order: ["1", "B", "2"],
  buttons: [
    buttonShape({ entity: "cover.gate", label: "Open", icon: "Gate Open", icon_on: "Auto", sensor: "open", type: "gate" }),
    buttonShape({ entity: "cover.gate", label: "Stop", icon: "Stop", icon_on: "Auto", sensor: "stop", type: "gate" }),
  ],
}, "compact gate command subpage parse");

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

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|A,scene.movie_mode,Movie%20Mode,Flash,,scene.turn_on,,,state_entity=light.living_room")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "scene.movie_mode", label: "Movie Mode", icon: "Flash", icon_on: "Auto", sensor: "scene.turn_on", type: "action", options: "state_entity=light.living_room" }),
  ],
}, "compact action subpage state entity parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|WH,https%3A//webhook-test.net/abc,Door%20Alert,,,POST,%7B%22value%22%3Atrue%7D,,webhook_headers=Content-Type%253A%20application/json")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({
      entity: "https://webhook-test.net/abc",
      label: "Door Alert",
      icon: "Auto",
      icon_on: "Auto",
      sensor: "POST",
      unit: "{\"value\":true}",
      type: "webhook",
      options: "webhook_headers=Content-Type%3A application/json",
    }),
  ],
}, "compact webhook subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|U,input_select.house_mode,House%20Mode,Chevron%20Down")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "input_select.house_mode", label: "House Mode", icon: "Flash", icon_on: "Auto", sensor: "input_select.select_option", type: "action" }),
  ],
}, "compact option select subpage migration");

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

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|M,media_player.office,Morning%20Mix,Music,,playlist,,,playlist_content_id=spotify%253Aplaylist%253A12345")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "media_player.office", label: "Morning Mix", icon: "Music", icon_on: "Auto", sensor: "playlist", type: "media", options: "playlist_content_id=spotify%3Aplaylist%3A12345" }),
  ],
}, "compact media playlist subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|M,media_player.living_room,Living%20Room,Speaker,,controls")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "media_player.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", sensor: "play_pause", type: "media" }),
  ],
}, "legacy media controls subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|H,climate.living_room,Living%20Room,,,,,1")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.living_room", label: "Living Room", type: "climate_control", precision: "1" }),
  ],
}, "compact climate subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|H,climate.hallway,Hallway,,,,,0%3A16%3A30")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "climate.hallway", label: "Hallway", type: "climate_control", precision: "0:16:30" }),
  ],
}, "compact climate range subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|N,light.living_room,Living%20Room,,,kelvin,2000-6500,color")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "light.living_room", label: "Living Room", icon: "Auto", icon_on: "Auto", sensor: "kelvin", unit: "2000-6500", type: "light_temperature", precision: "color" }),
  ],
}, "compact light temperature subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|B,fan.bedroom,Bedroom%20Fan,Fan%20Off,Fan")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Bedroom Fan", icon: "Fan Off", icon_on: "Fan", type: "fan_switch" }),
  ],
}, "compact fan switch subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|J,fan.bedroom,Bedroom%20Fan,Fan%20Speed%202")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Bedroom Fan", icon: "Fan Speed 2", icon_on: "Auto", type: "fan_speed" }),
  ],
}, "compact fan speed subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|O,fan.bedroom,Oscillation,Fan")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Oscillation", icon: "Fan", icon_on: "Auto", type: "fan_oscillate" }),
  ],
}, "compact fan oscillation subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|E,fan.bedroom,Direction,Swap%20Horizontal")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Direction", icon: "Swap Horizontal", icon_on: "Auto", type: "fan_direction" }),
  ],
}, "compact fan direction subpage parse");

assert.deepStrictEqual(subpageShape(hooks.parseSubpageConfig("~1,B|Z,fan.bedroom,Preset,Fan%20Auto")), {
  order: ["1", "B"],
  buttons: [
    buttonShape({ entity: "fan.bedroom", label: "Preset", icon: "Fan Auto", icon_on: "Auto", type: "fan_preset" }),
  ],
}, "compact fan preset subpage parse");

const issue248EightCardConfig = "~B,,4,2,3,,,,8,9,,,1,6,5|X,,Office,Window Closed,Window Open,binary_sensor.office_window_sensor_opening,,window,active_color|X,,Linnea 1,Window Closed,Window Open,binary_sensor.linnea_br_window_sensor_opening,,window,active_color|X,,Linnea 2,Window Closed,Window Open,binary_sensor.linnea_br_window_2_sensor_opening,,window,active_color|X,,Maxime,Window Closed,Window Open,binary_sensor.maxime_br_window_sensor_opening,,window,active_color|X,,Study 2,Window Closed,Window Open,binary_sensor.study_window_2_sensor_opening,,window,active_color|X,,Study 1,Window Closed,Window Open,binary_sensor.study_window_2_sensor_opening,,window,active_color||X,,Master 1,Window Closed,Window Open,binary_sensor.master_bedroom_window_1_sensor,,window,active_color|X,,Master 2,Window Closed,Window Open,binary_sensor.master_bedroom_window_2_sensor,,window,active_color";
const issue248NineCardConfig = issue248EightCardConfig +
  "|X,,Kitchen,Window Closed,Window Open,binary_sensor.kitchen_window_sensor_opening,,window,active_color";
const issue248NineCardChunks = hooks.splitSubpageConfigChunks(issue248NineCardConfig, 4, 255);
assert(issue248NineCardChunks, "issue 248 nine-card config should fit in four fixed chunks");
assert(issue248NineCardChunks.some((chunk) => chunk.length === 255), "issue 248 chunks may split inside card data");
assert.deepStrictEqual(
  subpageShape(hooks.parseSubpageConfig(issue248NineCardChunks.join(""))),
  subpageShape(hooks.parseSubpageConfig(issue248NineCardConfig)),
  "issue 248 fixed chunks reassemble before web parse"
);

const fullJc1060DoorWindowSubpage = {
  order: ["B", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14"],
  buttons: Array.from({ length: 14 }, (_, i) => {
    const names = [
      "office_front_window_sensor_opening",
      "linnea_bedroom_left_window_sensor_opening",
      "linnea_bedroom_right_window_sensor_opening",
      "maxime_bedroom_window_sensor_opening",
      "study_left_window_sensor_opening",
      "study_right_window_sensor_opening",
      "master_bedroom_left_window_sensor_opening",
      "master_bedroom_right_window_sensor_opening",
      "kitchen_patio_door_sensor_opening",
      "living_room_wide_window_sensor_opening",
      "utility_side_door_sensor_opening",
      "guest_bedroom_window_sensor_opening",
      "loft_roof_window_sensor_opening",
      "garage_internal_door_sensor_opening",
    ];
    return buttonShape({
      label: `Door Window ${i + 1}`,
      icon: "Window Closed",
      icon_on: "Window Open",
      sensor: `binary_sensor.${names[i]}`,
      type: "door_window",
      precision: "window",
      options: "active_color",
    });
  }),
};
const fullJc1060Encoded = assertSubpageRoundTrip(hooks, "full jc1060 door/window subpage", fullJc1060DoorWindowSubpage, true);
assert(fullJc1060Encoded.length > 4 * 255, "full jc1060 subpage should exceed old four-chunk capacity");
assert(fullJc1060Encoded.length <= 8 * 255, "full jc1060 subpage should fit eight chunks");
const fullJc1060Chunks = hooks.splitSubpageConfigChunks(fullJc1060Encoded, 8, 255);
assert(fullJc1060Chunks, "full jc1060 subpage should split into eight chunks");
assert.deepStrictEqual(
  subpageShape(hooks.parseSubpageConfig(fullJc1060Chunks.join(""))),
  subpageShape(fullJc1060DoorWindowSubpage),
  "full jc1060 chunks reassemble before web parse"
);

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

console.log("Config format current tests passed.");
