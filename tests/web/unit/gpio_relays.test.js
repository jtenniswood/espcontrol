"use strict";
const assert = require("node:assert/strict");
const { test } = require("node:test");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const gpio = loadTypescriptTest("src/webserver/application/gpio_relays.ts");
function settings() { return { enabled: false, allowed_pins: [1, 2, 3], controls: [0, 1, 2].map(() => ({pin: -1, name: "", inverted: false})) }; }
function response(body, status = 200) { return new Response(JSON.stringify(body), { status }); }
test("GPIO relay controls require opt-in, explicit supported pins, and unique assignments", async () => {
  const info = settings();
  await gpio.gpioRelayRequest(async () => response(info));
  assert.deepEqual(gpio.gpioRelayOptions(), []);
  info.enabled = true;
  assert.match(gpio.validateGpioRelaySettings(info), /Assign a GPIO pin/);
  info.controls[0].pin = 1; info.controls[0].name = "Porch";
  assert.equal(gpio.validateGpioRelaySettings(info), "");
  info.controls[1].pin = 1;
  assert.match(gpio.validateGpioRelaySettings(info), /only be assigned/);
  info.controls[1].pin = 23;
  assert.match(gpio.validateGpioRelaySettings(info), /supported GPIO/);
  info.controls[1].pin = -1;
  let sent;
  await gpio.gpioRelayRequest(async (_url, init) => { sent = JSON.parse(init.body); return response(info); }, info);
  assert.equal(sent.controls[0].pin, 1);
  assert.deepEqual(gpio.gpioRelayOptions(), [{key: "gpio_relay_1", label: "Porch"}]);
  await assert.rejects(gpio.gpioRelayRequest(async () => response({}, 500), {...info, enabled: false}));
  assert.equal(gpio.gpioRelayOptions().length, 1); // Failed saves do not change card targets.
  await gpio.gpioRelayRequest(async () => response({...info, enabled: false}));
  assert.deepEqual(gpio.gpioRelayOptions(), []);
  assert.equal(await gpio.gpioRelayRequest(async () => response({}, 404)), null);
});
test("GPIO cards remain distinct from built-in relay cards and keep invalid targets", () => {
  const options = loadTypescriptTest("src/webserver/application/config_internal_relay_options.ts")
    .createConfigInternalRelayOptionsFeature({}, () => [{key: "gpio_relay_2", label: "Fan"}]);
  let type, definition;
  loadTypescriptTest("src/webserver/cards/internal.ts").registerInternalCardTypes(
    { register(key, card) { type = key; definition = card; } }, options, {}, {}, "gpio_relay");
  assert.equal(type, "gpio_relay"); assert.equal(definition.label(), "GPIO Relay");
  const button = {}; definition.onSelect(button);
  assert.equal(button.type, "gpio_relay"); assert.equal(button.entity, "gpio_relay_2");
});
