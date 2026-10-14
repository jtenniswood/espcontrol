"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { cloneCardConfig, cardBackgroundAssetId, cardOptionsWithAppearance } = loadTypescriptTest("src/webserver/model/card.ts");
const { createConfigMediaOptionsFeature } = loadTypescriptTest("src/webserver/application/config_media_options.ts");

test("switching Media to Cover Art clears the typed background permanently", () => {
  const media = createConfigMediaOptionsFeature({ disabledCardTypes: [] });
  const card = cloneCardConfig({ type: "media", sensor: "play_pause", options: "bg_image=saved-image" });
  assert.equal(cardBackgroundAssetId(card), "saved-image");
  card.sensor = "cover_art";
  const previousOptions = card.options;
  const normalized = media.normalizeMediaOptions(card.options, card.sensor, card);
  assert.equal(card.options, previousOptions, "the editor must still detect the options change and save it");
  card.options = normalized;
  assert.equal(card.appearance.backgroundAssetId, "");
  assert.equal(cardBackgroundAssetId(card), "");
  card.sensor = "play_pause";
  card.options = media.normalizeMediaOptions(card.options, card.sensor, card);
  assert.doesNotMatch(cardOptionsWithAppearance(card.options, card), /bg_image=/);
});
