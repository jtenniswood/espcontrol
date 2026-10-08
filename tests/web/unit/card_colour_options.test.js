"use strict";

const assert = require("node:assert/strict");
const vm = require("node:vm");
const { test } = require("node:test");
const { loadBuiltWebSource } = require("../../../scripts/web_source");

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
  const colour = "card_off_color=6633B9";
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
