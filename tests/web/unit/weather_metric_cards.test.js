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
    const fields = [], controls = [], saved = {};
    const helpers = {
        idPrefix: "test-", renderCardModeSelector() {}, markCardPrimaryField() {},
        disclosureSection: () => ({ panel: {}, section: { className: "", appendChild() {} } }),
        segmentControl: (choices, value, onChange) => {
            controls.push({ choices, value, onChange }); return { segment: {} };
        },
        fieldWithControl() {},
        renderCardEntityField: (_panel, _button, _helpers, metadata) => fields.push(metadata.entity),
        saveField: (name, value) => saved[name] = value,
    };
    return { options, card, helpers, fields, controls, saved };
}
test("weather readings are available without daily forecast support and normalize only their own source", () => {
    for (const disabled of [[], ["weather_forecast"]]) {
        const { options, card } = setup(disabled);
        for (const mode of ["pressure", "humidity", "visibility", "temperature"]) {
            assert.equal(options.normalizeWeatherCardMode(mode), mode);
            assert(options.weatherModeOptions().some(([value]) => value === mode));
            assert.equal(options.weatherCardIsForecastMode({ precision: mode }), false);
            assert.equal(options.normalizeWeatherOptions("rainfall_source=forecast,wind_source=sensors,visibility_source=sensor," + mode + "_source=forecast", { precision: mode }), mode === "visibility" ? "" : mode + "_source=forecast");
            const preview = card.renderPreview({ precision: mode });
            assert.equal(preview.labelEditable, false);
            assert.match(preview.iconHtml, new RegExp(mode === "temperature" ? "thermometer" : mode === "pressure" ? "gauge" : mode === "humidity" ? "water-percent" : "weather-hazy"));
            assert.match(preview.labelHtml, new RegExp(mode === "temperature" ? "Currently.*18.5 °C" : mode === "pressure" ? "Air Pressure.*1013 hPa" : mode === "humidity" ? "Humidity.*65 %" : "Visibility.*10 km"));
        }
    }
});
test("temperature, pressure and humidity switch sources, clear incompatible entities and preserve the active source", () => {
    for (const mode of ["pressure", "humidity", "temperature"]) {
        const { card, helpers, controls, fields, options } = setup();
        const config = { precision: mode, entity: "sensor.reading", options: "" };
        const panel = { appendChild() {} };
        card.renderSettings(panel, config, 1, helpers);
        assert.deepEqual(fields[0].domains, ["sensor", "input_number"]);
        assert.deepEqual(controls[0].choices, [["forecast", "Forecast"], ["sensor", "Sensor"]]);
        controls[0].onChange("sensor"); assert.equal(config.entity, "sensor.reading");
        controls[0].onChange("forecast"); assert.equal(config.entity, "");
        assert.equal(config.options, mode + "_source=forecast");
        config.entity = "weather.home";
        card.renderSettings(panel, config, 1, helpers);
        assert.deepEqual(fields[1].domains, ["weather"]);
        assert.equal(controls[1].value, "forecast");
        controls[1].onChange("forecast"); assert.equal(config.entity, "weather.home");
        controls[1].onChange("sensor"); assert.equal(config.entity, "");
        assert.equal(options.weatherMetricUsesForecast(config), false);
    }
});
test("visibility has only a weather entity field and never renders source tabs", () => {
    const { card, helpers, fields, controls } = setup();
    card.renderSettings({ appendChild() {} }, { precision: "visibility", options: "" }, 1, helpers);
    assert.equal(controls.length, 0);
    assert.equal(fields.length, 1);
    assert.equal(fields[0].label, "Weather Entity");
    assert.deepEqual(fields[0].domains, ["weather"]);
});
test("changing from a sensor reading to visibility clears the sensor entity and source option", () => {
    for (const mode of ["pressure", "humidity", "rainfall", "temperature"]) {
        const { card, helpers } = setup();
        const config = { precision: mode, options: "", entity: "sensor.reading" };
        card.cardMetadata.mode.onChange.call({ value: "visibility" }, config, helpers);
        assert.equal(config.entity, "");
        assert.equal(config.options, "");
        assert.equal(config.precision, "visibility");
    }
});
