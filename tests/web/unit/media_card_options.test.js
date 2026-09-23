"use strict";

const assert = require("node:assert/strict");
const path = require("node:path");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const root = path.resolve(__dirname, "../../..");
const { createConfigMediaOptionsFeature } = loadTypeScriptModule(
  path.join(root, "src/webserver/application/config_media_options.ts"));
const { normalizeSavedConfigMediaShadow } = loadTypeScriptModule(
  path.join(root, "src/webserver/generated/saved_config_shadow.ts"));
const { decodeMediaCardConfigV1 } = loadTypeScriptModule(
  path.join(root, "src/webserver/model/media_card.ts"));
const feature = createConfigMediaOptionsFeature({ disabledCardTypes: [] });

for (const [precision, expected] of [["", "none"], ["progress", "play_pause"], ["play_pause", "play_pause"]]) {
  const legacy = { type: "media", sensor: "now_playing", precision, options: "" };
  assert.equal(feature.mediaNowPlayingTapAction(legacy), expected);
  assert.equal(decodeMediaCardConfigV1(legacy).tapAction, expected);
  const restored = normalizeSavedConfigMediaShadow(JSON.parse(JSON.stringify(legacy)));
  assert.equal(feature.mediaNowPlayingTapAction(restored), expected);
  assert.equal(restored.precision, precision);
  for (const action of ["none", "play_pause", "seek"]) {
    const saved = { ...legacy, options: `media_tap_action=${action}` };
    saved.options = feature.normalizeMediaOptions(saved.options, saved.sensor);
    const restored = normalizeSavedConfigMediaShadow(JSON.parse(JSON.stringify(saved)));
    assert.equal(restored.options, `media_tap_action=${action}`);
    assert.equal(feature.mediaNowPlayingTapAction(restored),
      action === "seek" && precision !== "progress" ? "none" : action);
  }
}
assert.equal(feature.normalizeMediaOptions("media_tap_action=bogus", "now_playing"), "");
assert.equal(feature.normalizeMediaOptions("media_tap_action=seek", "volume"), "");
console.log("Media display/action migration and restore tests passed.");
