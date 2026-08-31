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
    ";Connect;Wifi;Auto;;;wifi_qr;;ssid64=VGVzdA",
    "~,Connect,Wifi,Auto,,,wifi_qr_card,,ssid64%3DVGVzdA",
    "1|:Connect:Wifi:Auto:::wifi_qr::ssid64=VGVzdA",
    "~1|wifi_qr_card,,Connect,Wifi,Auto,,,,ssid64%3DVGVzdA",
  ]) {
    assert.equal(serializedConfigContainsWifiSharing(value), true, value);
  }
  assert.equal(serializedConfigContainsWifiSharing("sensor.wifi_qr_strength"), false);
  for (const value of [
    "sensor.example;wifi_qr;Wifi;Auto;;;action;;",
    "~sensor.example,wifi_qr,Wifi,Auto,,,action,,",
    "1|sensor.example:wifi_qr:Wifi:Auto:::action::",
    "~1|A,sensor.example,wifi_qr,Wifi,Auto,,,,",
  ]) {
    assert.equal(serializedConfigContainsWifiSharing(value), false, value);
  }
});

test("finds Wifi Sharing cards in backup panel documents", () => {
  const document = {
    buttons: { 1: "sensor.safe;Safe;Auto;Auto;;;action;;" },
    subpages: { 1: "~1|wifi_qr,,Connect,Wifi,Auto,,,,ssid64%3DVGVzdA" },
    settings: {},
  };
  assert.equal(panelConfigDocumentContainsWifiSharing(document), true);
  document.subpages[1] = "~1|A,,Safe,Auto,Auto,,,,";
  assert.equal(panelConfigDocumentContainsWifiSharing(document), false);
  document.buttons[1] = "sensor.safe;wifi_qr;Auto;Auto;;;action;;mode=one|wifi_qr";
  assert.equal(panelConfigDocumentContainsWifiSharing(document), false);
});
