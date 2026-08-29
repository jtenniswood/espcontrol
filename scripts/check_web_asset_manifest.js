#!/usr/bin/env node
"use strict";

const crypto = require("crypto");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const ROOT = path.resolve(__dirname, "..");
const WEB_ROOT = path.join(ROOT, "docs", "public", "webserver");
const DEVICE_MANIFEST_PATH = path.join(ROOT, "devices", "manifest.json");
const BUILD_SCRIPT_PATH = path.join(ROOT, "scripts", "build.py");

function sha256(content) {
  return crypto.createHash("sha256").update(content).digest("hex");
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

function expectedProfiles() {
  return Object.keys(readJson(DEVICE_MANIFEST_PATH).devices);
}

function expectedFirmwareVersions() {
  const source = fs.readFileSync(BUILD_SCRIPT_PATH, "utf8");
  const match = source.match(
    /^WEB_ASSET_SUPPORTED_FIRMWARE_VERSIONS = \(\n([\s\S]*?)^\)/m,
  );
  assert(match, "build source must declare web asset firmware versions");
  return [...match[1].matchAll(/"([^"]+)"/g)].map((item) => item[1]);
}

function verifyManifest(webRoot) {
  const manifestPath = path.join(webRoot, "web-assets.json");
  assert(fs.existsSync(manifestPath), "web asset manifest is missing");
  const manifest = readJson(manifestPath);
  assert(manifest.schemaVersion === 1, "web asset manifest schema version must be 1");
  assert(Array.isArray(manifest.bundles) && manifest.bundles.length === 1,
    "web asset manifest must declare one current bundle");

  const bundle = manifest.bundles[0];
  assert(typeof bundle.id === "string" && /^[a-f0-9]{64}$/.test(bundle.id),
    "web bundle id must be a SHA-256 digest");
  assert(bundle.sha256 === bundle.id, "web bundle digest must match its id");
  assert(bundle.path === `bundles/${bundle.id}/www.js`,
    "web bundle path must be content-addressed");
  assert(Array.isArray(bundle.deviceProfiles), "web bundle must declare device profiles");
  assert(JSON.stringify(bundle.deviceProfiles) === JSON.stringify(expectedProfiles()),
    "web bundle device profiles must match the device manifest");
  assert(JSON.stringify(bundle.firmwareVersions) === JSON.stringify(expectedFirmwareVersions()),
    "web bundle must declare the development and supported stable firmware versions");
  assert(bundle.webAssetVersion === 1, "web bundle must declare its web asset version");

  const bundlePath = path.join(webRoot, bundle.path);
  assert(fs.existsSync(bundlePath), "content-addressed web bundle is missing");
  const contents = fs.readFileSync(bundlePath);
  assert(sha256(contents) === bundle.sha256, "web bundle content does not match manifest digest");
  const embedded = fs.readFileSync(path.join(webRoot, "embedded", "www.js"), "utf8");
  assert(embedded.includes("__ESPCONTROL_START_EMBEDDED__"),
    "embedded editor must expose its offline fallback entry point");
  assert(embedded.includes("__ESPCONTROL_RELOAD_EMBEDDED__"),
    "embedded editor must expose a clean fallback reload entry point");
  assert(embedded.includes("__ESPCONTROL_UI_STARTING__"),
    "embedded editor must wait for deferred startup before using its fallback");
  assert(embedded.includes(contents.toString("utf8")),
    "embedded fallback must contain the immutable editor bundle");
  const bridge = fs.readFileSync(path.join(webRoot, "www.js"), "utf8");
  assert(bridge.includes("web-assets.json") && bridge.includes("firmwareVersions"),
    "hosted www.js must select an immutable bundle from the manifest");
  assert(bridge.includes("espcontrol_fallback"),
    "hosted www.js must honor a clean embedded fallback reload");
}

async function verifyBridge() {
  const manifest = readJson(path.join(WEB_ROOT, "web-assets.json"));
  const loaded = [];
  let cleanedFallbackPath = "";
  const sandbox = {
    URL,
    Promise,
    document: {
      currentScript: {
        getAttribute() {
          return "https://assets.example/webserver/www.js?device=esp32-p4-86";
        },
      },
      createElement() { return {}; },
      head: { appendChild(script) { loaded.push(script.src); } },
    },
    window: { location: { href: "http://panel.example/" } },
    history: { replaceState(_state, _title, path) { cleanedFallbackPath = path; } },
    fetch(url) {
      if (String(url).endsWith("web-assets.json")) {
        return Promise.resolve({ ok: true, json: () => Promise.resolve(manifest) });
      }
      if (String(url) === "/espcontrol/version.json") {
        return Promise.resolve({ ok: true, json: () => Promise.resolve({ version: "dev" }) });
      }
      return Promise.resolve({ ok: false, json: () => Promise.resolve(null) });
    },
  };
  vm.createContext(sandbox);
  vm.runInContext(fs.readFileSync(path.join(WEB_ROOT, "www.js"), "utf8"), sandbox);
  await new Promise((resolve) => setImmediate(resolve));
  assert(loaded.length === 1, "web bridge must load one matching immutable bundle");
  assert(loaded[0] === `https://assets.example/webserver/${manifest.bundles[0].path}?device=esp32-p4-86`,
    "web bridge must use the device firmware version when the URL omits it");

  const releaseLoaded = [];
  sandbox.document.currentScript.getAttribute = () =>
    "https://assets.example/webserver/www.js?device=esp32-p4-86&v=v2.7.1";
  sandbox.document.head.appendChild = (script) => releaseLoaded.push(script.src);
  vm.runInContext(fs.readFileSync(path.join(WEB_ROOT, "www.js"), "utf8"), sandbox);
  await new Promise((resolve) => setImmediate(resolve));
  assert(releaseLoaded.length === 1, "web bridge must load the supported stable firmware bundle");
  assert(releaseLoaded[0] === `https://assets.example/webserver/${manifest.bundles[0].path}?device=esp32-p4-86&v=v2.7.1`,
    "web bridge must select a bundle for an explicitly requested stable firmware version");

  let fallbackStarts = 0;
  sandbox.__ESPCONTROL_START_EMBEDDED__ = () => { fallbackStarts += 1; };
  sandbox.document.currentScript.getAttribute = () =>
    "https://assets.example/webserver/www.js?device=esp32-p4-86&v=v2.7.1";
  sandbox.document.head.appendChild = (script) => script.onerror();
  vm.runInContext(fs.readFileSync(path.join(WEB_ROOT, "www.js"), "utf8"), sandbox);
  await new Promise((resolve) => setImmediate(resolve));
  assert(fallbackStarts === 1,
    "web bridge must start the embedded editor when the immutable bundle fails to load");

  const cleanFallbackStarts = [];
  sandbox.window.location.href = "http://panel.example/?espcontrol_fallback=1";
  sandbox.document.head.appendChild = (script) => cleanFallbackStarts.push(script.src);
  vm.runInContext(fs.readFileSync(path.join(WEB_ROOT, "www.js"), "utf8"), sandbox);
  await new Promise((resolve) => setImmediate(resolve));
  assert(cleanFallbackStarts.length === 0,
    "web bridge must skip the hosted bundle after a clean embedded fallback reload");
  assert(cleanedFallbackPath === "/",
    "web bridge must remove the one-time clean fallback flag from the address");
  sandbox.window.location.href = "http://panel.example/";
}

async function main() {
  verifyManifest(WEB_ROOT);
  await verifyBridge();
  console.log("Web asset manifest checks passed.");
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
