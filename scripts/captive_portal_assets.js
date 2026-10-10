"use strict";

const fs = require("fs");
const path = require("path");
const esbuild = require("esbuild");
const postcss = require("postcss");
const zlib = require("zlib");
const ROOT = path.resolve(__dirname, "..");

function sharedStyles() {
  const bundled = esbuild.buildSync({
    entryPoints: [path.join(ROOT, "src/webserver/application/styles.ts")],
    bundle: true, write: false, platform: "node", format: "cjs",
  }).outputFiles[0].text;
  const exported = { exports: {} };
  new Function("module", "exports", bundled)(exported, exported.exports);
  // Reuse the actual settings controls without embedding the display preview
  // and editor CSS into every firmware's offline provisioning page.
  const selectors = new Set([
    ":root", "body", "#sp-app", ".sp-header", ".sp-brand", ".sp-brand-title",
    ".sp-brand-name", ".card", ".card:hover", ".card h3", ".sp-field",
    ".sp-field:last-child", ".sp-field-label", ".sp-input", ".sp-input:focus",
    ".sp-action-btn", ".sp-action-btn:focus-visible", ".sp-action-btn:active",
    ".sp-save-btn", ".sp-save-btn:hover",
  ]);
  const css = postcss.parse(exported.exports.createWebStyles(false));
  css.walkRules(rule => {
    const keep = rule.selectors.filter(selector => selectors.has(selector));
    if (keep.length) rule.selectors = keep;
    else rule.remove();
  });
  css.walkAtRules(rule => {
    if (rule.name !== "media" || !rule.nodes.length) rule.remove();
  });
  return css.toString();
}

function pageSource(filename, styles = sharedStyles()) {
  const template = fs.readFileSync(path.join(ROOT, "components/captive_portal", filename), "utf8");
  const marker = "/* __ESPCONTROL_WEB_STYLE__ */";
  if (!template.includes(marker)) throw new Error(`${filename}: missing stylesheet marker`);
  return template.replace(marker, styles);
}

function pageHeader(filename, symbol, styles) {
  const source = Buffer.from(pageSource(filename, styles));
  const encodings = [zlib.gzipSync(source, { level: 9, mtime: 0 }), zlib.brotliCompressSync(source)];
  encodings[0][9] = 255; // Keep the gzip OS byte stable across build hosts.
  const array = bytes => Array.from({ length: Math.ceil(bytes.length / 16) }, (_, i) =>
    "    " + [...bytes.subarray(i * 16, (i + 1) * 16)].map(b => `0x${b.toString(16).padStart(2, "0")}`).join(", ") + ","
  ).join("\n");
  return `// Generated from ${filename} and EspControl web styles; run python3 scripts/build.py portal.\n` +
    "// Upstream: ESPHome 2026.9.1; see README.md and PORTAL_LICENSE.\n" +
    '#pragma once\n#include "esphome/core/hal.h"\nnamespace esphome::captive_portal {\n#ifdef USE_CAPTIVE_PORTAL_GZIP\n' +
    `constexpr uint8_t ${symbol}[] PROGMEM = {\n${array(encodings[0])}\n};\n#else\n` +
    `constexpr uint8_t ${symbol}[] PROGMEM = {\n${array(encodings[1])}\n};\n#endif\n}\n`;
}

module.exports = { sharedStyles, pageSource };
if (require.main === module) {
  const styles = sharedStyles();
  process.stdout.write(JSON.stringify({
    "components/captive_portal/captive_index.h": pageHeader("portal.html", "INDEX_GZ", styles),
    "components/captive_portal/wifi_saved.h": pageHeader("wifi_saved.html", "WIFI_SAVED_GZ", styles),
  }));
}
