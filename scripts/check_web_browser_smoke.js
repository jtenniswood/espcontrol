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
  const sharedFourInchSquareSlugs = new Set([
    "esp32-p4-86",
    "guition-esp32-s3-4848s040",
  ]);
  return Object.entries(manifest.devices || {}).map(([slug, device]) => {
    const aspect = parseAspect(device.web && device.web.screen && device.web.screen.aspect);
    const orientation = orientationFor(aspect.ratio);
    return {
      name: `${orientation}-${slug}`,
      slug,
      slots: device.slots,
      viewport: viewportFor(aspect.ratio),
      isEpaper: device.web && device.web.previewTheme === "epaper",
      coverArtSquareOverlay: !!(device.web && device.web.coverArtSquareOverlay),
      minVisibleCards: device.web && device.web.infoOnly ? 1 : 4,
      exerciseInteractions: slug === "guition-esp32-p4-jc8012p4a1",
      exerciseDeviceMocks: sharedFourInchSquareSlugs.has(slug),
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
    { id: "switch-screen__clock_bar_time", state: "ON", value: true },
    { id: "switch-screen__network_status_icon", state: "ON", value: true },
    { id: "switch-indoor_temp_enable", state: "ON", value: true },
    { id: "switch-outdoor_temp_enable", state: "ON", value: true },
    { id: "text-indoor_temp_entity", state: "sensor.indoor_temperature" },
    { id: "text-outdoor_temp_entity", state: "sensor.outdoor_temperature" },
    { id: "select-screen__temperature_unit", state: "Auto", value: "Auto", option: ["Auto", "°C", "°F"] },
    { id: "switch-screen__temperature_degree_symbol", state: "ON", value: true },
    { id: "select-screen__timezone", state: "Europe/London (GMT+0)", value: "Europe/London (GMT+0)", option: ["Europe/London (GMT+0)", "America/New_York (GMT-5)"] },
    { id: "select-screen__language", state: "en", value: "en", option: ["en"] },
    { id: "select-screen__clock_format", state: "24h", value: "24h", option: ["12h", "24h"] },
    { id: "select-screen__rotation", state: "0", value: "0", option: ["0", "90", "180", "270"] },
    { id: "number-screensaver_timeout", state: "300", value: 300, min: 10, max: 3600 },
    { id: "text-subpage_6_config", state: "1,B|media_player.living:Living:Speaker:Auto:play_pause::media" },
  ];
  BUTTON_FIXTURES.forEach((state, index) => {
    events.push({ id: `text-button_${index + 1}_config`, state });
  });
  return events;
}

function rotationStartupBaseEvents(includeRotation = true) {
  const events = [
    { id: "text-button_on_color", state: "0073FF" },
    { id: "text-button_off_color", state: "CECECE" },
    { id: "text-sensor_card_color", state: "DEDEDE" },
  ];
  if (includeRotation) {
    events.push({ id: "select-screen__rotation", state: "90", value: "90", option: ["0", "90", "180", "270"] });
  }
  BUTTON_FIXTURES.forEach((state, index) => {
    events.push({ id: `text-button_${index + 1}_config`, state });
  });
  return events;
}

function assertNoLayoutBreaks(result, label, options = {}) {
  const minVisibleCards = options.minVisibleCards || BUTTON_FIXTURES.length;
  assert(result.appVisible, `${label}: #sp-app should be visible`);
  assert(result.screenVisible, `${label}: .sp-screen should be visible`);
  assert(result.mainVisible, `${label}: .sp-main should be visible`);
  assert(result.applyVisible, `${label}: apply controls should be visible`);
  assert(result.gridChildren > 0, `${label}: grid should render cells`);
  assert(result.visibleGridChildren > 0, `${label}: grid cells should have visible size`);
  assert(result.visibleCards >= minVisibleCards, `${label}: seeded cards should render`);
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

async function measureRotationStartupLayout(page) {
  return page.evaluate(() => {
    var screen = document.querySelector(".sp-screen");
    var main = document.querySelector(".sp-main");
    var screenRect = screen ? screen.getBoundingClientRect() : null;
    var style = main ? getComputedStyle(main) : null;
    return {
      loading: !!main && main.classList.contains("sp-grid-loading"),
      busy: main ? main.getAttribute("aria-busy") : null,
      gridHidden: !!style && style.visibility === "hidden",
      visibleCards: document.querySelectorAll(".sp-main > .sp-btn").length,
      screenWidth: screenRect ? screenRect.width : 0,
      screenHeight: screenRect ? screenRect.height : 0,
      gridTemplateColumns: style ? style.gridTemplateColumns : "",
      gridTemplateRows: style ? style.gridTemplateRows : "",
    };
  });
}

function gridTrackCount(value) {
  value = String(value || "").trim();
  if (!value) return 0;
  return value.split(/\s+/).length;
}

function assertPortraitGridLayout(result, label) {
  assert(!result.loading, `${label}: grid should no longer be waiting for startup rotation`);
  assert.strictEqual(result.busy, null, `${label}: grid should not remain aria-busy`);
  assert.strictEqual(result.gridHidden, false, `${label}: grid should be visible after startup rotation is known`);
  assert(result.visibleCards >= BUTTON_FIXTURES.length, `${label}: saved cards should render`);
  assert(result.screenHeight > result.screenWidth, `${label}: preview should end in portrait orientation`);
  assert.strictEqual(gridTrackCount(result.gridTemplateColumns), 3, `${label}: portrait grid should use 3 columns`);
  assert.strictEqual(gridTrackCount(result.gridTemplateRows), 5, `${label}: portrait grid should use 5 rows`);
}

async function assertRotationStartupOrdering(browser) {
  const slug = "guition-esp32-p4-jc1060p470";
  const context = await browser.newContext({ viewport: { width: 1100, height: 1000 } });
  await installRoutes(context, slug);
  const page = await context.newPage();
  await installFakeEventSource(page);
  try {
    await page.goto(`http://espcontrol.test/${slug}?events=1`, { waitUntil: "domcontentloaded" });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(() => window.__eventSources && window.__eventSources.length > 0);

    await page.evaluate((events) => window.__seedEspState(events), [
      { id: "text-button_order", state: "1,2,3w,4,5" },
    ].concat(rotationStartupBaseEvents(false)));
    let layout = await measureRotationStartupLayout(page);
    assert(layout.loading, "button_order before rotation: grid should stay in startup loading state");
    assert.strictEqual(layout.busy, "true", "button_order before rotation: grid should be marked busy");
    assert(layout.gridHidden, "button_order before rotation: grid should be hidden until rotation arrives");
    assert.strictEqual(layout.visibleCards, 0, "button_order before rotation: cards should not render in the default layout first");

    await page.evaluate(() => window.__seedEspState([
      { id: "select-screen__rotation", state: "90", value: "90", option: ["0", "90", "180", "270"] },
    ]));
    await page.waitForSelector(".sp-main > .sp-btn");
    layout = await measureRotationStartupLayout(page);
    assertPortraitGridLayout(layout, "button_order before rotation");
  } finally {
    await context.close();
  }

  const reverseContext = await browser.newContext({ viewport: { width: 1100, height: 1000 } });
  await installRoutes(reverseContext, slug);
  const reversePage = await reverseContext.newPage();
  await installFakeEventSource(reversePage);
  try {
    await reversePage.goto(`http://espcontrol.test/${slug}?events=1`, { waitUntil: "domcontentloaded" });
    await reversePage.waitForSelector("#sp-app");
    await reversePage.waitForFunction(() => window.__eventSources && window.__eventSources.length > 0);
    await reversePage.evaluate((events) => window.__seedEspState(events), rotationStartupBaseEvents(true).concat([
      { id: "text-button_order", state: "1,2,3w,4,5" },
    ]));
    await reversePage.waitForSelector(".sp-main > .sp-btn");
    assertPortraitGridLayout(await measureRotationStartupLayout(reversePage), "rotation before button_order");
  } finally {
    await reverseContext.close();
  }

  const fallbackContext = await browser.newContext({ viewport: { width: 1100, height: 1000 } });
  await installRoutes(fallbackContext, slug);
  const fallbackPage = await fallbackContext.newPage();
  await installFakeEventSource(fallbackPage);
  try {
    await fallbackPage.goto(`http://espcontrol.test/${slug}?events=1`, { waitUntil: "domcontentloaded" });
    await fallbackPage.waitForSelector("#sp-app");
    await fallbackPage.waitForFunction(() => window.__eventSources && window.__eventSources.length > 0);
    await fallbackPage.evaluate((events) => window.__seedEspState(events), [
      { id: "text-button_order", state: "1,2,3w,4,5" },
    ].concat(rotationStartupBaseEvents(false)));
    let layout = await measureRotationStartupLayout(fallbackPage);
    assert(layout.loading, "rotation fallback: grid should wait briefly when rotation support exists but no value has arrived");
    await fallbackPage.waitForFunction(() => {
      var main = document.querySelector(".sp-main");
      return main && !main.classList.contains("sp-grid-loading") && document.querySelectorAll(".sp-main > .sp-btn").length >= 4;
    }, null, { timeout: 3000 });
    layout = await measureRotationStartupLayout(fallbackPage);
    assert(!layout.loading, "rotation fallback: grid should appear after fallback timeout");
    assert.strictEqual(layout.gridHidden, false, "rotation fallback: grid should be visible after fallback timeout");
    assert(layout.visibleCards >= BUTTON_FIXTURES.length, "rotation fallback: saved cards should render after fallback timeout");
  } finally {
    await fallbackContext.close();
  }
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
  assert.strictEqual(themeVisible, !!options.isEpaper, `${label}: theme selector visibility should match display type`);
  assert.strictEqual(onColorVisible, !options.isEpaper, `${label}: color controls visibility should match display type`);
  const nightScheduleCard = page.locator("#sp-settings .card").filter({
    has: page.locator(".card-header h3", { hasText: /^Night Schedule$/ }),
  }).first();
  assert(await nightScheduleCard.isVisible(), `${label}: night schedule settings card should render`);
  await nightScheduleCard.locator(".card-header").click();
  const nightScheduleInfo = page.locator("#sp-night-schedule-info");
  assert(await nightScheduleInfo.isVisible(), `${label}: night schedule override info panel should render`);
  assert.strictEqual(
    await nightScheduleInfo.innerText(),
    "Time-based Night Schedule overrides screensaver presence wake and Media Cover Art while it is active. Use Sensor mode when you want presence to control the night schedule.",
    `${label}: night schedule override info panel text should match`
  );
  if (!options.isEpaper) {
    const coverArtCard = page.locator("#sp-settings .card").filter({
      has: page.locator(".card-header h3", { hasText: /^Media Cover Art$/ }),
    }).first();
    assert(await coverArtCard.isVisible(), `${label}: media cover art settings card should render`);
    await coverArtCard.locator(".card-header").click();
    assert.strictEqual(
      await page.locator("#sp-cover-art-info").count(),
      0,
      `${label}: media cover art override info panel should not render`
    );
    assert.strictEqual(
      await page.locator("#sp-set-ss-media-sleep-prevention").count(),
      0,
      `${label}: keep-screen-awake option should not render separately`
    );
    await coverArtCard.locator("#sp-set-ss-cover-art-enable + .sp-toggle-track").click();
    assert(
      await coverArtCard.locator("#sp-set-ss-cover-art-player").isVisible(),
      `${label}: media player entity field should render when cover art is enabled`
    );
    assert(
      await coverArtCard.locator("#sp-set-ss-cover-art-delay").isVisible(),
      `${label}: cover art show-after field should render when cover art is enabled`
    );
    assert.strictEqual(
      await page.locator("#sp-set-ss-track-overlay").count(),
      options.coverArtSquareOverlay ? 1 : 0,
      `${label}: track overlay duration visibility should match square cover art layout`
    );
    assert(
      await coverArtCard.getByText("Advanced Options", { exact: true }).isVisible(),
      `${label}: media cover art advanced options should render`
    );
    assert.strictEqual(
      await coverArtCard.locator("#sp-set-ss-cover-art-conditions").isVisible(),
      false,
      `${label}: cover art conditions field should be hidden until advanced filtering is enabled`
    );
    await coverArtCard.getByText("Advanced Options", { exact: true }).click();
    assert(
      await coverArtCard.getByText("Hide for external source inputs", { exact: true }).isVisible(),
      `${label}: external source input option should render inside advanced options`
    );
    assert(
      await coverArtCard.getByText("Advanced Filtering", { exact: true }).isVisible(),
      `${label}: advanced filtering toggle should render inside advanced options`
    );
    await coverArtCard.locator("#sp-set-ss-cover-art-filtering + .sp-toggle-track").click();
    assert(
      await coverArtCard.locator("#sp-set-ss-cover-art-conditions").isVisible(),
      `${label}: cover art conditions field should render after advanced filtering is enabled`
    );
    assert(
      await page.locator("#sp-set-ss-cover-art-server").count() === 0,
      `${label}: cover art fallback URL field should not render`
    );
  }
  assert(
    await page.locator("#sp-set-ss-media-player").count() === 0,
    `${label}: timer media player field should not render`
  );
  assert.strictEqual(
    await page.locator("#sp-set-sensor-media-player-enable").count(),
    0,
    `${label}: sensor cover art override should not render`
  );
  const homeAssistantSettingsCard = page.locator("#sp-settings .card").filter({
    has: page.locator(".card-header h3", { hasText: /^Home Assistant Settings$/ }),
  }).first();
  assert(await homeAssistantSettingsCard.isVisible(), `${label}: Home Assistant settings card should render`);
  assert(
    !(await homeAssistantSettingsCard.locator("#sp-set-ha-artwork-port").isVisible()),
    `${label}: Home Assistant settings card should be collapsed by default`
  );
  await homeAssistantSettingsCard.locator(".card-header").click();
  assert(
    await homeAssistantSettingsCard.locator("#sp-set-ha-artwork-port").isVisible(),
    `${label}: Home Assistant port field should render in Home Assistant settings`
  );
  assert.strictEqual(
    await homeAssistantSettingsCard.locator("#sp-set-ha-artwork-port").inputValue(),
    "8123",
    `${label}: Home Assistant port field should default to 8123`
  );
  assert(
    await homeAssistantSettingsCard.locator("#sp-set-ha-artwork-port.sp-input--no-stepper").count() === 1,
    `${label}: Home Assistant port field should hide browser stepper controls`
  );
  const overflow = await page.evaluate(() => document.documentElement.scrollWidth > window.innerWidth + 1);
  assert(!overflow, `${label}: settings page has horizontal overflow`);
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
}

async function assertMobileTabLayout(page, label, restoreViewport) {
  await page.setViewportSize({ width: 360, height: 740 });
  await page.waitForTimeout(100);
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  let mobile = await page.evaluate(() => {
    var support = document.querySelector(".sp-support-btn");
    var supportStyle = support ? getComputedStyle(support) : null;
    var screen = document.querySelector(".sp-screen").getBoundingClientRect();
    return {
      tab: document.querySelector("#sp-app").getAttribute("data-active-tab"),
      viewportMeta: document.querySelector('meta[name="viewport"]') &&
        document.querySelector('meta[name="viewport"]').getAttribute("content"),
      supportVisible: !!support && supportStyle.display !== "none" && support.getBoundingClientRect().width > 1,
      screenWidth: screen.width,
      documentScrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
  assert.strictEqual(mobile.tab, "screen", `${label}: screen tab is marked active on mobile`);
  assert.strictEqual(mobile.viewportMeta, "width=device-width,initial-scale=1", `${label}: web app should provide mobile viewport metadata`);
  assert(mobile.supportVisible, `${label}: support button remains visible on the screen tab`);
  assert(mobile.screenWidth <= mobile.windowWidth + 1, `${label}: mobile screen preview fits viewport`);
  assert(
    mobile.documentScrollWidth <= mobile.windowWidth + 1,
    `${label}: mobile screen tab has horizontal overflow`
  );

  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const nightScheduleCard = page.locator("#sp-settings .card").filter({ hasText: "Night Schedule" }).first();
  await nightScheduleCard.locator(".card-header").click();
  mobile = await page.evaluate(() => {
    var support = document.querySelector(".sp-support-btn");
    var supportStyle = support ? getComputedStyle(support) : null;
    var activeCard = Array.from(document.querySelectorAll("#sp-settings .card")).find(function (card) {
      return !card.classList.contains("collapsed");
    });
    var activeCardRect = activeCard ? activeCard.getBoundingClientRect() : null;
    var headers = Array.from(document.querySelectorAll("#sp-settings .card-header")).map(function (header) {
      var rect = header.getBoundingClientRect();
      return { width: rect.width, left: rect.left, right: rect.right };
    });
    return {
      tab: document.querySelector("#sp-app").getAttribute("data-active-tab"),
      supportVisible: !!support && supportStyle.display !== "none" && support.getBoundingClientRect().width > 1,
      activeCardVisible: !!activeCardRect && activeCardRect.width > 1 && activeCardRect.height > 1,
      activeCardWithinViewport: !!activeCardRect && activeCardRect.left >= -1 && activeCardRect.right <= window.innerWidth + 1,
      headersWithinViewport: headers.every(function (rect) {
        return rect.left >= -1 && rect.right <= window.innerWidth + 1;
      }),
      documentScrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
  assert.strictEqual(mobile.tab, "settings", `${label}: settings tab is marked active on mobile`);
  assert.strictEqual(mobile.supportVisible, false, `${label}: support button should not cover mobile settings`);
  assert(mobile.activeCardVisible, `${label}: expanded mobile settings card should be visible`);
  assert(mobile.activeCardWithinViewport, `${label}: expanded mobile settings card should fit viewport`);
  assert(mobile.headersWithinViewport, `${label}: mobile settings headers should fit viewport`);
  assert(
    mobile.documentScrollWidth <= mobile.windowWidth + 1,
    `${label}: mobile settings tab has horizontal overflow`
  );
  await page.setViewportSize(restoreViewport);
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  await page.waitForTimeout(100);
}

async function assertMobileDeviceViewport(browser, testCase) {
  const context = await browser.newContext({
    viewport: { width: 360, height: 740 },
    deviceScaleFactor: 3,
    hasTouch: true,
    isMobile: true,
  });
  await installRoutes(context, testCase.slug);
  const page = await context.newPage();
  await installFakeEventSource(page);
  try {
    await page.goto(`http://espcontrol.test/${testCase.slug}?events=1`, { waitUntil: "domcontentloaded" });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(() => window.__eventSources && window.__eventSources.length > 0);
    await page.evaluate((events) => window.__seedEspState(events), seededEvents());
    await page.waitForSelector(".sp-main > .sp-btn");
    await page.getByRole("tab", { name: "Settings" }).click();
    await page.waitForSelector("#sp-settings.sp-page.active");
    const mobile = await page.evaluate(() => {
      var support = document.querySelector(".sp-support-btn");
      var supportStyle = support ? getComputedStyle(support) : null;
      return {
        viewportMeta: document.querySelector('meta[name="viewport"]') &&
          document.querySelector('meta[name="viewport"]').getAttribute("content"),
        windowWidth: window.innerWidth,
        documentScrollWidth: document.documentElement.scrollWidth,
        supportVisible: !!support && supportStyle.display !== "none" && support.getBoundingClientRect().width > 1,
      };
    });
    assert.strictEqual(
      mobile.viewportMeta,
      "width=device-width,initial-scale=1",
      `${testCase.name}: mobile browser should receive device-width viewport metadata`
    );
    assert(
      mobile.windowWidth <= 380,
      `${testCase.name}: mobile browser should not render with a desktop layout viewport (${mobile.windowWidth}px)`
    );
    assert(
      mobile.documentScrollWidth <= mobile.windowWidth + 1,
      `${testCase.name}: mobile device settings tab has horizontal overflow`
    );
    assert.strictEqual(mobile.supportVisible, false, `${testCase.name}: support button should not cover settings on mobile devices`);
  } catch (error) {
    fs.mkdirSync(FAILURE_DIR, { recursive: true });
    await page.screenshot({ path: path.join(FAILURE_DIR, `${testCase.name}-${testCase.slug}-mobile.png`), fullPage: true });
    throw error;
  } finally {
    await context.close();
  }
}

async function assertEmptyCellSettings(page, posts, label) {
  const emptyCell = page.locator(".sp-empty-cell:not(.sp-info-only-hidden)").first();
  if ((await emptyCell.count()) === 0) return;
  assert.strictEqual(await page.locator(".sp-btn .sp-add-pill").count(), 0, `${label}: configured slots do not show add controls`);
  const addPill = emptyCell.locator(".sp-add-pill");
  await page.mouse.move(0, 0);
  assert.strictEqual(
    await addPill.evaluate((el) => getComputedStyle(el).opacity),
    "0",
    `${label}: empty-slot add control is hidden until hover`
  );
  await emptyCell.hover();
  await page.waitForFunction((el) => parseFloat(getComputedStyle(el).opacity) > 0.9, await addPill.elementHandle());
  const pos = await emptyCell.getAttribute("data-pos");
  const before = posts.length;
  await emptyCell.click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.waitForTimeout(100);
  assert.strictEqual(posts.length, before, `${label}: opening a new card draft should not post immediately`);
  assert(await page.locator("#sp-card-type-picker").isVisible(), `${label}: new card draft shows the card type grid`);
  assert(await page.locator(".sp-card-type-option").filter({ hasText: "Switch" }).first().isVisible(), `${label}: new card draft shows Switch as a card type`);
  assert.strictEqual(await page.locator("#sp-inp-type").count(), 0, `${label}: new card draft does not show the compact type dropdown before selection`);
  assert.strictEqual(await page.locator(".sp-settings-modal .sp-save-btn").count(), 0, `${label}: new card draft hides Save until a type is selected`);
  assert.strictEqual(await page.locator(".sp-settings-modal .sp-delete-btn").count(), 0, `${label}: new card draft hides Delete before save`);
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
  assert.strictEqual(posts.length, before, `${label}: closing a new card draft before choosing a type should not post`);
  await page.locator(`.sp-main [data-pos="${pos}"].sp-empty-cell`).waitFor({ state: "visible" });

  await page.locator(`.sp-main [data-pos="${pos}"]`).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.getByRole("button", { name: "Switch card type" }).click();
  await page.locator("#sp-inp-entity").waitFor({ state: "visible" });
  assert(await page.locator(".sp-settings-modal .sp-save-btn").isVisible(), `${label}: selecting a card type shows Save`);
  assert.strictEqual(await page.locator(".sp-settings-modal .sp-delete-btn").count(), 0, `${label}: unsaved new card keeps Delete hidden after type selection`);
  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
  await page.waitForTimeout(100);
  assert.strictEqual(posts.length, before, `${label}: closing a typed new card draft before Save should not post`);
  await page.locator(`.sp-main [data-pos="${pos}"].sp-empty-cell`).waitFor({ state: "visible" });

  await page.locator(`.sp-main [data-pos="${pos}"]`).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.getByRole("button", { name: "Switch card type" }).click();
  await page.locator("#sp-inp-label").fill("New Card");
  await page.locator("#sp-inp-entity").fill("switch.new_card");
  await page.getByRole("button", { name: "Save" }).click();
  await page.locator(`.sp-main [data-pos="${pos}"][data-slot]`).waitFor({ state: "visible" });
  const slot = await page.locator(`.sp-main [data-pos="${pos}"]`).getAttribute("data-slot");
  await waitForPost(posts, { domain: "text", name: "button_order", action: "set" }, `${label}: saving new card posts button order`, before);
  await waitForAnyPost(posts, [
    { domain: "text", name: `button_${slot}_config`, action: "set" },
    { domain: "text", name: `Button ${slot} Config`, action: "set" },
  ], `${label}: saving new card posts card config`, before);
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

async function waitForAnyPost(posts, expectedList, label, startIndex = 0) {
  const deadline = Date.now() + 4000;
  while (Date.now() < deadline) {
    if (posts.slice(startIndex).some((post) => expectedList.some((expected) => postMatches(post, expected)))) return;
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  assert.fail(`${label}: expected one of ${JSON.stringify(expectedList)}, got ${JSON.stringify(posts.slice(startIndex), null, 2)}`);
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
      clock_bar_time: true,
      network_status_icon: true,
      temperature_degree_symbol: true,
      timezone: "Europe/London (GMT+0)",
      language: "en",
      clock_format: "24h",
      ntp_server_1: "pool.ntp.org",
      ntp_server_2: "time.nist.gov",
      ntp_server_3: "time.cloudflare.com",
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

async function assertBackupImportSmoke(page, posts, testCase) {
  const before = posts.length;
  await importBackup(page, backupFixture(testCase.slug, testCase.slots), "same-device-backup");
  await page.waitForSelector(".sp-banner.sp-success");
  assert((await page.locator(".sp-banner").textContent()).includes("Configuration imported successfully"), "same-device import succeeds");
  await waitForPost(posts, { domain: "text", name: "button_on_color", action: "set", value: "AA5500" }, "backup color import", before);
  await waitForPost(posts, { domain: "text", name: "button_3_config", action: "set" }, "backup subpage button config", before);
  await waitForPost(posts, { domain: "text", name: "Subpage 3 Config", action: "set" }, "backup subpage config", before);
  await waitForPost(posts, { domain: "select", name: "screen__timezone", action: "set", option: "Europe/London (GMT+0)" }, "backup timezone import", before);
  await waitForPost(posts, { domain: "select", name: "screen__language", action: "set", option: "en" }, "backup language import", before);
  await waitForPost(posts, { domain: "switch", name: "screen__clock_bar_time", action: "turn_on" }, "backup clock bar time reset", before);
  await waitForPost(posts, { domain: "number", name: "Screen: Daytime Brightness", action: "set", value: "88" }, "backup brightness import", before);
  await waitForPost(posts, { domain: "select", name: "screen__rotation", action: "set", option: "90" }, "backup rotation import", before);

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

async function entitySuggestionValues(page, inputSelector, query = "light", expectedValues = []) {
  await page.locator(inputSelector).fill(query);
  const suggestions = await page.waitForFunction(({ selector, query, expectedValues }) => {
    const input = document.querySelector(selector);
    const normalizedQuery = String(query || "").toLowerCase();
    if (!input || String(input.value || "").toLowerCase() !== normalizedQuery) return false;
    const options = Array.from(input.parentElement.querySelectorAll(".sp-entity-dropdown.sp-open .sp-entity-option"));
    if (!options.length) return false;
    const values = options.map((option) => String(option.textContent || ""));
    if (!options.every((option) => String(option.textContent || "").toLowerCase().includes(normalizedQuery))) return false;
    if (!expectedValues.every((value) => values.indexOf(value) !== -1)) return false;
    return values;
  }, { selector: inputSelector, query, expectedValues });
  return suggestions.jsonValue();
}

async function assertEditAndApplySmoke(page, posts, errors) {
  const before = posts.length;
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");

  await page.locator('.sp-main [data-slot="1"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  const switchSuggestions = await entitySuggestionValues(page, "#sp-inp-entity", "light", ["light.kitchen"]);
  assert(switchSuggestions.includes("light.kitchen"), "switch card suggestions include a recently used light");
  assert(!switchSuggestions.includes("sensor.energy"), "switch card suggestions exclude recently used sensors");
  assert(!switchSuggestions.includes("media_player.living"), "switch card suggestions exclude recently used media players");
  await page.locator("#sp-inp-label").fill("Kitchen Main");
  await page.locator("#sp-inp-entity").fill("switch.kitchen_main");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(posts, {
    domain: "text",
    name: "button_1_config",
    action: "set",
    value: "switch.kitchen_main;Kitchen Main;Lightbulb;Lightbulb",
  }, "switch card edit", before);

  await page.locator('.sp-main [data-slot="2"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.locator("#sp-inp-label").fill("Energy Usage");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(posts, {
    domain: "text",
    name: "button_2_config",
    action: "set",
    value: "sensor.energy;Energy Usage;Gauge;Auto;sensor.energy;W;sensor;0",
  }, "sensor card edit", before);

  await page.locator('.sp-main [data-slot="4"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.locator("#sp-inp-label").fill("Living Media");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(posts, {
    domain: "text",
    name: "button_4_config",
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

async function assertClockBarEditorSmoke(page, posts, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");

  async function openClockBarContextMenu(item, expectedAction) {
    await page.locator(`[data-clockbar-item="${item}"]`).click({ button: "right", force: true });
    await page.locator(".sp-ctx-menu").waitFor({ state: "visible" });
    assert(
      await page.locator(".sp-ctx-menu").getByText(expectedAction, { exact: true }).isVisible(),
      `${label}: right click menu for ${item} shows ${expectedAction}`
    );
  }

  const fixedItems = [
    { selector: '[data-clockbar-item="temperature"]', section: "left" },
    { selector: '[data-clockbar-item="time"]', section: "middle" },
    { selector: '[data-clockbar-item="network"]', section: "right" },
  ];
  for (const item of fixedItems) {
    const selector = `${item.selector}[data-clockbar-section="${item.section}"]`;
    const box = await page.locator(selector).boundingBox();
    assert(box, `${label}: ${selector} remains visible in the fixed clock bar`);
  }

  assert.strictEqual(await page.locator("[data-clockbar-add]").count(), 0, `${label}: clock bar add controls are removed`);
  assert.strictEqual(await page.locator('[data-clockbar-item="temperature_2"]').count(), 0, `${label}: extra temperature mini cards are not rendered`);
  assert.strictEqual(await page.locator("#sp-clockbar-add-type").count(), 0, `${label}: add-item selector is not present`);
  assert.strictEqual(await page.locator("#sp-clockbar-weather-entity").count(), 0, `${label}: clock bar weather editor is not present`);
  assert.strictEqual(await page.locator("#sp-clockbar-temperature-entity-0").count(), 0, `${label}: clock bar temperature editor is not present`);

  const before = posts.length;
  await page.locator('.sp-clockbar-section[data-clockbar-section="left"]').hover();
  await page.locator('[data-clockbar-item="time"]').click({ force: true });
  assert.strictEqual(
    await page.locator('[data-clockbar-item="time"]').getAttribute("data-clockbar-section"),
    "middle",
    `${label}: clock bar time remains in the fixed middle section`
  );
  assert.strictEqual(await page.locator(".sp-selection-bar.sp-visible").count(), 1, `${label}: clicking clock bar selects an item`);
  assert((await page.locator(".sp-selection-bar").textContent()).includes("Clock selected"), `${label}: clock selection is labelled`);
  assert.strictEqual(await page.getByRole("button", { name: "Edit", exact: true }).isDisabled(), true, `${label}: clock edit button is disabled`);
  assert.strictEqual(await page.getByRole("button", { name: "Clock bar actions", exact: true }).count(), 0, `${label}: clock selection does not expose redundant actions menu button`);
  const selectedClockItem = page.locator('[data-clockbar-item="time"]');
  await page.mouse.move(0, 0);
  await page.waitForTimeout(250);
  const selectedClockStyle = await selectedClockItem.evaluate((el) => {
    var style = getComputedStyle(el);
    return { borderColor: style.borderTopColor, backgroundColor: style.backgroundColor };
  });
  await selectedClockItem.hover();
  await page.waitForTimeout(250);
  const hoveredSelectedClockStyle = await selectedClockItem.evaluate((el) => {
    var style = getComputedStyle(el);
    return { borderColor: style.borderTopColor, backgroundColor: style.backgroundColor };
  });
  assert.notStrictEqual(
    selectedClockStyle.borderColor,
    "rgba(0, 0, 0, 0)",
    `${label}: selected clock bar item has a visible border`
  );
  assert.deepStrictEqual(
    hoveredSelectedClockStyle,
    selectedClockStyle,
    `${label}: selected clock bar item keeps its selected styling while hovered`
  );
  await openClockBarContextMenu("time", "Hide Clock");
  await page.locator(".sp-ctx-menu").getByText("Hide Clock", { exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "screen__clock_bar_time", action: "turn_off" }, `${label}: hiding clock posts clock switch`, before);
  assert((await page.locator('[data-clockbar-item="time"]').getAttribute("class")).includes("sp-clockbar-hidden"), `${label}: hidden clock is greyed in preview`);
  await openClockBarContextMenu("time", "Show Clock");
  await page.locator(".sp-ctx-menu").getByText("Show Clock", { exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "screen__clock_bar_time", action: "turn_on" }, `${label}: showing clock posts clock switch`, before);

  await openClockBarContextMenu("network", "Hide Connectivity");
  assert((await page.locator(".sp-selection-bar").textContent()).includes("Connectivity selected"), `${label}: connectivity selection is labelled`);
  assert.strictEqual(await page.getByRole("button", { name: "Edit", exact: true }).isDisabled(), true, `${label}: connectivity edit button is disabled`);
  await page.locator(".sp-ctx-menu").getByText("Hide Connectivity", { exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "screen__network_status_icon", action: "turn_off" }, `${label}: hiding connectivity posts network switch`, before);
  assert((await page.locator('[data-clockbar-item="network"]').getAttribute("class")).includes("sp-clockbar-hidden"), `${label}: hidden connectivity is greyed in preview`);
  const hiddenNetworkPreview = await page.locator('[data-clockbar-item="network"] .sp-network-preview').evaluate((el) => {
    const style = getComputedStyle(el);
    const rect = el.getBoundingClientRect();
    return {
      className: el.className,
      opacity: style.opacity,
      width: rect.width,
      height: rect.height,
    };
  });
  assert(hiddenNetworkPreview.className.includes("sp-visible"), `${label}: hidden connectivity keeps the preview icon visible`);
  assert.strictEqual(hiddenNetworkPreview.opacity, "1", `${label}: hidden connectivity icon is not faded out by its own style`);
  assert(hiddenNetworkPreview.width > 0 && hiddenNetworkPreview.height > 0, `${label}: hidden connectivity icon remains measurable`);
  await openClockBarContextMenu("network", "Show Connectivity");
  await page.locator(".sp-ctx-menu").getByText("Show Connectivity", { exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "screen__network_status_icon", action: "turn_on" }, `${label}: showing connectivity posts network switch`, before);

  await page.locator('[data-clockbar-item="temperature"]').click({ force: true });
  assert((await page.locator(".sp-selection-bar").textContent()).includes("Temperature selected"), `${label}: temperature selection is labelled`);
  assert.strictEqual(await page.getByRole("button", { name: "Edit", exact: true }).isDisabled(), false, `${label}: temperature edit button is enabled`);
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.locator("#sp-clockbar-temperature-entity").fill("sensor.porch_temperature");
  const degreeSymbolLabel = page.locator('label[for="sp-clockbar-temperature-degree-symbol"]');
  assert.strictEqual(await degreeSymbolLabel.count(), 1, `${label}: degree-symbol label is clickable`);
  await degreeSymbolLabel.click();
  assert.strictEqual(
    await page.locator("#sp-clockbar-temperature-degree-symbol").isChecked(),
    false,
    `${label}: clicking the degree-symbol label toggles the control`
  );
  await page.getByRole("button", { name: "Save", exact: true }).click();
  await waitForAnyPost(posts, [
    { domain: "text", name: "Clock Bar: Temperature Entities", action: "set", value: "sensor.porch_temperature" },
    { domain: "text", name: "clock_bar__temperature_entities", action: "set", value: "sensor.porch_temperature" },
  ], `${label}: saving temperature posts entity`, before);
  await waitForPost(posts, { domain: "switch", name: "screen__temperature_degree_symbol", action: "turn_off" }, `${label}: saving temperature posts degree symbol`, before);
  assert(
    !(await page.locator('[data-clockbar-item="temperature"] .sp-temp').textContent()).includes("\u00B0"),
    `${label}: saving temperature removes the degree symbol from the preview`
  );
  await page.locator('[data-clockbar-item="temperature"]').click({ force: true });
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  const modalHideButton = page.locator(".sp-settings-modal .sp-hide-btn");
  assert.strictEqual(await modalHideButton.count(), 1, `${label}: temperature modal shows one hide button`);
  assert.strictEqual((await modalHideButton.innerText()).trim(), "Hide", `${label}: temperature modal hide button is labelled`);
  const hideIconClass = await modalHideButton.locator(".mdi").getAttribute("class");
  assert(hideIconClass.includes("mdi-eye-off-outline"), `${label}: temperature modal hide button uses the eye-off icon`);
  assert(!hideIconClass.includes("trash"), `${label}: temperature modal hide button does not use a trash icon`);
  await modalHideButton.click();
  await waitForPost(posts, { domain: "switch", name: "outdoor_temp_enable", action: "turn_off" }, `${label}: hiding temperature posts visibility switch`, before);
  assert((await page.locator('[data-clockbar-item="temperature"]').getAttribute("class")).includes("sp-clockbar-hidden"), `${label}: hidden temperature is greyed in preview`);
  await page.locator('[data-clockbar-item="temperature"]').click({ force: true });
  await page.getByRole("button", { name: "Show", exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "outdoor_temp_enable", action: "turn_on" }, `${label}: showing temperature posts visibility switch`, before);

  await openClockBarContextMenu("temperature", "Edit Temperature");
  assert(
    await page.locator(".sp-ctx-menu").getByText("Hide Temperature", { exact: true }).isVisible(),
    `${label}: right click menu for temperature includes visibility action`
  );
  await page.locator(".sp-ctx-menu").getByText("Hide Temperature", { exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "outdoor_temp_enable", action: "turn_off" }, `${label}: hiding temperature posts visibility switch`, before);
  assert((await page.locator('[data-clockbar-item="temperature"]').getAttribute("class")).includes("sp-clockbar-hidden"), `${label}: hidden temperature is greyed in preview`);
  await openClockBarContextMenu("temperature", "Show Temperature");
  await page.locator(".sp-ctx-menu").getByText("Show Temperature", { exact: true }).click();
  await waitForPost(posts, { domain: "switch", name: "outdoor_temp_enable", action: "turn_on" }, `${label}: showing temperature posts visibility switch`, before);
  assert.strictEqual(await page.locator(".sp-settings-overlay.sp-visible").count(), 0, `${label}: clock bar temperature dialog closes after save`);

  const screenBox = await page.locator(".sp-screen").boundingBox();
  const timeBox = await page.locator('[data-clockbar-item="time"][data-clockbar-section="middle"]').boundingBox();
  assert(screenBox && timeBox, `${label}: middle clock bar time is measurable`);
  const screenCenter = screenBox.x + screenBox.width / 2;
  const timeCenter = timeBox.x + timeBox.width / 2;
  assert(
    Math.abs(timeCenter - screenCenter) <= 1,
    `${label}: middle clock bar time remains centered on the screen`
  );

  const topbarBox = await page.locator(".sp-topbar").boundingBox();
  const networkBox = await page.locator('[data-clockbar-item="network"][data-clockbar-section="right"]').boundingBox();
  const firstCardBox = await page.locator(".sp-main > *").first().boundingBox();
  assert(topbarBox && networkBox, `${label}: network status has a visible bounded area`);
  assert(networkBox.y > topbarBox.y, `${label}: network status does not touch the top of the clock bar`);
  assert(
    networkBox.y + networkBox.height < topbarBox.y + topbarBox.height,
    `${label}: network status does not touch the bottom of the clock bar`
  );
  assert(firstCardBox, `${label}: first card has a visible bounded area`);
  assert(
    networkBox.y + networkBox.height < firstCardBox.y,
    `${label}: network status stays clear of the first card row`
  );

  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const timeSettingsCard = page.locator("#sp-settings .card").filter({ hasText: "Time Settings" }).first();
  const timeSettingsText = await timeSettingsCard.textContent();
  assert(timeSettingsText.includes("Timezone"), `${label}: timezone remains in global time settings`);
  assert(timeSettingsText.includes("Clock Format"), `${label}: clock format remains in global time settings`);
  const clockBarCard = page.locator("#sp-settings .card").filter({ hasText: "Clock Bar" }).first();
  const clockBarText = await clockBarCard.textContent();
  assert(clockBarText.includes("Show Clock Bar"), `${label}: clock bar settings keep the master toggle`);
  assert(!clockBarText.includes("Show Network Status Icon"), `${label}: network toggle moved out of clock bar settings`);
  assert(!clockBarText.includes("Outdoor Temperature"), `${label}: outdoor controls moved out of clock bar settings`);
  assert(!clockBarText.includes("Indoor Temperature"), `${label}: indoor controls moved out of clock bar settings`);
  assert(!clockBarText.includes("Show Degree Symbol"), `${label}: degree-symbol control moved out of clock bar settings`);
  await page.getByRole("tab", { name: "Screen" }).click();
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
    assertNoLayoutBreaks(await measureCoreLayout(page), testCase.name, testCase);
    await assertSettingsPage(page, testCase.name, testCase);
    assertNoLayoutBreaks(await measureCoreLayout(page), `${testCase.name} after settings`, testCase);
    if (testCase.exerciseInteractions) {
      await assertMobileTabLayout(page, testCase.name, testCase.viewport);
    }
    await assertEmptyCellSettings(page, posts, testCase.name);
    if (testCase.exerciseInteractions) {
      await assertClockBarEditorSmoke(page, posts, testCase.name);
      await assertBackupImportSmoke(page, posts, testCase);
      await assertEditAndApplySmoke(page, posts, errors);
    } else if (testCase.exerciseDeviceMocks) {
      await assertBackupImportSmoke(page, posts, testCase);
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
    await assertRotationStartupOrdering(browser);
    for (const testCase of CASES) {
      await runCase(browser, testCase);
      await assertMobileDeviceViewport(browser, testCase);
    }
  } finally {
    await browser.close();
  }
  console.log(`Browser web smoke checks passed for ${CASES.length} generated layouts.`);
})().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
