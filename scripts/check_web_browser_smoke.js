#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { chromium } = require("playwright");

const ROOT = path.resolve(__dirname, "..");
const MANIFEST_PATH = path.join(ROOT, "devices", "manifest.json");
const WEB_OUTPUT_DIR = path.join(ROOT, "docs", "public", "webserver");
const FAILURE_DIR = path.join(ROOT, ".cache", "web-browser-smoke");

function readManifest() {
  return JSON.parse(fs.readFileSync(MANIFEST_PATH, "utf8"));
}

function parseAspect(aspect) {
  const parts = String(aspect || "").split("/");
  assert.strictEqual(parts.length, 2, `invalid web screen aspect: ${aspect}`);
  const width = Number(parts[0]);
  const height = Number(parts[1]);
  assert(width > 0 && height > 0, `invalid web screen aspect: ${aspect}`);
  return { width, height, ratio: width / height };
}

function orientationFor(ratio) {
  if (Math.abs(ratio - 1) < 0.05) return "square";
  return ratio > 1 ? "landscape" : "portrait";
}

function viewportFor(ratio) {
  const orientation = orientationFor(ratio);
  if (orientation === "square") return { width: 1000, height: 900 };
  if (orientation === "portrait") return { width: 1100, height: 1000 };
  return { width: 1280, height: 900 };
}

function casesFromManifest() {
  const manifest = readManifest();
  return Object.entries(manifest.devices || {}).map(([slug, device]) => {
    const aspect = parseAspect(device.web && device.web.screen && device.web.screen.aspect);
    const orientation = orientationFor(aspect.ratio);
    return {
      name: `${orientation}-${slug}`,
      slug,
      monochromeDisplay: !!(device.firmware && device.firmware.display && device.firmware.display.mode === "monochrome"),
      viewport: viewportFor(aspect.ratio),
      exerciseInteractions: slug === "guition-esp32-p4-jc8012p4a1",
    };
  });
}

const CASES = casesFromManifest();

const BUTTON_FIXTURES = [
  "light.kitchen;Kitchen;Lightbulb;Lightbulb",
  "sensor.energy;Energy;Gauge;Auto;sensor.energy;W;sensor;0",
  "climate.hall;Hall;Thermostat;Auto;;;climate;;",
  "media_player.living;Media;Auto;Auto;play_pause;;media;;",
];

function htmlFor(slug) {
  return [
    "<!doctype html>",
    '<html lang="en">',
    "<head>",
    '<meta charset="utf-8">',
    '<meta name="viewport" content="width=device-width,initial-scale=1">',
    `<title>${slug}</title>`,
    "</head>",
    "<body>",
    "<esp-app></esp-app>",
    `<script src="/webserver/${slug}/www.js"></script>`,
    "</body>",
    "</html>",
  ].join("");
}

function routeContentType(url) {
  if (/\.css(?:$|\?)/.test(url)) return "text/css";
  if (/\.(?:png|jpg|jpeg|gif|webp|svg)(?:$|\?)/.test(url)) return "image/svg+xml";
  return "text/plain";
}

async function installRoutes(context, slug) {
  const scriptPath = path.join(WEB_OUTPUT_DIR, slug, "www.js");
  assert(fs.existsSync(scriptPath), `${slug}: generated web UI does not exist at ${scriptPath}`);

  await context.route("**/*", async (route) => {
    const requestUrl = new URL(route.request().url());
    if (requestUrl.hostname === "espcontrol.test" && requestUrl.pathname === `/${slug}`) {
      await route.fulfill({ status: 200, contentType: "text/html", body: htmlFor(slug) });
      return;
    }
    if (requestUrl.hostname === "espcontrol.test" && requestUrl.pathname === `/webserver/${slug}/www.js`) {
      await route.fulfill({
        status: 200,
        contentType: "application/javascript",
        body: fs.readFileSync(scriptPath, "utf8"),
      });
      return;
    }
    if (requestUrl.hostname === "espcontrol.test") {
      await route.fulfill({ status: 204, contentType: "text/plain", body: "" });
      return;
    }
    await route.fulfill({ status: 200, contentType: routeContentType(requestUrl.pathname), body: "" });
  });
}

async function installFakeEventSource(page) {
  await page.addInitScript(() => {
    window.__eventSources = [];
    window.__seedEspState = function (events) {
      if (!window.__eventSources.length) throw new Error("No EventSource instance was created");
      var source = window.__eventSources[0];
      events.forEach(function (event) {
        source.dispatch("state", { data: JSON.stringify(event) });
      });
    };

    window.EventSource = class FakeEventSource {
      constructor(url) {
        this.url = url;
        this.readyState = 0;
        this.listeners = {};
        window.__eventSources.push(this);
        setTimeout(() => {
          this.readyState = 1;
          this.dispatch("open", {});
        }, 0);
      }

      addEventListener(type, listener) {
        if (!this.listeners[type]) this.listeners[type] = [];
        this.listeners[type].push(listener);
      }

      close() {
        this.readyState = 2;
      }

      dispatch(type, event) {
        (this.listeners[type] || []).forEach((listener) => listener(event));
      }
    };
  });
}

function seededEvents() {
  const events = [
    { id: "text-button_order", state: "1,2,3w,4,5" },
    { id: "text-button_on_color", state: "0073FF" },
    { id: "text-button_off_color", state: "CECECE" },
    { id: "text-sensor_card_color", state: "DEDEDE" },
    { id: "select-screen__theme", state: "Light", value: "Light", option: ["Light", "Dark"] },
    { id: "switch-screen__clock_bar", state: "ON", value: true },
    { id: "switch-screen__network_status_icon", state: "ON", value: true },
    { id: "select-screen__timezone", state: "Europe/London (GMT+0)", value: "Europe/London (GMT+0)", option: ["Europe/London (GMT+0)", "America/New_York (GMT-5)"] },
    { id: "select-screen__clock_format", state: "24h", value: "24h", option: ["12h", "24h"] },
    { id: "select-screen__rotation", state: "0", value: "0", option: ["0", "90", "180", "270"] },
    { id: "number-screensaver_timeout", state: "300", value: 300, min: 10, max: 3600 },
    { id: "switch-developer__experimental_features", state: "ON", value: true },
    { id: "text-subpage_6_config", state: "1,B|media_player.living:Living:Speaker:Auto:play_pause::media" },
  ];
  BUTTON_FIXTURES.forEach((state, index) => {
    events.push({ id: `text-button_${index + 1}_config`, state });
  });
  return events;
}

function assertNoLayoutBreaks(result, label) {
  assert(result.appVisible, `${label}: #sp-app should be visible`);
  assert(result.screenVisible, `${label}: .sp-screen should be visible`);
  assert(result.mainVisible, `${label}: .sp-main should be visible`);
  assert(result.applyVisible, `${label}: apply controls should be visible`);
  assert(result.gridChildren > 0, `${label}: grid should render cells`);
  assert(result.visibleGridChildren > 0, `${label}: grid cells should have visible size`);
  assert(result.visibleCards >= BUTTON_FIXTURES.length, `${label}: seeded cards should render`);
  assert.strictEqual(result.outsideGrid.length, 0, `${label}: grid children overflowed the preview: ${result.outsideGrid.join(", ")}`);
  assert.strictEqual(result.overlaps.length, 0, `${label}: grid children overlapped: ${result.overlaps.join(", ")}`);
  assert(
    result.documentScrollWidth <= result.windowWidth + 1,
    `${label}: page has horizontal overflow (${result.documentScrollWidth}px > ${result.windowWidth}px)`
  );
}

async function measureCoreLayout(page) {
  return page.evaluate(() => {
    function rectFor(el) {
      if (!el) return null;
      var r = el.getBoundingClientRect();
      return {
        left: r.left,
        top: r.top,
        right: r.right,
        bottom: r.bottom,
        width: r.width,
        height: r.height,
      };
    }
    function visible(rect) {
      return !!rect && rect.width > 1 && rect.height > 1;
    }
    function overlap(a, b) {
      var width = Math.min(a.right, b.right) - Math.max(a.left, b.left);
      var height = Math.min(a.bottom, b.bottom) - Math.max(a.top, b.top);
      return width > 1 && height > 1;
    }

    var app = document.querySelector("#sp-app");
    var screen = document.querySelector(".sp-screen");
    var main = document.querySelector(".sp-main");
    var apply = document.querySelector(".sp-apply-btn");
    var mainRect = rectFor(main);
    var children = Array.from(document.querySelectorAll(".sp-main > *")).map(function (el, index) {
      return { index: index, className: el.className, rect: rectFor(el) };
    });
    var visibleChildren = children.filter(function (child) { return visible(child.rect); });
    var outsideGrid = visibleChildren.filter(function (child) {
      var r = child.rect;
      return r.left < mainRect.left - 1 ||
        r.top < mainRect.top - 1 ||
        r.right > mainRect.right + 1 ||
        r.bottom > mainRect.bottom + 1;
    }).map(function (child) { return String(child.index); });
    var overlaps = [];
    for (var i = 0; i < visibleChildren.length; i++) {
      for (var j = i + 1; j < visibleChildren.length; j++) {
        if (overlap(visibleChildren[i].rect, visibleChildren[j].rect)) {
          overlaps.push(visibleChildren[i].index + "/" + visibleChildren[j].index);
        }
      }
    }

    return {
      appVisible: visible(rectFor(app)),
      screenVisible: visible(rectFor(screen)),
      mainVisible: visible(mainRect),
      applyVisible: visible(rectFor(apply)),
      gridChildren: children.length,
      visibleGridChildren: visibleChildren.length,
      visibleCards: document.querySelectorAll(".sp-main > .sp-btn").length,
      outsideGrid: outsideGrid,
      overlaps: overlaps,
      documentScrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
}

async function assertSettingsPage(page, label, options = {}) {
  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const settingsVisible = await page.locator("#sp-settings").isVisible();
  const appearanceVisible = await page.locator("text=Appearance").first().isVisible();
  if (!(await page.locator("#sp-set-on-color").isVisible())) {
    await page.getByText("Appearance", { exact: true }).click();
  }
  const themeVisible = await page.locator("#sp-set-theme").isVisible();
  const onColorVisible = await page.locator("#sp-set-on-color").isVisible();
  assert(settingsVisible, `${label}: settings page should be visible`);
  assert(appearanceVisible, `${label}: settings content should render`);
  assert(themeVisible, `${label}: theme selector should render`);
  assert(onColorVisible, `${label}: appearance controls should render`);
  const overflow = await page.evaluate(() => document.documentElement.scrollWidth > window.innerWidth + 1);
  assert(!overflow, `${label}: settings page has horizontal overflow`);
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
}

async function assertEpaperThemePreview(page, label) {
  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  if (!(await page.locator("#sp-set-theme").isVisible())) {
    await page.getByText("Appearance", { exact: true }).click();
  }
  await page.locator("#sp-set-theme").selectOption("Dark");
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  const dark = await page.evaluate(() => {
    const app = document.querySelector("#sp-app");
    const screen = document.querySelector(".sp-screen");
    const card = document.querySelector(".sp-main > .sp-btn");
    return {
      theme: app && app.getAttribute("data-screen-theme"),
      screenBg: screen && getComputedStyle(screen).backgroundColor,
      cardBg: card && getComputedStyle(card).backgroundColor,
      cardBorder: card && getComputedStyle(card).borderTopColor,
      cardText: card && getComputedStyle(card.querySelector(".sp-btn-label")).color,
    };
  });
  assert.strictEqual(dark.theme, "dark", `${label}: dark theme should be reflected on the preview root`);
  assert.strictEqual(dark.screenBg, "rgb(0, 0, 0)", `${label}: dark preview should use the normal dark screen`);
  assert.strictEqual(dark.cardBg, "rgb(49, 49, 49)", `${label}: dark preview cards should use the normal dark card colour`);
  assert.strictEqual(dark.cardText, "rgb(255, 255, 255)", `${label}: dark preview card text should use the normal dark text colour`);

  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  if (!(await page.locator("#sp-set-theme").isVisible())) {
    await page.getByText("Appearance", { exact: true }).click();
  }
  await page.locator("#sp-set-theme").selectOption("Light");
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  const light = await page.evaluate(() => {
    const app = document.querySelector("#sp-app");
    const screen = document.querySelector(".sp-screen");
    const card = document.querySelector(".sp-main > .sp-btn");
    return {
      theme: app && app.getAttribute("data-screen-theme"),
      screenBg: screen && getComputedStyle(screen).backgroundColor,
      cardBg: card && getComputedStyle(card).backgroundColor,
      cardBorder: card && getComputedStyle(card).borderTopColor,
      cardText: card && getComputedStyle(card.querySelector(".sp-btn-label")).color,
    };
  });
  assert.strictEqual(light.theme, "light", `${label}: light theme should be reflected on the preview root`);
  assert.strictEqual(light.screenBg, "rgb(255, 255, 255)", `${label}: light preview should use the normal light screen`);
  assert.strictEqual(light.cardBg, "rgb(206, 206, 206)", `${label}: light preview cards should use the normal light card colour`);
  assert.strictEqual(light.cardText, "rgb(0, 0, 0)", `${label}: light preview card text should use the normal light text colour`);
}

async function assertEmptyCellSettings(page, label) {
  const emptyCell = page.locator(".sp-empty-cell").first();
  if ((await emptyCell.count()) === 0) return;
  await emptyCell.click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  const modalLayout = await page.evaluate(() => {
    var modal = document.querySelector(".sp-settings-modal");
    var rect = modal.getBoundingClientRect();
    return {
      visible: rect.width > 1 && rect.height > 1,
      left: rect.left,
      right: rect.right,
      top: rect.top,
      bottom: rect.bottom,
      windowWidth: window.innerWidth,
      windowHeight: window.innerHeight,
      documentScrollWidth: document.documentElement.scrollWidth,
    };
  });
  assert(modalLayout.visible, `${label}: card settings modal should be visible`);
  assert(modalLayout.left >= -1 && modalLayout.right <= modalLayout.windowWidth + 1, `${label}: card settings modal overflows horizontally`);
  assert(modalLayout.top < modalLayout.windowHeight && modalLayout.bottom > 0, `${label}: card settings modal is outside the viewport`);
  assert(
    modalLayout.documentScrollWidth <= modalLayout.windowWidth + 1,
    `${label}: card settings modal introduced horizontal overflow`
  );
  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
}

function postRecord(requestUrl) {
  const url = new URL(requestUrl);
  const parts = url.pathname.split("/").filter(Boolean).map((part) => decodeURIComponent(part));
  return {
    domain: parts[0] || "",
    name: parts[1] || "",
    action: parts.slice(2).join("/"),
    value: url.searchParams.get("value"),
    option: url.searchParams.get("option"),
    path: decodeURIComponent(url.pathname) + url.search,
  };
}

function postMatches(post, expected) {
  return Object.keys(expected).every((key) => post[key] === expected[key]);
}

async function waitForPost(posts, expected, label, startIndex = 0) {
  const deadline = Date.now() + 4000;
  while (Date.now() < deadline) {
    if (posts.slice(startIndex).some((post) => postMatches(post, expected))) return;
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  assert.fail(`${label}: expected POST ${JSON.stringify(expected)}, got ${JSON.stringify(posts.slice(startIndex), null, 2)}`);
}

function backupButtons(count) {
  const buttons = Array.from({ length: count }, () => ({}));
  buttons[0] = { entity: "light.kitchen", label: "Kitchen", icon: "Lightbulb", icon_on: "Lightbulb" };
  buttons[1] = { entity: "sensor.energy", label: "Energy", icon: "Gauge", sensor: "sensor.energy", unit: "W", type: "sensor", precision: "0" };
  buttons[2] = { label: "Rooms", icon: "Home", type: "subpage" };
  buttons[3] = { entity: "media_player.living", label: "Media", type: "media", sensor: "play_pause" };
  return buttons;
}

function backupFixture(device, slots) {
  return {
    version: 2,
    format: "espcontrol.backup",
    device,
    source: { device, slots },
    exported_at: "2026-05-24T12:00:00.000Z",
    button_order: "1,2,3w,4",
    button_on_color: "AA5500",
    button_off_color: "101010",
    sensor_card_color: "202020",
    buttons: backupButtons(slots),
    subpages: {
      3: "1,B|media_player.living:Living:Speaker:Auto:play_pause::media",
    },
    settings: {
      indoor_temp_enable: true,
      outdoor_temp_enable: false,
      indoor_temp_entity: "sensor.indoor_temperature",
      outdoor_temp_entity: "sensor.outdoor_temperature",
      temperature_unit: "°C",
      clock_bar: true,
      network_status_icon: true,
      temperature_degree_symbol: true,
      timezone: "Europe/London (GMT+0)",
      clock_format: "24h",
      ntp_server_1: "pool.ntp.org",
      ntp_server_2: "time.nist.gov",
      ntp_server_3: "time.cloudflare.com",
      month_names: "Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec",
      screensaver_mode: "timer",
      presence_sensor_entity: "binary_sensor.office_presence",
      media_player_sleep_prevention: true,
      media_player_sleep_prevention_entity: "media_player.living",
      screensaver_action: "dim",
      clock_brightness_day: 44,
      clock_brightness_night: 22,
      screensaver_dimmed_brightness: 15,
      screensaver_timeout: 60,
      home_screen_timeout: 120,
      screen_rotation: "90",
    },
    screen: {
      brightness_day: 88,
      brightness_night: 55,
      automatic_brightness: false,
      schedule_enabled: true,
      schedule_on_hour: 7,
      schedule_off_hour: 22,
      schedule_mode: "clock",
      schedule_wake_timeout: 30,
      schedule_wake_brightness: 70,
      schedule_dimmed_brightness: 12,
      schedule_clock_brightness: 40,
    },
  };
}

function writeJsonFixture(name, value) {
  fs.mkdirSync(FAILURE_DIR, { recursive: true });
  const file = path.join(os.tmpdir(), `${name}-${process.pid}-${Date.now()}.json`);
  fs.writeFileSync(file, typeof value === "string" ? value : JSON.stringify(value, null, 2));
  return file;
}

async function openBackupControls(page) {
  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  if (!(await page.getByRole("button", { name: "Import" }).isVisible())) {
    await page.getByText("Backup", { exact: true }).click();
  }
  await page.getByRole("button", { name: "Import" }).waitFor({ state: "visible" });
}

async function importBackup(page, data, name) {
  const file = writeJsonFixture(name, data);
  await openBackupControls(page);
  const chooserPromise = page.waitForEvent("filechooser");
  await page.getByRole("button", { name: "Import" }).click();
  const chooser = await chooserPromise;
  await chooser.setFiles(file);
}

async function startBannerCapture(page) {
  await page.evaluate(() => {
    window.__bannerMessages = [];
    if (!window.__bannerTextCaptureInstalled) {
      var descriptor = Object.getOwnPropertyDescriptor(Node.prototype, "textContent");
      if (descriptor && descriptor.set && descriptor.get) {
        Object.defineProperty(Node.prototype, "textContent", {
          get: function () { return descriptor.get.call(this); },
          set: function (value) {
            if (this.classList && this.classList.contains("sp-banner")) {
              window.__bannerMessages = window.__bannerMessages || [];
              window.__bannerMessages.push({ className: this.className || "", text: String(value || "") });
            }
            return descriptor.set.call(this, value);
          },
          configurable: true,
        });
        window.__bannerTextCaptureInstalled = true;
      }
    }
    if (window.__bannerObserver) window.__bannerObserver.disconnect();
    var banner = document.querySelector(".sp-banner");
    if (!banner) return;
    function record() {
      window.__bannerMessages.push({
        className: banner.className,
        text: banner.textContent || "",
      });
    }
    window.__bannerObserver = new MutationObserver(record);
    window.__bannerObserver.observe(banner, { attributes: true, childList: true, subtree: true });
    record();
  });
}

async function assertBackupImportSmoke(page, posts, slug) {
  const before = posts.length;
  await importBackup(page, backupFixture(slug, 20), "same-device-backup");
  await page.waitForSelector(".sp-banner.sp-success");
  assert((await page.locator(".sp-banner").textContent()).includes("Configuration imported successfully"), "same-device import succeeds");
  await waitForPost(posts, { domain: "text", name: "Button On Color", action: "set", value: "AA5500" }, "backup color import", before);
  await waitForPost(posts, { domain: "text", name: "Button 3 Config", action: "set" }, "backup subpage button config", before);
  await waitForPost(posts, { domain: "text", name: "Subpage 3 Config", action: "set" }, "backup subpage config", before);
  await waitForPost(posts, { domain: "select", name: "Screen: Timezone", action: "set", option: "Europe/London (GMT+0)" }, "backup timezone import", before);
  await waitForPost(posts, { domain: "number", name: "Screen: Daytime Brightness", action: "set", value: "88" }, "backup brightness import", before);
  await waitForPost(posts, { domain: "select", name: "Screen: Rotation", action: "set", option: "90" }, "backup rotation import", before);

  await importBackup(page, "{", "invalid-backup");
  await page.waitForSelector(".sp-banner.sp-error");
  assert((await page.locator(".sp-banner").textContent()).includes("could not parse JSON"), "invalid JSON shows an error");

  await startBannerCapture(page);
  await importBackup(page, backupFixture("different-panel", 3), "cross-device-backup");
  await page.waitForSelector(".sp-banner.sp-success");
  const warnings = await page.evaluate(() => window.__bannerMessages || []);
  assert(
    warnings.some((entry) => entry.className.includes("sp-warning") &&
      (entry.text.includes("different panel") || entry.text.includes("slots"))),
    `cross-device import shows an adaptation warning: ${JSON.stringify(warnings)}`
  );
}

async function entitySuggestionValues(page, inputSelector) {
  await page.locator(inputSelector).fill("");
  await page.waitForSelector(".sp-entity-dropdown.sp-open .sp-entity-option");
  return page.locator(".sp-entity-dropdown.sp-open .sp-entity-option").evaluateAll((options) => {
    return options.map((option) => option.textContent || "");
  });
}

async function assertEditAndApplySmoke(page, posts, errors) {
  const before = posts.length;
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");

  await page.locator('.sp-main [data-slot="1"]').click();
  await page.getByRole("button", { name: /Edit/ }).click();
  const switchSuggestions = await entitySuggestionValues(page, "#sp-inp-entity");
  assert(switchSuggestions.includes("light.kitchen"), "switch card suggestions include a recently used light");
  assert(!switchSuggestions.includes("sensor.energy"), "switch card suggestions exclude recently used sensors");
  assert(!switchSuggestions.includes("media_player.living"), "switch card suggestions exclude recently used media players");
  await page.locator("#sp-inp-label").fill("Kitchen Main");
  await page.locator("#sp-inp-entity").fill("switch.kitchen_main");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(posts, {
    domain: "text",
    name: "Button 1 Config",
    action: "set",
    value: "switch.kitchen_main;Kitchen Main;Lightbulb;Lightbulb",
  }, "switch card edit", before);

  await page.locator('.sp-main [data-slot="2"]').click();
  await page.getByRole("button", { name: /Edit/ }).click();
  await page.locator("#sp-inp-label").fill("Energy Usage");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(posts, {
    domain: "text",
    name: "Button 2 Config",
    action: "set",
    value: "sensor.energy;Energy Usage;Gauge;Auto;sensor.energy;W;sensor;0",
  }, "sensor card edit", before);

  await page.locator('.sp-main [data-slot="4"]').click();
  await page.getByRole("button", { name: /Edit/ }).click();
  await page.locator("#sp-inp-label").fill("Living Media");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(posts, {
    domain: "text",
    name: "Button 4 Config",
    action: "set",
  }, "media card edit", before);

  await page.getByRole("button", { name: "Apply Configuration" }).click();
  await waitForPost(posts, {
    domain: "button",
    name: "Apply Configuration",
    action: "press",
  }, "apply configuration", before);
  assert.deepStrictEqual(errors, [], "browser errors were reported during edit interactions");
}

async function runCase(browser, testCase) {
  const context = await browser.newContext({ viewport: testCase.viewport });
  await installRoutes(context, testCase.slug);
  const page = await context.newPage();
  const errors = [];
  const posts = [];

  page.on("pageerror", (error) => errors.push(error.message));
  page.on("console", (message) => {
    if (message.type() === "error") errors.push(message.text());
  });
  page.on("request", (request) => {
    const requestUrl = new URL(request.url());
    if (request.method() === "POST" && requestUrl.hostname === "espcontrol.test") {
      posts.push(postRecord(request.url()));
    }
  });
  await installFakeEventSource(page);

  try {
    await page.goto(`http://espcontrol.test/${testCase.slug}?events=1`, { waitUntil: "domcontentloaded" });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(() => window.__eventSources && window.__eventSources.length > 0);
    await page.evaluate((events) => window.__seedEspState(events), seededEvents());
    await page.waitForSelector(".sp-main > .sp-btn");
    await page.waitForTimeout(100);

    assert.deepStrictEqual(errors, [], `${testCase.name}: browser errors were reported`);
    assertNoLayoutBreaks(await measureCoreLayout(page), testCase.name);
    await assertSettingsPage(page, testCase.name, testCase);
    if (testCase.monochromeDisplay) await assertEpaperThemePreview(page, testCase.name);
    assertNoLayoutBreaks(await measureCoreLayout(page), `${testCase.name} after settings`);
    await assertEmptyCellSettings(page, testCase.name);
    if (testCase.exerciseInteractions) {
      await assertBackupImportSmoke(page, posts, testCase.slug);
      await assertEditAndApplySmoke(page, posts, errors);
    }
  } catch (error) {
    fs.mkdirSync(FAILURE_DIR, { recursive: true });
    await page.screenshot({ path: path.join(FAILURE_DIR, `${testCase.name}-${testCase.slug}.png`), fullPage: true });
    throw error;
  } finally {
    await context.close();
  }
}

(async function main() {
  const browser = await chromium.launch();
  try {
    for (const testCase of CASES) {
      await runCase(browser, testCase);
    }
  } finally {
    await browser.close();
  }
  console.log(`Browser web smoke checks passed for ${CASES.length} generated layouts.`);
})().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
