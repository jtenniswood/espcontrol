"use strict";

const assert = require("node:assert/strict");
const vm = require("node:vm");
const { test } = require("node:test");
const { loadBuiltWebSource } = require("../../../scripts/web_source");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

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

test("Webhook normalization retains colour and headers for every HTTP method", () => {
  const { createConfigWebhookOptionsFeature } = loadTypescriptTest("src/webserver/application/config_webhook_options.ts");
  const feature = createConfigWebhookOptionsFeature();
  for (const [method] of feature.methods) {
    const card = { sensor: method, unit: "body", options: "card_off_color=6633B9" };
    feature.setWebhookHeaders(card, '{"X-Test":"value,with|delimiters"}');
    feature.normalizeWebhookConfig(card);
    assert(card.options.includes("card_off_color=6633B9"));
    assert.equal(feature.webhookHeaders(card), '{"X-Test":"value,with|delimiters"}');
    assert.equal(card.unit, ["GET", "DELETE"].includes(method) ? "" : "body");
  }
});
