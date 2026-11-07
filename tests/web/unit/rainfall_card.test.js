"use strict";
const assert=require("node:assert/strict");
const {test}=require("node:test");
const path=require("node:path");
const {loadTypeScriptModule}=require("../../../scripts/load_typescript_module");
const root=path.resolve(__dirname,"../../..");
const {createConfigWeatherOptionsFeature}=loadTypeScriptModule(path.join(root,"src/webserver/application/config_weather_options.ts"));
const {registerWeatherCardTypes}=loadTypeScriptModule(path.join(root,"src/webserver/cards/weather.ts"));
const {escHtml}=loadTypeScriptModule(path.join(root,"src/webserver/application/ui_primitives.ts"));
test("rainfall supports sensors and daily forecast data without making sensor cards forecast cards",()=>{
 for(const disabledCardTypes of [[],["weather_forecast"]]){
  const options=createConfigWeatherOptionsFeature({disabledCardTypes});
  assert.equal(options.normalizeWeatherCardMode("rainfall"),"rainfall");
  assert(options.weatherModeOptions().some(([value])=>value==="rainfall"));
  const sensor={precision:"rainfall",options:""};
  const forecast={precision:"rainfall",options:"rainfall_source=forecast"};
  assert.equal(options.rainfallUsesForecast(sensor),false);
  assert.equal(options.rainfallUsesForecast(forecast),!disabledCardTypes.length);
  assert.equal(options.normalizeWeatherOptions(forecast.options,forecast),disabledCardTypes.length?"":"rainfall_source=forecast");
  assert.equal(options.normalizeWeatherOptions(forecast.options,{precision:"today"}),"");
  let card;
  registerWeatherCardTypes({register:(_,definition)=>card=definition},options,{},{},{});
  assert.equal(card.cardMetadata.mode.value(sensor),"rainfall");
  for(const button of [sensor,forecast]){
   const preview=card.renderPreview(button);
   assert.match(preview.iconHtml,/weather-rainy/);
   assert.match(preview.labelHtml,/Rainfall.*2.4 mm/);
   assert.equal(preview.labelEditable,false);
  }
 }
});


test("changing existing weather modes to Rainfall clears incompatible entities", () => {
 const options = createConfigWeatherOptionsFeature({});
 let card;
 registerWeatherCardTypes({register: (_, definition) => card = definition}, options, {}, {}, {renderButtonSettings: () => {}});
 for (const precision of ["", "today", "tomorrow", "daily", "wind"]) {
  const config = {precision, entity: "weather.home", sensor: "", options: ""};
  const saved = {};
  card.cardMetadata.mode.onChange.call({value: "rainfall"}, config, {saveField: (key, value) => saved[key] = value});
  assert.equal(config.entity, "");
  assert.equal(saved.entity, "");
  assert.equal(config.precision, "rainfall");
  assert.equal(options.rainfallUsesForecast(config), false);
 }
});

test("rainfall sensor settings offer an optional unit suffix while forecast retains provider units", () => {
 const options = createConfigWeatherOptionsFeature({});
 let card;
 registerWeatherCardTypes({register: (_, definition) => card = definition}, options, {}, {}, {});
 const group = {appendChild() {}};
 for (const forecast of [false, true]) {
  const button = {precision: "rainfall", options: forecast ? "rainfall_source=forecast" : "", unit: "in"};
  const textFields = [];
  card.renderSettings(group, button, 0, {
   idPrefix: "test-",
   renderCardModeSelector() {}, markCardPrimaryField() {}, renderCardEntityField() {},
   disclosureSection: () => ({panel: group, section: {className: "", appendChild() {}}}),
   segmentControl: () => ({segment: {}}), fieldWithControl() {},
   renderCardTextField: (_, config, helpers, metadata) => textFields.push({config, metadata}),
  });
  assert.equal(textFields.length, forecast ? 0 : 1);
  if (!forecast) {
   assert.equal(textFields[0].config, button);
   assert.equal(textFields[0].metadata.field, "unit");
   assert.match(textFields[0].metadata.placeholder, /mm or in/);
  }
  assert.match(card.renderPreview(button, {escHtml}).labelHtml, forecast ? /2.4 mm/ : /2.4 in/);
 }
});

test("rainfall preview escapes a custom unit suffix", () => {
 const options = createConfigWeatherOptionsFeature({});
 let card;
 registerWeatherCardTypes({register: (_, definition) => card = definition}, options, {}, {}, {});
 const preview = card.renderPreview({precision: "rainfall", options: "", unit: "<mm>"}, {escHtml});
 assert.match(preview.labelHtml, /2.4 &lt;mm&gt;/);
 assert.doesNotMatch(preview.labelHtml, /<mm>/);
});
