"use strict";

const assert = require("node:assert/strict");
const vm = require("node:vm");
const { test } = require("node:test");
const { loadBuiltWebSource } = require("../../../scripts/web_source");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const colours = loadTypescriptTest("src/webserver/model/config_primitives.ts");

test("active colour choices preserve the background and resolve the selected active colour", () => {
  let options = "active_color,card_off_color=3F51B5";
  assert.equal(colours.cardActiveColorMode(options), "matching");
  assert.equal(colours.cardActiveColor(options, "FF8C00"), "8C97D3");
  options = colours.setCardActiveColor(options, "theme");
  assert.equal(colours.cardActiveColor(options, "FF8C00"), "FF8C00");
  assert.equal(colours.cardActiveColor(options, "00BCD4"), "00BCD4");
  options = colours.setCardActiveColor(options, "custom", "#ffffff");
  assert.equal(colours.cardActiveColor(options, "FF8C00"), "FFFFFF");
  options = colours.setCardColor(options, "#000000");
  assert.equal(colours.normalizedCardColorOptions(options), "card_off_color=000000,card_active_color=custom,card_on_color=FFFFFF");
  options = colours.setCardActiveColor(options, "matching");
  assert.equal(colours.cardActiveColor(options, "FF8C00"), "666666");
  assert.equal(colours.normalizedCardColorOptions(options), "card_off_color=000000");
  assert.equal(colours.withoutCardColorOptions(options), "active_color");
  assert.equal(colours.cardActiveColor("card_active_color=custom,card_on_color=bad", "FF8C00"), "FF8C00");
  assert.equal(colours.normalizedCardColorOptions("card_active_color=custom,card_on_color=000000"), "card_active_color=custom,card_on_color=000000");
  assert.equal(colours.normalizedCardColorOptions("card_on_color=00BCD4"), "card_off_color=00BCD4");
});

function loadCodec() {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {}, console, URLSearchParams, TextEncoder, TextDecoder,
    atob, btoa, setTimeout, clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    location: { search: "" },
    document: { readyState: "loading", activeElement: null, addEventListener() {} },
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox);
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config;
}

test("card colours survive other editor option changes", () => {
  const codec = loadCodec();
  const colour = "card_off_color=6633B9,card_active_color=custom,card_on_color=FFFFFF";
  const toggle = { type: "", options: colour };
  codec.setCardOnPattern(toggle, "stripes");
  assert(toggle.options.includes(colour));
  assert(toggle.options.includes("on_pattern=stripes"));
  const media = { type: "media", sensor: "cover_art", options: colour };
  codec.setMediaCoverArtDetailsEnabled(media, true);
  assert(media.options.includes(colour));
  assert(media.options.includes("cover_art_details"));
  codec.setMediaCoverArtDetailsEnabled(media, false);
  assert.equal(media.options, colour);
  const sensor = { type: "sensor", precision: "time", options: colour };
  codec.setSensorTimeUnit(sensor, "seconds");
  assert(sensor.options.includes(colour));
  for (const normalize of [
    () => codec.normalizeMediaOptions(colour, "play_pause"),
    () => codec.normalizeMediaOptions(colour, "control_modal"),
    () => codec.normalizeClimateOptions(colour, true),
    () => codec.normalizeAlarmOptions(colour),
    () => codec.normalizeFanControlOptions(colour),
    () => codec.normalizeCoverOptions(colour),
    () => codec.normalizeLightControlOptions(colour),
    () => codec.normalizeSensorOptions(colour, "text"),
  ]) {
    assert(normalize().includes(colour));
  }
});

test("media colours and modal settings survive changes in both views", () => {
  const codec = loadCodec();
  const colour = "card_off_color=6633B9";
  for (const mode of ["control_modal", "cover_art"]) {
    const media = {
      type: "media", sensor: mode,
      options: colour + ",power_entity=switch.tv,media_tabs=power|volume|controls",
    };
    media.options = codec.normalizeMediaOptions(media.options, mode);
    assert(media.options.includes(colour));
    assert(media.options.includes("power_entity=switch.tv"));
    assert(media.options.includes("media_tabs=power%7Cvolume%7Ccontrols"));
    if (mode === "cover_art") {
      codec.setMediaCoverArtDetailsEnabled(media, true);
      assert(media.options.includes(colour));
      assert(media.options.includes("power_entity=switch.tv"));
      assert(media.options.includes("media_tabs=power%7Cvolume%7Ccontrols"));
    }
    media.sensor = mode === "cover_art" ? "control_modal" : "cover_art";
    media.options = codec.normalizeMediaOptions(media.options, media.sensor);
    assert(media.options.includes(colour));
    assert(media.options.includes("power_entity=switch.tv"));
    assert(media.options.includes("media_tabs=power%7Cvolume%7Ccontrols"));
  }
});


test("back button colours survive saving, reloading, resizing and reset", () => {
  const codec = loadCodec();
  const button = { entity: "switch.test", label: "Test", icon: "Auto", icon_on: "Auto", sensor: "", unit: "", type: "", precision: "", options: "" };
  for (const buttons of [[], [button]]) {
    const subpage = { order: ["Bw", ...(buttons.length ? ["1"] : [])], buttons, backLabel: "Return:Home", backColor: "F5F5F5" };
    let encoded = codec.serializeSubpageConfig(subpage);
    assert(encoded.includes("Bw=Return%3AHome:F5F5F5"));
    let reloaded = codec.parseSubpageConfig(encoded);
    assert.equal(reloaded.backColor, "F5F5F5");
    assert.equal(reloaded.backLabel, "Return:Home");
    reloaded.order[0] = "Bt";
    encoded = codec.serializeSubpageConfig(reloaded);
    assert(encoded.includes("Bt=Return%3AHome:F5F5F5"));
    reloaded.backColor = "";
    encoded = codec.serializeSubpageConfig(reloaded);
    assert(!encoded.includes(":F5F5F5"));
    assert.equal(codec.parseSubpageConfig(encoded).backColor, undefined);
  }
});
