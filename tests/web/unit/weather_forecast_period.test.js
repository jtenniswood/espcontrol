"use strict";
const assert = require("node:assert/strict");
const { test } = require("node:test");
const path = require("node:path");
const { loadTypeScriptModule } = require("../../../scripts/load_typescript_module");
const root = path.resolve(__dirname, "../../..");
const { createConfigWeatherOptionsFeature } = loadTypeScriptModule(path.join(root, "src/webserver/application/config_weather_options.ts"));
const { registerWeatherCardTypes } = loadTypeScriptModule(path.join(root, "src/webserver/cards/weather.ts"));
function setup(disabledCardTypes = []) {
    const options = createConfigWeatherOptionsFeature({ disabledCardTypes });
    let card;
    registerWeatherCardTypes({ register: (_, definition) => card = definition }, options, {}, {}, { renderButtonSettings() {} });
    const saved = {};
    return { options, card, saved, helpers: { saveField: (name, value) => saved[name] = value } };
}
test("forecast grouping keeps legacy stored periods and hides Forecast where unsupported", () => {
    const { card, options, helpers } = setup();
    for (const precision of ["today", "tomorrow", "daily"]) {
        const config = { precision, entity: "weather.home", options: "" };
        assert.equal(card.cardMetadata.mode.value(config), "forecast");
        card.cardMetadata.mode.onChange.call({ value: "forecast" }, config, helpers);
        assert.equal(config.precision, precision);
        assert.equal(config.entity, "weather.home");
    }
    assert.deepEqual(options.weatherModeOptions().map(([, label]) => label), ["Air Pressure", "Current Conditions", "Forecast", "Humidity", "Rainfall", "Temperature", "Visibility", "Wind"]);
    const disabled = setup(["weather_forecast"]).options;
    assert.equal(disabled.weatherModeOptions().some(([value]) => value === "forecast"), false);
    assert.equal(disabled.normalizeWeatherCardMode("daily"), "");
});
test("selecting Forecast from a sensor clears incompatible source settings and stores Today", () => {
    for (const precision of ["rainfall", "humidity", "pressure", "wind", "temperature"]) {
        const { card, helpers, saved } = setup();
        const config = { precision, entity: "sensor.reading", sensor: precision === "wind" ? "sensor.speed" : "", options: precision === "wind" ? "wind_source=sensors" : "" };
        card.cardMetadata.mode.onChange.call({ value: "forecast" }, config, helpers);
        assert.equal(config.entity, "");
        assert.equal(config.sensor, "");
        assert.equal(config.options, "");
        assert.equal(config.precision, "today");
        assert.equal(saved.precision, "today");
    }
});
