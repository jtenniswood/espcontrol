"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const path = require("node:path");
const vm = require("node:vm");
const esbuild = require("esbuild");

function mediaPreviewFixtures() {
  const root = path.resolve(__dirname, "../../..");
  const bundled = esbuild.buildSync({
    stdin: { contents: `
      export {createPreviewRenderFeature} from "./src/webserver/application/preview_render";
      export {registerMediaCardTypes} from "./src/webserver/cards/media";
      export {createConfigMediaOptionsFeature} from "./src/webserver/application/config_media_options";
      export {createConfigModalTabOptionsFeature} from "./src/webserver/application/config_modal_tab_options";
      export {initializeAppState,state} from "./src/webserver/state/app_instance";
      export {initializeDeviceConfig} from "./src/webserver/device_config";
      export {PREVIEW_THEME_COLORS} from "./src/webserver/state/preview_theme";
      export {cardPreviewTextColor} from "./src/webserver/features/preview";
      export {createWebStyles} from "./src/webserver/application/styles";
    `, resolveDir: root, loader: "ts" },
    bundle: true, format: "cjs", platform: "node", write: false, logLevel: "silent",
  }).outputFiles[0].text;
  const moduleObject = { exports: {} };
  vm.runInNewContext(bundled, {
    module: moduleObject, exports: moduleObject.exports,
    __ESPCONTROL_DEFAULT_DEVICE_ID__: "test",
    __ESPCONTROL_DEVICE_PROFILES__: { test: { slots: 1, cols: 1, rows: 1, features: {}, timezoneOptions: [] } },
    __ESPCONTROL_TIMEZONE_OPTIONS__: [],
  });
  const ui = moduleObject.exports;
  ui.initializeDeviceConfig();
  ui.initializeAppState();
  const registry = { definitions: {}, register(type, definition) { this.definitions[type] = definition; } };
  ui.registerMediaCardTypes(registry, ui.createConfigMediaOptionsFeature({ disabledCardTypes: [] }),
    ui.createConfigModalTabOptionsFeature({ document: {}, renderButtonSettings() {} }), "test", {
    cardBadgeLabelHtml(helpers, label) { return '<span class="sp-btn-label">' + helpers.escHtml(label) + '</span>'; },
    cardLargeNumbersActiveForCardSize() { return false; }, cardSensorPreviewHtml() { return ""; },
  }, { infoPanel() {} }, { renderButtonSettings() {}, renderPreview() {} });
  function element() {
    const node = { className: "", children: [], style: { setProperty(name, value) { this[name] = value; } },
      setAttribute() {}, removeAttribute() {}, appendChild(child) { this.children.push(child); } };
    node.classList = { contains(name) { return node.className.split(" ").includes(name); } };
    return node;
  }
  const fixtures = [];
  for (const theme of ["Dark", "Light"]) for (const subpage of [false, true]) {
    for (const colour of ["00BCD4", "FFEC16", "FFFFFF", "#9d9d9d", "invalid", ""]) {
      for (const [mode, precision, extra] of [
        ["position", "", ""], ["now_playing", "progress", ""],
        ["now_playing", "play_pause", ""], ["cover_art", "", ""],
        ["cover_art", "", "cover_art_details"],
      ]) {
        ui.state.themeMode = theme;
        const base = /^#?[0-9a-f]{6}$/i.test(colour) ? colour.replace(/^#/, "").toUpperCase() : "";
        const neutral = ui.PREVIEW_THEME_COLORS[theme];
        const progressColours = { "00BCD4": "4DD0E1", "FFEC16": "B3A50F", "FFFFFF": "B3B3B3", "9D9D9D": "BABABA" };
        const main = element();
        const button = { type: "media", sensor: mode, precision, label: "Living room", options: [extra, colour ? "card_off_color=" + colour : ""].filter(Boolean).join(",") };
        ui.createPreviewRenderFeature({
          document: { createElement: element }, updateClockBarItemUi() {},
          layout: { config: { disabledCardTypes: [], infoOnly: false } }, cards: registry,
          confirmationOptions: { cardOnPattern() { return "solid"; } }, codec: { getSubpage() { return {}; } },
          runtime: { els: { previewMain: main } }, screenRotation: { gridPreviewBlocked() { return false; } },
          shell: { isConfigLocked() { return false; } },
          grid: { ctx() { return { grid: [1], buttons: [button], maxSlots: 1, sizes: {}, selected: [], isSub: subpage }; }, resolveIcon() { return "music"; }, sizeClass() { return ""; } },
          selection: { renderSelectionBar() {}, updatePreviewHint() {} },
        }).render();
        const card = main.children[0];
        fixtures.push({
          name: `${theme}/${subpage ? "secondary" : "main"}/${mode}/${precision}/${extra}/${colour}`,
          html: card.innerHTML, className: card.className,
          style: Object.fromEntries(Object.entries(card.style).filter(([, value]) => typeof value === "string")),
          expectedBackground: "#" + (base || (mode === "cover_art" ? neutral.surfaceCard : neutral.surfacePrimary)),
          expectedFill: mode === "position" || precision === "progress" ? "#" + (base ? progressColours[base] : neutral.trackBackground) : null,
          artworkDetails: extra === "cover_art_details",
          expectedForeground: base ? ui.cardPreviewTextColor(base) : "#" + neutral.textPrimary,
          placeholder: mode === "cover_art" && !extra,
        });
      }
    }
  }
  return { fixtures, css: ui.createWebStyles(false) };
}

module.exports = { mediaPreviewFixtures };

test("media overlays and cover placeholders reflect the validated card colour on both pages and themes", () => {
  for (const fixture of mediaPreviewFixtures().fixtures) {
    const overlay = fixture.html.match(/class="sp-(?:slider|image)-preview[^\"]*" style="([^\"]*)"/);
    assert(overlay, fixture.name);
    assert(overlay[1].includes(fixture.expectedBackground), fixture.name + " overlay colour");
    if (fixture.expectedFill) {
      const fill = fixture.html.match(/class="sp-slider-fill" style="([^\"]*)"/);
      assert(fill && fill[1].includes(fixture.expectedFill), fixture.name + " progress fill colour");
    }
    if (fixture.placeholder) {
      assert(fixture.html.includes('class="sp-image-label-text sp-image-label-main" style="color:' + fixture.expectedForeground + '"'), fixture.name + " placeholder contrast");
    }
  }
});
