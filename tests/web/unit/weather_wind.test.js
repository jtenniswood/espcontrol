"use strict";
const assert = require("node:assert/strict");
const { test } = require("node:test");
const path = require("node:path");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const root = path.resolve(__dirname, "../../..");
const { createConfigWeatherOptionsFeature } = loadTypeScriptModule(path.join(root, "src/webserver/application/config_weather_options.ts"));
const { registerWeatherCardTypes } = loadTypeScriptModule(path.join(root, "src/webserver/cards/weather.ts"));
test("wind is selectable and retained with or without forecast support", () => {
  for (const disabledCardTypes of [[], ["weather_forecast"]]) {
    const options = createConfigWeatherOptionsFeature({ disabledCardTypes });
    assert.equal(options.normalizeWeatherCardMode("wind"), "wind");
    assert(options.weatherModeOptions().some(([value]) => value === "wind"));
    assert.equal(options.weatherCardIsForecastMode({ precision: "wind" }), false);
    let weather;
    registerWeatherCardTypes({ register: (_, definition) => weather = definition }, options, {}, {}, {});
    const config = { precision: "wind", entity: "weather.home" };
    weather.onSelect(config);
    assert.equal(config.precision, "wind");
    assert.equal(weather.cardMetadata.mode.value(config), "wind");
    const preview = weather.renderPreview(config);
    assert.match(preview.iconHtml, /arrow-top-right/);
    assert.match(preview.labelHtml, /North East.*12.5 km\/h/);
    assert.equal(preview.labelEditable, false);
  }
});

test("separate wind sensors preserve their source and render two required fields", () => {
    const options = createConfigWeatherOptionsFeature({});
    const config = { precision: "wind", options: "wind_source=sensors", entity: "sensor.direction", sensor: "sensor.speed" };
    assert.equal(options.weatherWindUsesSensors(config), true);
    assert.equal(options.normalizeWeatherOptions("ignored=1,wind_source=sensors", config), "wind_source=sensors");
    assert.equal(options.normalizeWeatherOptions("wind_source=sensors", { precision: "today" }), "");
    let weather;
    registerWeatherCardTypes({ register: (_, definition) => weather = definition }, options, {}, {}, {});
    const oldDocument = globalThis.document;
    globalThis.document = { createElement: () => ({ appendChild: () => {} }) };
    const fields = [];
    let sourceChange;
    const helpers = {
        renderCardModeSelector: () => {},
        markCardPrimaryField: () => {},
        disclosureSection: (label, _id, open) => {
            assert.equal(label, "Source");
            assert.equal(open, true);
            return { panel: {}, section: { appendChild: () => {} } };
        },
        fieldWithControl: () => ({}),
        segmentControl: (choices, value, onChange) => {
            assert.equal(value, "sensors");
            assert.deepEqual(choices.map(([key]) => key), ["weather", "sensors"]);
            sourceChange = onChange;
            return { segment: {} };
        },
        renderCardEntityField: (_panel, _button, _helpers, metadata) => fields.push(metadata.entity),
        saveField: () => {},
    };
    weather.renderSettings({ appendChild: () => {} }, config, 1, helpers);
    assert.deepEqual(fields.map(field => field.bindName), ["entity", "sensor"]);
    assert(fields.every(field => field.requiredMessage && field.domains.includes("sensor")));
    // Re-register with a real rerender callback for changing sources.
    registerWeatherCardTypes({ register: (_, definition) => weather = definition }, options, {}, {}, { renderButtonSettings: () => {} });
    weather.renderSettings({ appendChild: () => {} }, config, 1, helpers);
    sourceChange("weather");
    assert.equal(config.entity, "");
    assert.equal(config.sensor, "");
    assert.equal(options.weatherWindUsesSensors(config), false);
    globalThis.document = oldDocument;
});
