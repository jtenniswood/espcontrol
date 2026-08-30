"use strict";

const { test } = require("node:test");
const assert = require("node:assert/strict");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");

const {
  panelConfigDocumentContainsWifiSharing,
  serializedConfigContainsWifiSharing,
} = loadTypescriptTest("src/webserver/features/wifi_sharing_config.ts");

test("recognizes Wifi Sharing in every supported serialized card format", () => {
  for (const value of [
    "wifi_qr,,Connect,Wifi",
    "prefix;wifi_qr_card;options",
    "button:wifi_qr:ssid64=VGVzdA",
    "buttons|wifi_qr_card,ssid64=VGVzdA",
  ]) {
    assert.equal(serializedConfigContainsWifiSharing(value), true, value);
  }
  assert.equal(serializedConfigContainsWifiSharing("sensor.wifi_qr_strength"), false);
  assert.equal(serializedConfigContainsWifiSharing("action,,Connect,Wifi"), false);
});

test("finds Wifi Sharing cards in backup panel documents", () => {
  const document = {
    buttons: { 1: "action,,Safe" },
    subpages: { 1: "buttons|wifi_qr,ssid64=VGVzdA,pass64=c2VjcmV0" },
    settings: {},
  };
  assert.equal(panelConfigDocumentContainsWifiSharing(document), true);
  document.subpages[1] = "buttons|action,Safe";
  assert.equal(panelConfigDocumentContainsWifiSharing(document), false);
});
