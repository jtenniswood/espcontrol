"use strict";

const assert = require("node:assert/strict");
const { test } = require("node:test");
const path = require("node:path");
const vm = require("node:vm");
const esbuild = require("esbuild");

test("layout previews use conditional sensor fallbacks ahead of retained single colours", () => {
  const root = path.resolve(__dirname, "../../..");
  const bundled = esbuild.buildSync({
    stdin: { contents: `
      export {createPreviewRenderFeature} from "./src/webserver/application/preview_render";
      export {initializeAppState,state} from "./src/webserver/state/app_instance";
      export {initializeDeviceConfig} from "./src/webserver/device_config";
      export {PREVIEW_THEME_COLORS} from "./src/webserver/state/preview_theme";
      export {setConfigOptionValue} from "./src/webserver/model/config_primitives";
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
  function element() {
    const node = { className: "", children: [], style: { setProperty(name, value) { this[name] = value; } },
      setAttribute() {}, removeAttribute() {}, appendChild(child) { this.children.push(child); } };
    node.classList = { contains(name) { return node.className.split(" ").includes(name); } };
    return node;
  }
  for (const theme of ["Dark", "Light"]) for (const isSub of [false, true]) {
    for (const precision of ["1", "time", "text", "icon"]) for (const draft of [false, true]) {
      for (const [type, payload, expectedColour] of [
        ["sensor", "v2||t,running,FF0000|", ""],
        ["sensor", "v2|sensor.power|t,running,FF0000|FFFFFF", "FFFFFF"],
        ["sensor", "v2||t,running,FF0000|000000", "000000"],
        ["sensor", "v1||t,running,FF0000", ""],
        ["sensor", "v3||t,running,FF0000|FFFFFF", "6633B9"],
        ["sensor", "v2||t,running,FF0000|invalid", "6633B9"],
        ["sensor", "", "6633B9"],
        ["local_sensor", "v2||t,running,FF0000|FFFFFF", "6633B9"],
      ]) {
        ui.state.themeMode = theme;
        const button = { type, precision, options: ui.setConfigOptionValue("card_off_color=6633B9", "sensor_colours", payload) };
        ui.state.settingsDraft = draft ? { slot: 1, isSub, homeSlot: ui.state.editingSubpage, button } : null;
        const main = element();
        let renderedColours;
        const definition = { allowInSubpage: true, renderPreview(b, helpers) { renderedColours = helpers; return {}; } };
        ui.createPreviewRenderFeature({
          document: { createElement: element }, updateClockBarItemUi() {},
          layout: { config: { disabledCardTypes: [], infoOnly: false } }, cards: { definitions: { sensor: definition, local_sensor: definition } },
          confirmationOptions: { cardOnPattern() { return "solid"; } }, codec: { getSubpage() { return {}; } },
          runtime: { els: { previewMain: main } }, screenRotation: { gridPreviewBlocked() { return false; } },
          shell: { isConfigLocked() { return false; } },
          grid: { ctx() { return { grid: [1], buttons: [draft ? { type: "sensor" } : button], maxSlots: 1, sizes: {}, selected: [], isSub }; }, resolveIcon() { return "gauge"; }, sizeClass() { return ""; } },
          selection: { renderSelectionBar() {}, updatePreviewHint() {} },
        }).render();
        const label = `${theme}/${isSub}/${precision}/${draft}/${type}/${payload}`;
        const foreground = expectedColour === "FFFFFF" ? "#212121" : expectedColour ? "#FFFFFF" : "#" + ui.PREVIEW_THEME_COLORS[theme].textPrimary;
        assert.equal(main.children[0].style.backgroundColor, "#" + (expectedColour || ui.PREVIEW_THEME_COLORS[theme].surfaceSensor), label);
        assert.equal(main.children[0].style["--card-text-color"], foreground, label + " foreground");
        assert.equal(renderedColours.cardBackgroundColor, expectedColour, label + " renderer background");
        assert.equal(renderedColours.cardTextColor, foreground, label + " renderer foreground");
      }
    }
  }
});
