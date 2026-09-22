#!/usr/bin/env node
"use strict";

// Documentation renders: production preview cards with native firmware metrics.
// This is a browser approximation, not an LVGL framebuffer capture.
const fs = require("node:fs");
const path = require("node:path");
const assert = require("node:assert/strict");
const { chromium } = require("playwright");
const { freshWebOutputDir } = require("./web_source");
const { loadTypeScriptModule } = require("./load_typescript_module");
const ROOT = path.resolve(__dirname, "..");
const DEST = path.join(ROOT, "docs/public/images/examples");
const manifest = JSON.parse(fs.readFileSync(path.join(ROOT, "devices/manifest.json"), "utf8"));
const { WEB_UI_COLORS: colors } = loadTypeScriptModule(path.join(ROOT, "src/webserver/state/ui_tokens.ts"));
const examples = JSON.parse(fs.readFileSync(path.join(__dirname, "fixtures/docs-interface-examples.json"), "utf8"));

function firmwareMetrics(slug) {
  const device = manifest.devices[slug];
  const substitutions = device.firmware.package.substitutions;
  const fonts = fs.readFileSync(path.join(ROOT, "devices", slug, "device/fonts.yaml"), "utf8");
  const lvgl = fs.readFileSync(path.join(ROOT, "devices", slug, "device/lvgl.yaml"), "utf8");
  const page = lvgl.slice(lvgl.indexOf("- id: main_page"));
  function number(value) {
    const n = Number(String(value).replace(/["']|px/g, ""));
    assert(Number.isFinite(n), `Invalid firmware metric: ${value}`);
    return n;
  }
  function match(text, regex) {
    const value = text.match(regex);
    assert(value, `Missing firmware metric: ${regex}`);
    return number(value[1]);
  }
  function font(role) {
    return match(fonts, new RegExp(`id: ${role}\\s+size: (\\d+)`));
  }
  const [width, height] = device.public.resolution.split(" x ").map(Number);
  return {
    width, height, cols: device.layout.cols, rows: device.layout.rows,
    padding: number(substitutions.padding), radius: number(substitutions.radius),
    gap: number(substitutions.main_page_card_gap), top: number(substitutions.main_page_pad_top),
    left: match(page, /pad_left: (\d+)/), right: match(page, /pad_right: (\d+)/),
    bottom: match(page, /pad_bottom: (\d+)/),
    icon: font(device.firmware.fonts.icon), label: font(substitutions.label_font),
    sensor: font(device.firmware.fonts.sensor), largeSensor: font(device.firmware.fonts.largeSensor),
    mediaTitle: font(device.firmware.fonts.mediaTitle),
    clockY: match(lvgl, /id: display_time[\s\S]*?\n\s+y: (\d+)/),
  };
}

function captureStyles(m) {
  return `
    .sp-screen { width:${m.width}px!important; height:${m.height}px!important; max-width:none!important;
      border:0!important; border-radius:0!important; box-shadow:none!important;
      --btn-pad:${m.padding}px; --btn-r:${m.radius}px; --btn-border:0px;
      --btn-icon:${m.icon}px; --btn-label:${m.label}px; --media-title:${m.mediaTitle}px;
      --btn-label-max-height:${m.label * 2.4}px; --btn-label-max-height-dbl:${m.label * 3.6}px;
      --grid-top:${m.top}px; --grid-left:${m.left}px; --grid-right:${m.right}px;
      --grid-bottom:${m.bottom}px; --grid-gap:${m.gap}px;
    }
    .sp-sensor-value { font-size:${m.sensor}px }
    .sp-sensor-preview-large .sp-sensor-value { font-size:${m.largeSensor}px!important }
    .sp-sensor-badge, .sp-subpage-badge, .sp-badge, .sp-support-btn { display:none!important }
    .sp-btn { box-shadow:none!important; transition:none!important }
    .sp-btn:hover { filter:none!important }
    .sp-clockbar-item { border:0!important; background:none!important }
    .sp-clockbar-hidden, .sp-temp { opacity:1!important; background:none!important; border:0!important }
    .sp-clockbar-left, .sp-clockbar-middle, .sp-clockbar-right { top:${m.clockY}px; height:${m.label * 1.2}px }
    .sp-topbar { align-items:flex-start; padding-top:0 }
  `;
}

async function render(browser, bundleDir, example) {
  const m = firmwareMetrics(example.device);
  const page = await browser.newPage({ viewport: { width: 1400, height: 1100 }, deviceScaleFactor: 1 });
  const errors = [];
  page.on("pageerror", error => errors.push(error.message));
  await page.route("**/*", async route => {
    const url = new URL(route.request().url());
    if (url.hostname !== "espcontrol.test") return route.abort();
    if (url.pathname === "/example") return route.fulfill({ contentType: "text/html", body:
      `<html><body><esp-app></esp-app><script src="/webserver/www.js?device=${example.device}"></script></body></html>` });
    if (url.pathname.startsWith("/webserver/")) {
      const file = path.resolve(bundleDir, url.pathname.slice("/webserver/".length));
      assert(file.startsWith(bundleDir + path.sep));
      if (fs.existsSync(file)) return route.fulfill({ path: file,
        contentType: file.endsWith(".js") ? "application/javascript" : "application/json" });
    }
    if (url.pathname === "/api/v1/capabilities") return route.fulfill({ json:
      { api: { version: 1 }, configuration: { read: false, write: false, document_versions: [] } } });
    return route.fulfill({ status: 404, body: "" });
  });
  await page.addInitScript(() => {
    window.__ESPCONTROL_TEST_HOOKS__ = {};
    window.EventSource = class {
      constructor() {
        this.listeners = {}; this.readyState = 1; window.exampleSource = this;
        setTimeout(() => this.emit("open", {}), 0);
      }
      addEventListener(type, fn) { (this.listeners[type] ||= []).push(fn); }
      close() {}
      emit(type, event) { (this.listeners[type] || []).forEach(fn => fn(event)); }
    };
  });
  await page.goto("http://espcontrol.test/example?events=1");
  await page.waitForFunction(() => window.exampleSource && window.__ESPCONTROL_TEST_HOOKS__.config?.serializeButtonConfig);
  await page.evaluate(example => {
    const emit = (id, state) => window.exampleSource.emit("state", { data: JSON.stringify({ id, state, value: state === "ON" }) });
    emit("select-screen__rotation", "0");
    example.cards.forEach((card, index) => emit(`text-button_${index + 1}_config`,
      window.__ESPCONTROL_TEST_HOOKS__.config.serializeButtonConfig({
        icon: "Auto", icon_on: "Auto", entity: "", label: "", sensor: "", unit: "", type: "", precision: "", options: "", ...card,
      })));
    emit("text-button_order", example.order);
    emit("switch-screen__clock_bar", "ON");
    emit("switch-screen__clock_bar_time", "ON");
    emit("switch-screen__network_status_icon", "ON");
    emit("switch-screen__subpage_chevron", "OFF");
  }, example);
  await page.waitForFunction(count => document.querySelectorAll(".sp-main > .sp-btn").length === count, example.cards.length);
  await page.addStyleTag({ content: captureStyles(m) });
  // The preview uses sample values, not live HA state. Supply only text and active
  // state here; retain its actual card DOM, icon glyphs, font faces and typography.
  await page.evaluate(({ example, colors }) => {
    for (const sample of example.samples || []) {
      const card = document.querySelector(`.sp-main > [data-slot="${sample.slot}"]`);
      if (sample.active) card.style.backgroundColor = "#" + colors.primary;
      for (const [selector, text] of Object.entries(sample.text || {})) {
        const element = card.querySelector(selector);
        if (!element) throw new Error(`Missing sample field ${sample.slot}: ${selector}`);
        element.textContent = text;
      }
    }
    document.querySelector(".sp-clock").textContent = example.time;
    document.querySelector(".sp-temp").textContent = example.temperature;
  }, { example, colors });
  await page.evaluate(() => document.fonts.ready);
  await page.mouse.move(1390, 1090);
  const metrics = await page.locator(".sp-screen").evaluate(screen => {
    const grid = screen.querySelector(".sp-main");
    const card = grid.querySelector(".sp-btn");
    const style = getComputedStyle(card);
    return { width: screen.clientWidth, height: screen.clientHeight,
      padding: parseFloat(style.paddingLeft), radius: parseFloat(style.borderRadius),
      gap: parseFloat(getComputedStyle(grid).columnGap), cols: getComputedStyle(grid).gridTemplateColumns.split(" ").length,
      label: parseFloat(getComputedStyle(card.querySelector(".sp-btn-label") || grid.querySelector(".sp-btn-label")).fontSize),
      fontsReady: document.fonts.check('22px Roboto') && document.fonts.check('55px "Material Design Icons"'),
      chevrons: [...screen.querySelectorAll(".sp-subpage-badge")].some(el => el.getBoundingClientRect().width > 0) };
  });
  for (const key of ["width", "height", "padding", "radius", "gap", "cols", "label"]) assert.equal(metrics[key], m[key], `${example.name}: ${key}`);
  assert(metrics.fontsReady, "Bundled fonts must be loaded");
  assert(!metrics.chevrons, "No subpage arrows in documentation examples");
  assert.deepEqual(errors, []);
  await page.locator(".sp-screen").screenshot({ path: path.join(DEST, `${example.name}-render.png`) });
  await page.close();
  console.log(`${example.name}: ${m.width}×${m.height}; ${m.padding}px padding; ${m.gap}px gaps; ${m.icon}px icons; ${m.label}px labels`);
}

(async () => {
  const bundleDir = freshWebOutputDir();
  const browser = await chromium.launch();
  try {
    for (const example of examples) await render(browser, bundleDir, example);
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
