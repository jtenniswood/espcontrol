#!/usr/bin/env node
"use strict";

const assert = require("assert");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { chromium } = require("playwright");
const { freshWebOutputDir } = require("./web_source");

const ROOT = path.resolve(__dirname, "..");
const MANIFEST_PATH = path.join(ROOT, "devices", "manifest.json");
const WEB_OUTPUT_DIR = freshWebOutputDir();
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
    const aspect = parseAspect(
      device.web && device.web.screen && device.web.screen.aspect,
    );
    const orientation = orientationFor(aspect.ratio);
    return {
      name: `${orientation}-${slug}`,
      slug,
      slots: device.slots,
      viewport: viewportFor(aspect.ratio),
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
  "cover.office_blind;Blind;Blinds Open;Blinds;modal;;cover;;cover_tabs=controls%7Cposition%7Ctilt",
  "alarm_control_panel.house;Alarm;Security;Auto;;;alarm;;",
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
    `<script src="/webserver/www.js?device=${slug}"></script>`,
    "</body>",
    "</html>",
  ].join("");
}

function routeContentType(url) {
  const pathname = typeof url === "string" ? url : url.pathname;
  if (
    typeof url !== "string" &&
    url.hostname === "fonts.googleapis.com" &&
    pathname === "/css2"
  )
    return "text/css";
  if (/\.css(?:$|\?)/.test(pathname)) return "text/css";
  if (/\.(?:png|jpg|jpeg|gif|webp|svg)(?:$|\?)/.test(pathname))
    return "image/svg+xml";
  return "text/plain";
}

function publicFirmwareManifest(slug) {
  return {
    version: "v1.13.0",
    builds: [{
      ota: {
        path: `${slug}.ota.bin`,
        md5: "0123456789abcdef0123456789abcdef",
      },
    }],
  };
}

function publicFirmwareVersions(slug) {
  return {
    device: slug,
    versions: ["v1.13.0", "v1.12.0", "v1.11.0"].map((version, index) => ({
      version,
      ota: {
        path: index === 0 ? `${slug}.ota.bin` : `versions/${version}/${slug}.ota.bin`,
        md5: "0123456789abcdef0123456789abcdef",
      },
    })),
  };
}

async function installRoutes(context, slug) {
  const scriptPath = path.join(WEB_OUTPUT_DIR, "www.js");
  assert(
    fs.existsSync(scriptPath),
    `${slug}: generated web UI does not exist at ${scriptPath}`,
  );

  await context.route("**/*", async (route) => {
    const requestUrl = new URL(route.request().url());
    if (
      requestUrl.hostname === "espcontrol.test" &&
      requestUrl.pathname === `/${slug}`
    ) {
      await route.fulfill({
        status: 200,
        contentType: "text/html",
        body: htmlFor(slug),
      });
      return;
    }
    if (
      requestUrl.hostname === "espcontrol.test" &&
      requestUrl.pathname === "/webserver/www.js"
    ) {
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
    if (requestUrl.hostname === "jtenniswood.github.io") {
      if (requestUrl.pathname.endsWith("/manifest.json")) {
        await route.fulfill({
          status: 200,
          contentType: "application/json",
          body: JSON.stringify(publicFirmwareManifest(slug)),
        });
        return;
      }
      if (requestUrl.pathname.endsWith("/versions.json")) {
        await route.fulfill({
          status: 200,
          contentType: "application/json",
          body: JSON.stringify(publicFirmwareVersions(slug)),
        });
        return;
      }
    }
    await route.fulfill({
      status: 200,
      contentType: routeContentType(requestUrl),
      body: "",
    });
  });
}

async function installFakeEventSource(page) {
  await page.addInitScript(() => {
    window.__eventSources = [];
    window.__seedEspState = function (events) {
      if (!window.__eventSources.length)
        throw new Error("No EventSource instance was created");
      var source = window.__eventSources[0];
      events.forEach(function (event) {
        source.dispatch("state", { data: JSON.stringify(event) });
      });
    };
    window.__seedEspPing = function (payload) {
      if (!window.__eventSources.length)
        throw new Error("No EventSource instance was created");
      window.__eventSources[0].dispatch("ping", {
        data: typeof payload === "string" ? payload : JSON.stringify(payload),
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
    { id: "text-button_order", state: "1,2,3w,4,5,6" },
    { id: "text-button_on_color", state: "0073FF" },
    { id: "text-button_off_color", state: "CECECE" },
    { id: "text-sensor_card_color", state: "DEDEDE" },
    { id: "switch-screen__clock_bar", state: "ON", value: true },
    { id: "switch-screen__clock_bar_time", state: "ON", value: true },
    { id: "switch-screen__network_status_icon", state: "ON", value: true },
    { id: "switch-indoor_temp_enable", state: "ON", value: true },
    { id: "switch-outdoor_temp_enable", state: "ON", value: true },
    { id: "text-indoor_temp_entity", state: "sensor.indoor_temperature" },
    { id: "text-outdoor_temp_entity", state: "sensor.outdoor_temperature" },
    {
      id: "select-screen__temperature_unit",
      state: "Auto",
      value: "Auto",
      option: ["Auto", "°C", "°F"],
    },
    {
      id: "switch-screen__temperature_degree_symbol",
      state: "ON",
      value: true,
    },
    {
      id: "select-screen__timezone",
      state: "Europe/London (GMT+0)",
      value: "Europe/London (GMT+0)",
      option: ["Europe/London (GMT+0)", "America/New_York (GMT-5)"],
    },
    { id: "select-screen__language", state: "en", value: "en", option: ["en"] },
    {
      id: "select-home_assistant_artwork_protocol",
      state: "http",
      value: "http",
      option: ["http", "https"],
    },
    { id: "switch-firmware__auto_update", state: "ON", value: true },
    { id: "text_sensor-firmware__version", state: "v1.12.0" },
    {
      id: "update-firmware__update",
      state: "UPDATE AVAILABLE",
      current_version: "v1.12.0",
      latest_version: "v1.13.0",
    },
    {
      id: "select-firmware__update_frequency",
      state: "Daily",
      value: "Daily",
      option: ["Hourly", "Daily", "Weekly", "Monthly"],
    },
    {
      id: "select-screen__clock_format",
      state: "24h",
      value: "24h",
      option: ["12h", "24h"],
    },
    {
      id: "select-screen__rotation",
      state: "0",
      value: "0",
      option: ["0", "90", "180", "270"],
    },
    {
      id: "number-screensaver_timeout",
      state: "300",
      value: 300,
      min: 10,
      max: 3600,
    },
    {
      id: "text-subpage_6_config",
      state: "1,B|media_player.living:Living:Speaker:Auto:play_pause::media",
    },
  ];
  BUTTON_FIXTURES.forEach((state, index) => {
    events.push({ id: `text-button_${index + 1}_config`, state });
  });
  return events;
}

function rotationStartupBaseEvents(includeRotation = true, fixtureCount = BUTTON_FIXTURES.length) {
  const events = [
    { id: "text-button_on_color", state: "0073FF" },
    { id: "text-button_off_color", state: "CECECE" },
    { id: "text-sensor_card_color", state: "DEDEDE" },
  ];
  if (includeRotation) {
    events.push({
      id: "select-screen__rotation",
      state: "90",
      value: "90",
      option: ["0", "90", "180", "270"],
    });
  }
  BUTTON_FIXTURES.slice(0, fixtureCount).forEach((state, index) => {
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
  assert(
    result.visibleGridChildren > 0,
    `${label}: grid cells should have visible size`,
  );
  assert(
    result.visibleCards >= minVisibleCards,
    `${label}: seeded cards should render`,
  );
  assert.strictEqual(
    result.outsideGrid.length,
    0,
    `${label}: grid children overflowed the preview: ${result.outsideGrid.join(", ")}`,
  );
  assert.strictEqual(
    result.overlaps.length,
    0,
    `${label}: grid children overlapped: ${result.overlaps.join(", ")}`,
  );
  assert(
    result.documentScrollWidth <= result.documentClientWidth + 1,
    `${label}: page has horizontal overflow (${result.documentScrollWidth}px > ${result.documentClientWidth}px)`,
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
    var children = Array.from(document.querySelectorAll(".sp-main > *")).map(
      function (el, index) {
        return { index: index, className: el.className, rect: rectFor(el) };
      },
    );
    var visibleChildren = children.filter(function (child) {
      return visible(child.rect);
    });
    var outsideGrid = visibleChildren
      .filter(function (child) {
        var r = child.rect;
        return (
          r.left < mainRect.left - 1 ||
          r.top < mainRect.top - 1 ||
          r.right > mainRect.right + 1 ||
          r.bottom > mainRect.bottom + 1
        );
      })
      .map(function (child) {
        return String(child.index);
      });
    var overlaps = [];
    for (var i = 0; i < visibleChildren.length; i++) {
      for (var j = i + 1; j < visibleChildren.length; j++) {
        if (overlap(visibleChildren[i].rect, visibleChildren[j].rect)) {
          overlaps.push(
            visibleChildren[i].index + "/" + visibleChildren[j].index,
          );
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
      documentClientWidth: document.documentElement.clientWidth,
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

function assertPortraitGridLayout(result, label, options = {}) {
  assert(
    !result.loading,
    `${label}: grid should no longer be waiting for startup rotation`,
  );
  assert.strictEqual(
    result.busy,
    null,
    `${label}: grid should not remain aria-busy`,
  );
  assert.strictEqual(
    result.gridHidden,
    false,
    `${label}: grid should be visible after startup rotation is known`,
  );
  assert(
    result.visibleCards >= (options.minVisibleCards || BUTTON_FIXTURES.length),
    `${label}: saved cards should render`,
  );
  assert(
    result.screenHeight > result.screenWidth,
    `${label}: preview should end in portrait orientation`,
  );
  assert.strictEqual(
    gridTrackCount(result.gridTemplateColumns),
    3,
    `${label}: portrait grid should use 3 columns`,
  );
  assert.strictEqual(
    gridTrackCount(result.gridTemplateRows),
    5,
    `${label}: portrait grid should use 5 rows`,
  );
}

async function assertPageTitleEvents(browser) {
  const slug = "guition-esp32-p4-jc1060p470";
  const context = await browser.newContext({
    viewport: { width: 1100, height: 1000 },
  });
  await installRoutes(context, slug);
  const page = await context.newPage();
  await installFakeEventSource(page);
  try {
    await page.goto(`http://espcontrol.test/${slug}?events=1`, {
      waitUntil: "domcontentloaded",
    });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );

    assert.strictEqual(
      await page.title(),
      "EspControl",
      "page should start with EspControl fallback title",
    );

    await page.evaluate(() =>
      window.__seedEspPing({ title: "EspControl 7inch P4" }),
    );
    assert.strictEqual(
      await page.title(),
      "EspControl 7inch P4",
      "ping title should set browser title",
    );

    await page.evaluate(() => window.__seedEspPing({ uptime: 12 }));
    assert.strictEqual(
      await page.title(),
      "EspControl 7inch P4",
      "keepalive ping should preserve browser title",
    );

    await page.evaluate(() => window.__seedEspPing({ title: "   " }));
    assert.strictEqual(
      await page.title(),
      "EspControl",
      "blank ping title should restore fallback title",
    );

    await page.evaluate(() =>
      window.__seedEspPing({ title: "EspControl 7inch P4" }),
    );
    await page.evaluate(() => window.__seedEspPing({}));
    assert.strictEqual(
      await page.title(),
      "EspControl 7inch P4",
      "missing ping title should preserve current title",
    );

    await page.evaluate(() => window.__seedEspPing("not-json"));
    assert.strictEqual(
      await page.title(),
      "EspControl",
      "malformed ping payload should not break title fallback",
    );
  } finally {
    await context.close();
  }

  const normalContext = await browser.newContext({
    viewport: { width: 1100, height: 1000 },
  });
  await installRoutes(normalContext, slug);
  const normalPage = await normalContext.newPage();
  await installFakeEventSource(normalPage);
  try {
    await normalPage.goto(`http://espcontrol.test/${slug}`, {
      waitUntil: "domcontentloaded",
    });
    await normalPage.waitForSelector("#sp-app");
    await normalPage.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );
    await normalPage.evaluate(() =>
      window.__seedEspPing({ title: "EspControl 4inch S3" }),
    );
    assert.strictEqual(
      await normalPage.title(),
      "EspControl 4inch S3",
      "normal page should read title from one-shot event stream",
    );
    assert.strictEqual(
      await normalPage.evaluate(() => window.__eventSources[0].readyState),
      2,
      "normal page title probe should close event stream after title ping",
    );
  } finally {
    await normalContext.close();
  }
}

async function assertRotationStartupOrdering(browser) {
  const slug = "guition-esp32-p4-jc1060p470";
  const context = await browser.newContext({
    viewport: { width: 1100, height: 1000 },
  });
  await installRoutes(context, slug);
  const page = await context.newPage();
  await installFakeEventSource(page);
  try {
    await page.goto(`http://espcontrol.test/${slug}?events=1`, {
      waitUntil: "domcontentloaded",
    });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );

    await page.evaluate(
      (events) => window.__seedEspState(events),
      [{ id: "text-button_order", state: "1,2,3w,4,5" }].concat(
        rotationStartupBaseEvents(false, 5),
      ),
    );
    let layout = await measureRotationStartupLayout(page);
    assert(
      layout.loading,
      "button_order before rotation: grid should stay in startup loading state",
    );
    assert.strictEqual(
      layout.busy,
      "true",
      "button_order before rotation: grid should be marked busy",
    );
    assert(
      layout.gridHidden,
      "button_order before rotation: grid should be hidden until rotation arrives",
    );
    assert.strictEqual(
      layout.visibleCards,
      0,
      "button_order before rotation: cards should not render in the default layout first",
    );

    await page.evaluate(() =>
      window.__seedEspState([
        {
          id: "select-screen__rotation",
          state: "90",
          value: "90",
          option: ["0", "90", "180", "270"],
        },
      ]),
    );
    await page.waitForSelector(".sp-main > .sp-btn");
    layout = await measureRotationStartupLayout(page);
    assertPortraitGridLayout(layout, "button_order before rotation", {
      minVisibleCards: 5,
    });
  } finally {
    await context.close();
  }

  const reverseContext = await browser.newContext({
    viewport: { width: 1100, height: 1000 },
  });
  await installRoutes(reverseContext, slug);
  const reversePage = await reverseContext.newPage();
  await installFakeEventSource(reversePage);
  try {
    await reversePage.goto(`http://espcontrol.test/${slug}?events=1`, {
      waitUntil: "domcontentloaded",
    });
    await reversePage.waitForSelector("#sp-app");
    await reversePage.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );
    await reversePage.evaluate(
      (events) => window.__seedEspState(events),
      rotationStartupBaseEvents(true, 5).concat([
        { id: "text-button_order", state: "1,2,3w,4,5" },
      ]),
    );
    await reversePage.waitForSelector(".sp-main > .sp-btn");
    assertPortraitGridLayout(
      await measureRotationStartupLayout(reversePage),
      "rotation before button_order",
      { minVisibleCards: 5 },
    );
  } finally {
    await reverseContext.close();
  }

  const fallbackContext = await browser.newContext({
    viewport: { width: 1100, height: 1000 },
  });
  await installRoutes(fallbackContext, slug);
  const fallbackPage = await fallbackContext.newPage();
  await installFakeEventSource(fallbackPage);
  try {
    await fallbackPage.goto(`http://espcontrol.test/${slug}?events=1`, {
      waitUntil: "domcontentloaded",
    });
    await fallbackPage.waitForSelector("#sp-app");
    await fallbackPage.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );
    await fallbackPage.evaluate(
      (events) => window.__seedEspState(events),
      [{ id: "text-button_order", state: "1,2,3w,4,5" }].concat(
        rotationStartupBaseEvents(false, 5),
      ),
    );
    let layout = await measureRotationStartupLayout(fallbackPage);
    assert(
      layout.loading,
      "rotation fallback: grid should wait briefly when rotation support exists but no value has arrived",
    );
    await fallbackPage.waitForFunction(
      () => {
        var main = document.querySelector(".sp-main");
        return (
          main &&
          !main.classList.contains("sp-grid-loading") &&
          document.querySelectorAll(".sp-main > .sp-btn").length >= 4
        );
      },
      null,
      { timeout: 3000 },
    );
    layout = await measureRotationStartupLayout(fallbackPage);
    assert(
      !layout.loading,
      "rotation fallback: grid should appear after fallback timeout",
    );
    assert.strictEqual(
      layout.gridHidden,
      false,
      "rotation fallback: grid should be visible after fallback timeout",
    );
    assert(
      layout.visibleCards >= 5,
      "rotation fallback: saved cards should render after fallback timeout",
    );
  } finally {
    await fallbackContext.close();
  }
}

async function assertSettingsPage(page, label, options = {}) {
  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const settingsVisible = await page.locator("#sp-settings").isVisible();
  const appearanceVisible = await page
    .locator("text=Appearance")
    .first()
    .isVisible();
  if (!(await page.locator("#sp-set-on-color").isVisible())) {
    await page.getByText("Appearance", { exact: true }).click();
  }
  const onColorVisible = await page.locator("#sp-set-on-color").isVisible();
  assert(settingsVisible, `${label}: settings page should be visible`);
  assert(appearanceVisible, `${label}: settings content should render`);
  assert.strictEqual(
    onColorVisible,
    true,
    `${label}: color controls should be visible`,
  );
  assert.deepStrictEqual(
    await page
      .locator("#sp-settings .sp-settings-status-title")
      .evaluateAll((nodes) => nodes.map((node) => node.textContent)),
    ["Display", "Sleep & Schedule", "Preferences", "System"],
    `${label}: settings groups should be ordered by purpose`,
  );
  const firmwareCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Firmware$/ }),
    })
    .first();
  assert(
    await firmwareCard.isVisible(),
    `${label}: firmware settings card should render`,
  );
  if (options.exerciseInteractions) {
    await firmwareCard.locator(":scope > .card-header").click();
    await page.waitForSelector("#sp-fw-updates-panel", { state: "visible" });
    assert.deepStrictEqual(
      await firmwareCard
        .locator(".sp-fw-subpanels > .sp-disclosure")
        .evaluateAll((nodes) => nodes.map((node) => node.id)),
      ["sp-fw-updates-panel", "sp-fw-auto-panel", "sp-fw-wifi-panel", "sp-fw-previous-panel"],
      `${label}: firmware sub-panels should use the requested order`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-updates-panel .sp-disclosure-button > span").first().innerText(),
      "Firmware updates",
      `${label}: firmware update details should have a clear panel title`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-updates-panel .sp-disclosure-button").getAttribute("aria-expanded"),
      "false",
      `${label}: firmware updates should start closed`,
    );
    assert(
      await page.locator("#sp-fw-updates-panel .sp-disclosure-badge").isVisible(),
      `${label}: available firmware should show an update badge while closed`,
    );
    await page.locator("#sp-fw-updates-panel .sp-disclosure-button").click();
    assert(
      await page.locator("#sp-fw-updates-panel .sp-fw-overview").isVisible(),
      `${label}: firmware update details should render inside the panel`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-updates-panel .sp-disclosure-badge").isVisible(),
      false,
      `${label}: firmware update badge should hide while open`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-auto-panel").getAttribute("class"),
      "sp-disclosure",
      `${label}: auto updates should start closed`,
    );
    assert(
      await page.locator("#sp-fw-auto-panel .sp-disclosure-badge").isVisible(),
      `${label}: enabled auto updates should show an On badge while closed`,
    );
    await page.locator("#sp-fw-auto-panel .sp-disclosure-button").click();
    assert.strictEqual(
      await page.locator("#sp-fw-auto-panel .sp-disclosure-button").getAttribute("aria-expanded"),
      "true",
      `${label}: auto updates should expose its expanded state`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-auto-panel .sp-disclosure-badge").isVisible(),
      false,
      `${label}: auto-update badge should hide while open`,
    );
    assert(
      await page.locator("#sp-set-update-freq").isVisible(),
      `${label}: enabled auto updates should show frequency inside the panel`,
    );
    assert.deepStrictEqual(
      await firmwareCard
        .locator(
          ".sp-fw-label, .sp-fw-version, .sp-disclosure-button, .sp-toggle-label, .sp-select, .sp-fw-btn",
        )
        .evaluateAll((nodes) => [
          ...new Set(nodes.map((node) => getComputedStyle(node).fontSize)),
        ]),
      ["14px"],
      `${label}: firmware labels, values, headings, fields, and actions should use one primary font size`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-previous-panel .sp-disclosure-button").getAttribute("aria-expanded"),
      "false",
      `${label}: previous firmware should start closed`,
    );
    await page.locator("#sp-fw-previous-panel .sp-disclosure-button").click();
    assert.deepStrictEqual(
      await page.locator("#sp-set-firmware-version option").evaluateAll(
        (options) => options.map((option) => option.value),
      ),
      ["v1.11.0"],
      `${label}: previous firmware should exclude latest and installed versions`,
    );
    assert.strictEqual(
      await page.locator("#sp-fw-previous-panel .sp-fw-btn").isEnabled(),
      true,
      `${label}: a previous firmware selection should enable Install`,
    );
    const confirmPromise = page.waitForEvent("dialog");
    const installClick = page.locator("#sp-fw-previous-panel .sp-fw-btn").click();
    const confirmDialog = await confirmPromise;
    assert.strictEqual(
      confirmDialog.message(),
      "Install older firmware v1.11.0? The display will restart during installation.",
      `${label}: previous firmware installation should require confirmation`,
    );
    await confirmDialog.dismiss();
    await installClick;
    assert.strictEqual(
      await page
        .locator("#sp-settings .card-header h3")
        .filter({ hasText: /^WiFi$/ })
        .count(),
      0,
      `${label}: WiFi firmware should not remain a standalone settings card`,
    );
    await page.evaluate(() => window.__seedEspState([
      { id: "text_sensor-esp32_c6__current_firmware", state: "2.12.8" },
      { id: "text_sensor-esp32_c6__latest_firmware", state: "2.12.9" },
      { id: "switch-wifi_firmware__auto_update", state: "ON", value: true },
      { id: "button-firmware_esp32_c6__install_update", state: "" },
    ]));
    const wifiPanel = page.locator("#sp-fw-wifi-panel");
    assert(await wifiPanel.isVisible(), `${label}: supported WiFi firmware panel should render`);
    assert(
      await wifiPanel.locator(".sp-disclosure-badge").isVisible(),
      `${label}: WiFi update badge should show while the closed panel has an update`,
    );
    await wifiPanel.locator(".sp-disclosure-button").click();
    assert.strictEqual(
      await wifiPanel.locator(".sp-disclosure-badge").isVisible(),
      false,
      `${label}: WiFi update badge should hide while open`,
    );
    assert.deepStrictEqual(
      await wifiPanel.locator(".sp-fw-version").evaluateAll(
        (nodes) => nodes.map((node) => node.textContent),
      ),
      ["2.12.8", "2.12.9"],
      `${label}: WiFi panel should show current and available versions`,
    );
    assert.strictEqual(
      await wifiPanel.locator("#sp-set-c6-auto-update").isChecked(),
      true,
      `${label}: WiFi automatic updates should be enabled by default`,
    );
    assert.strictEqual(
      await firmwareCard
        .locator(".sp-fw-overview .sp-fw-actions")
        .evaluate((node) => getComputedStyle(node).justifyContent),
      "flex-end",
      `${label}: firmware actions should align with the version values`,
    );
    await page.evaluate(() => window.__seedEspState([{
      id: "update-firmware__update",
      state: "INSTALLING",
      current_version: "v1.12.0",
      latest_version: "v1.13.0",
    }]));
    assert.strictEqual(
      await firmwareCard.locator(".sp-fw-overview .sp-fw-btn").innerText(),
      "Installing…",
      `${label}: install progress should stay in the action button`,
    );
    assert.strictEqual(
      await firmwareCard.locator(".sp-fw-overview .sp-fw-status").innerText(),
      "",
      `${label}: install progress should not be duplicated below the action`,
    );
  }
  const clockBarCard = page
    .locator("#sp-settings .card")
    .filter({ hasText: "Clock Bar" })
    .first();
  const clockBarText = await clockBarCard.textContent();
  const voiceServicesCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Voice Services$/ }),
    })
    .first();
  const alarmDelayAudioCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Alarm Delay Audio$/ }),
    })
    .first();
  if (options.slug === "esp32-p4-86") {
    assert(
      await voiceServicesCard.isVisible(),
      `${label}: voice services settings card is available for the voice-capable panel`,
    );
    assert(
      await alarmDelayAudioCard.isVisible(),
      `${label}: alarm delay audio settings are available for the speaker panel`,
    );
    await alarmDelayAudioCard.locator(".card-header").click();
    const alarmDelayAudioToggle = alarmDelayAudioCard.locator("#sp-set-alarm-delay-audio");
    async function setAlarmDelayAudioEnabled(enabled) {
      await alarmDelayAudioToggle.evaluate((input, checked) => {
        input.checked = checked;
        input.dispatchEvent(new Event("change", { bubbles: true }));
      }, enabled);
    }
    await setAlarmDelayAudioEnabled(true);
    const entryAnnouncement = alarmDelayAudioCard.locator("#sp-set-alarm-delay-entry-announcement");
    const exitAnnouncement = alarmDelayAudioCard.locator("#sp-set-alarm-delay-exit-announcement");
    async function changeAnnouncement(input, value) {
      await input.evaluate((element, nextValue) => {
        element.value = nextValue;
        element.dispatchEvent(new Event("change", { bubbles: true }));
      }, value);
    }
    await changeAnnouncement(entryAnnouncement, "Updated entry announcement");
    await changeAnnouncement(exitAnnouncement, "Updated exit announcement");
    await setAlarmDelayAudioEnabled(false);
    await setAlarmDelayAudioEnabled(true);
    assert.strictEqual(
      await entryAnnouncement.inputValue(),
      "Updated entry announcement",
      `${label}: entry announcement state survives settings UI synchronization`,
    );
    assert.strictEqual(
      await exitAnnouncement.inputValue(),
      "Updated exit announcement",
      `${label}: exit announcement state survives settings UI synchronization`,
    );
  } else {
    assert(
      !clockBarText.includes("Voice Services"),
      `${label}: voice services toggle is hidden from the clock bar`,
    );
    assert.strictEqual(
      await voiceServicesCard.count(),
      0,
      `${label}: voice services settings card is hidden on panels without local voice`,
    );
    assert.strictEqual(
      await alarmDelayAudioCard.count(),
      0,
      `${label}: alarm delay audio settings are hidden on panels without speaker support`,
    );
  }
  const nightScheduleCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Night Schedule$/ }),
    })
    .first();
  assert(
    await nightScheduleCard.isVisible(),
    `${label}: night schedule settings card should render`,
  );
  await nightScheduleCard.locator(".card-header").click();
  const nightScheduleInfo = page.locator("#sp-night-schedule-info");
  assert(
    await nightScheduleInfo.isVisible(),
    `${label}: night schedule override info panel should render`,
  );
  assert.strictEqual(
    await nightScheduleInfo.innerText(),
    "Time-based Night Schedule overrides screensaver presence wake and Media Cover Art while it is active.",
    `${label}: night schedule override info panel text should match`,
  );
  assert.strictEqual(
    await nightScheduleCard.locator("#sp-set-schedule-on-hour").isVisible(),
    false,
    `${label}: disabled night schedule should hide time fields`,
  );
  assert.strictEqual(
    await nightScheduleCard.locator("#sp-set-schedule-presence").isVisible(),
    false,
    `${label}: disabled night schedule should hide the sensor field`,
  );
  assert.strictEqual(
    await nightScheduleCard.locator("#sp-set-schedule-actions").isVisible(),
    false,
    `${label}: disabled night schedule should hide night action controls`,
  );
  const coverArtCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Cover Art$/ }),
    })
    .first();
  assert(
    await coverArtCard.isVisible(),
    `${label}: cover art settings card should render`,
  );
  await coverArtCard.locator(".card-header").click();
  assert.strictEqual(
    await page.locator("#sp-cover-art-info").count(),
    0,
    `${label}: media cover art override info panel should not render`,
  );
  assert.strictEqual(
    await page.locator("#sp-set-ss-media-sleep-prevention").count(),
    1,
    `${label}: keep-screen-awake option should exist in cover art settings`,
  );
  assert.strictEqual(
    await coverArtCard
      .getByText("Keep Screen Awake During Playback", { exact: true })
      .isVisible(),
    false,
    `${label}: keep-screen-awake option should hide when cover art is disabled`,
  );
  assert.strictEqual(
    await coverArtCard.locator("#sp-set-ss-cover-art-player").isVisible(),
    false,
    `${label}: media player entity field should hide when cover art is disabled`,
  );
  assert.strictEqual(
    await coverArtCard.locator("#sp-set-ss-cover-art-delay").isVisible(),
    false,
    `${label}: cover art show-after field should stay hidden until cover art is enabled`,
  );
  await coverArtCard
    .locator("#sp-set-ss-cover-art-enable + .sp-toggle-track")
    .click();
  assert.strictEqual(
    await coverArtCard
      .getByText("Keep Screen Awake During Playback", { exact: true })
      .isVisible(),
    false,
    `${label}: keep-screen-awake option should remain inside collapsed advanced options`,
  );
  assert(
    await coverArtCard.locator("#sp-set-ss-cover-art-player").isVisible(),
    `${label}: media player entity field should render when cover art is enabled`,
  );
  assert(
    await coverArtCard.locator("#sp-set-ss-cover-art-delay").isVisible(),
    `${label}: cover art show-after field should render when cover art is enabled`,
  );
  assert.deepStrictEqual(
    await coverArtCard.locator("#sp-set-ss-cover-art-delay option").evaluateAll(
      (options) => options.map((option) => option.value),
    ),
    ["3", "5", "10", "30", "60", "300"],
    `${label}: cover art show-after options should start at three seconds`,
  );
  assert.strictEqual(
    await page.locator("#sp-set-ss-track-overlay").count(),
    options.coverArtSquareOverlay ? 1 : 0,
    `${label}: track overlay duration visibility should match square cover art layout`,
  );
  assert(
    await coverArtCard
      .getByText("Advanced Options", { exact: true })
      .isVisible(),
    `${label}: media cover art advanced options should render`,
  );
  assert.strictEqual(
    await coverArtCard.locator("#sp-set-ss-cover-art-conditions").isVisible(),
    false,
    `${label}: cover art conditions field should be hidden until advanced filtering is enabled`,
  );
  await coverArtCard.getByText("Advanced Options", { exact: true }).click();
  assert(
    await coverArtCard
      .getByText("Keep Screen Awake During Playback", { exact: true })
      .isVisible(),
    `${label}: keep-screen-awake option should render inside advanced options`,
  );
  assert(
    await coverArtCard
      .getByText("Hide for external source inputs", { exact: true })
      .isVisible(),
    `${label}: external source input option should render inside advanced options`,
  );
  assert(
    await coverArtCard
      .getByText("Advanced Filtering", { exact: true })
      .isVisible(),
    `${label}: advanced filtering toggle should render inside advanced options`,
  );
  await coverArtCard
    .locator("#sp-set-ss-cover-art-filtering + .sp-toggle-track")
    .click();
  assert(
    await coverArtCard.locator("#sp-set-ss-cover-art-conditions").isVisible(),
    `${label}: cover art conditions field should render after advanced filtering is enabled`,
  );
  if (
    !(await coverArtCard
      .locator("#sp-set-ss-media-sleep-prevention")
      .isChecked())
  ) {
    await coverArtCard
      .locator("#sp-set-ss-media-sleep-prevention + .sp-toggle-track")
      .click();
  }
  await coverArtCard
    .locator("#sp-set-ss-cover-art-enable + .sp-toggle-track")
    .click();
  assert.strictEqual(
    await coverArtCard
      .getByText("Keep Screen Awake During Playback", { exact: true })
      .isVisible(),
    false,
    `${label}: enabled keep-screen-awake option should hide when cover art is disabled`,
  );
  assert.strictEqual(
    await coverArtCard.locator("#sp-set-ss-cover-art-player").isVisible(),
    false,
    `${label}: media player entity field should hide when cover art is disabled even if keep-screen-awake is enabled`,
  );
  assert(
    (await page.locator("#sp-set-ss-cover-art-server").count()) === 0,
    `${label}: cover art fallback URL field should not render`,
  );
  assert(
    (await page.locator("#sp-set-ss-media-player").count()) === 0,
    `${label}: timer media player field should not render`,
  );
  assert.strictEqual(
    await page.locator("#sp-set-sensor-media-player-enable").count(),
    0,
    `${label}: sensor cover art override should not render`,
  );
  const homeAssistantSettingsCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", {
        hasText: /^Home Assistant Settings$/,
      }),
    })
    .first();
  assert(
    await homeAssistantSettingsCard.isVisible(),
    `${label}: Home Assistant settings card should render`,
  );
  assert(
    !(await homeAssistantSettingsCard
      .locator("#sp-set-ha-artwork-port")
      .isVisible()),
    `${label}: Home Assistant settings card should be collapsed by default`,
  );
  await homeAssistantSettingsCard.locator(".card-header").click();
  assert(
    await homeAssistantSettingsCard
      .locator("#sp-set-ha-artwork-port")
      .isVisible(),
    `${label}: Home Assistant port field should render in Home Assistant settings`,
  );
  assert.strictEqual(
    await homeAssistantSettingsCard
      .locator("#sp-set-ha-artwork-port")
      .inputValue(),
    "8123",
    `${label}: Home Assistant port field should default to 8123`,
  );
  assert(
    (await homeAssistantSettingsCard
      .locator("#sp-set-ha-artwork-port.sp-input--no-stepper")
      .count()) === 1,
    `${label}: Home Assistant port field should hide browser stepper controls`,
  );
  const overflow = await page.evaluate(
    () => document.documentElement.scrollWidth > window.innerWidth + 1,
  );
  assert(!overflow, `${label}: settings page has horizontal overflow`);
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  await assertVoiceClockBarPreview(page, label, options.slug === "esp32-p4-86");
}

async function assertVoiceClockBarPreview(page, label, supported) {
  const voiceItem = page.locator('[data-clockbar-item="voice"]');
  if (!supported) {
    assert.strictEqual(
      await voiceItem.count(),
      0,
      `${label}: voice services clock bar item is not rendered on panels without local voice`,
    );
    return;
  }

  assert.strictEqual(
    await voiceItem.count(),
    1,
    `${label}: voice services clock bar item is rendered on the voice-capable panel`,
  );
  await page.evaluate(() =>
    window.__seedEspState([
      { id: "switch-voice_services", state: "ON", value: true },
      { id: "switch-screen__network_status_icon", state: "OFF", value: false },
    ]),
  );
  await page.waitForFunction(() => {
    var voice = document.querySelector(
      '[data-clockbar-item="voice"] .sp-voice-preview',
    );
    return voice && voice.className.indexOf("sp-visible") !== -1;
  });

  const preview = await page.evaluate(() => {
    function box(selector) {
      var el = document.querySelector(selector);
      if (!el) return null;
      var rect = el.getBoundingClientRect();
      return {
        className: el.className,
        left: rect.left,
        right: rect.right,
        width: rect.width,
        height: rect.height,
      };
    }
    return {
      voice: box('[data-clockbar-item="voice"]'),
      network: box('[data-clockbar-item="network"]'),
      voiceIcon: box('[data-clockbar-item="voice"] .sp-voice-preview'),
      networkIcon: box('[data-clockbar-item="network"] .sp-network-preview'),
    };
  });
  assert(
    preview.voice && preview.network,
    `${label}: voice and network clock bar items are measurable`,
  );
  assert(
    preview.voice.right <= preview.network.left + 1,
    `${label}: voice mic is positioned to the left of connectivity`,
  );
  assert(
    !preview.voice.className.includes("sp-clockbar-hidden"),
    `${label}: voice item stays active when connectivity is hidden`,
  );
  assert(
    preview.network.className.includes("sp-clockbar-hidden"),
    `${label}: network item is hidden independently of voice`,
  );
  assert(
    preview.voiceIcon.className.includes("sp-visible"),
    `${label}: voice mic remains visible when connectivity is hidden`,
  );
  assert(
    preview.voiceIcon.width > 0 && preview.voiceIcon.height > 0,
    `${label}: voice mic remains measurable when connectivity is hidden`,
  );
  assert(
    preview.networkIcon.className.includes("sp-visible"),
    `${label}: hidden connectivity keeps its placeholder icon visible`,
  );

  await page.evaluate(() =>
    window.__seedEspState([
      { id: "switch-voice_services", state: "OFF", value: false },
      { id: "switch-screen__network_status_icon", state: "ON", value: true },
    ]),
  );
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
      viewportMeta:
        document.querySelector('meta[name="viewport"]') &&
        document.querySelector('meta[name="viewport"]').getAttribute("content"),
      supportVisible:
        !!support &&
        supportStyle.display !== "none" &&
        support.getBoundingClientRect().width > 1,
      screenWidth: screen.width,
      documentClientWidth: document.documentElement.clientWidth,
      documentScrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
  assert.strictEqual(
    mobile.tab,
    "screen",
    `${label}: screen tab is marked active on mobile`,
  );
  assert.strictEqual(
    mobile.viewportMeta,
    "width=device-width,initial-scale=1",
    `${label}: web app should provide mobile viewport metadata`,
  );
  assert(
    mobile.supportVisible,
    `${label}: support button remains visible on the screen tab`,
  );
  assert(
    mobile.screenWidth <= mobile.windowWidth + 1,
    `${label}: mobile screen preview fits viewport`,
  );
  assert(
    mobile.documentScrollWidth <= mobile.documentClientWidth + 1,
    `${label}: mobile screen tab has horizontal overflow`,
  );
  await page.locator('.sp-main [data-slot="1"]').click();
  await page.waitForSelector(".sp-selection-bar.sp-visible");
  mobile = await page.evaluate(() => {
    var header = document.querySelector(".sp-header");
    var bar = document.querySelector(".sp-selection-bar");
    var label = document.querySelector(".sp-selection-label");
    var actions = document.querySelector(".sp-selection-actions");
    var headerRect = header ? header.getBoundingClientRect() : null;
    var barRect = bar ? bar.getBoundingClientRect() : null;
    var labelRect = label ? label.getBoundingClientRect() : null;
    var actionsRect = actions ? actions.getBoundingClientRect() : null;
    return {
      selectionBarVisible: !!barRect && barRect.width > 1 && barRect.height > 1,
      selectionBarMatchesHeader:
        !!headerRect &&
        !!barRect &&
        Math.abs(barRect.left - headerRect.left) <= 1 &&
        Math.abs(barRect.right - headerRect.right) <= 1,
      selectionBarWithinViewport:
        !!barRect &&
        barRect.left >= -1 &&
        barRect.right <= document.documentElement.clientWidth + 1,
      selectionLabelFits:
        !!labelRect &&
        !!actionsRect &&
        labelRect.left >= barRect.left - 1 &&
        labelRect.right <= actionsRect.left + 1,
      selectionActionsWithinViewport:
        !!actionsRect &&
        actionsRect.right <= document.documentElement.clientWidth + 1,
      documentClientWidth: document.documentElement.clientWidth,
      documentScrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
  assert(
    mobile.selectionBarVisible,
    `${label}: mobile card selection bar should be visible after selecting a card`,
  );
  assert(
    mobile.selectionBarMatchesHeader,
    `${label}: mobile card selection bar should match header width`,
  );
  assert(
    mobile.selectionBarWithinViewport,
    `${label}: mobile card selection bar should fit viewport`,
  );
  assert(
    mobile.selectionLabelFits,
    `${label}: mobile card selection label should shrink before action buttons`,
  );
  assert(
    mobile.selectionActionsWithinViewport,
    `${label}: mobile card selection actions should fit viewport`,
  );
  assert(
    mobile.documentScrollWidth <= mobile.documentClientWidth + 1,
    `${label}: mobile card selection bar has horizontal overflow`,
  );

  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const nightScheduleCard = page
    .locator("#sp-settings .card")
    .filter({ hasText: "Night Schedule" })
    .first();
  await nightScheduleCard.locator(".card-header").click();
  mobile = await page.evaluate(() => {
    var support = document.querySelector(".sp-support-btn");
    var supportStyle = support ? getComputedStyle(support) : null;
    var activeCard = Array.from(
      document.querySelectorAll("#sp-settings .card"),
    ).find(function (card) {
      return !card.classList.contains("collapsed");
    });
    var activeCardRect = activeCard ? activeCard.getBoundingClientRect() : null;
    var headers = Array.from(
      document.querySelectorAll("#sp-settings .card-header"),
    ).map(function (header) {
      var rect = header.getBoundingClientRect();
      return { width: rect.width, left: rect.left, right: rect.right };
    });
    return {
      tab: document.querySelector("#sp-app").getAttribute("data-active-tab"),
      supportVisible:
        !!support &&
        supportStyle.display !== "none" &&
        support.getBoundingClientRect().width > 1,
      activeCardVisible:
        !!activeCardRect &&
        activeCardRect.width > 1 &&
        activeCardRect.height > 1,
      activeCardWithinViewport:
        !!activeCardRect &&
        activeCardRect.left >= -1 &&
        activeCardRect.right <= window.innerWidth + 1,
      headersWithinViewport: headers.every(function (rect) {
        return rect.left >= -1 && rect.right <= window.innerWidth + 1;
      }),
      documentScrollWidth: document.documentElement.scrollWidth,
      windowWidth: window.innerWidth,
    };
  });
  assert.strictEqual(
    mobile.tab,
    "settings",
    `${label}: settings tab is marked active on mobile`,
  );
  assert.strictEqual(
    mobile.supportVisible,
    false,
    `${label}: support button should not cover mobile settings`,
  );
  assert(
    mobile.activeCardVisible,
    `${label}: expanded mobile settings card should be visible`,
  );
  assert(
    mobile.activeCardWithinViewport,
    `${label}: expanded mobile settings card should fit viewport`,
  );
  assert(
    mobile.headersWithinViewport,
    `${label}: mobile settings headers should fit viewport`,
  );
  assert(
    mobile.documentScrollWidth <= mobile.windowWidth + 1,
    `${label}: mobile settings tab has horizontal overflow`,
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
    await page.goto(`http://espcontrol.test/${testCase.slug}?events=1`, {
      waitUntil: "domcontentloaded",
    });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );
    await page.evaluate(
      (events) => window.__seedEspState(events),
      seededEvents(),
    );
    await page.waitForSelector(".sp-main > .sp-btn");
    await page.getByRole("tab", { name: "Settings" }).click();
    await page.waitForSelector("#sp-settings.sp-page.active");
    const mobile = await page.evaluate(() => {
      var support = document.querySelector(".sp-support-btn");
      var supportStyle = support ? getComputedStyle(support) : null;
      return {
        viewportMeta:
          document.querySelector('meta[name="viewport"]') &&
          document
            .querySelector('meta[name="viewport"]')
            .getAttribute("content"),
        windowWidth: window.innerWidth,
        documentScrollWidth: document.documentElement.scrollWidth,
        supportVisible:
          !!support &&
          supportStyle.display !== "none" &&
          support.getBoundingClientRect().width > 1,
      };
    });
    assert.strictEqual(
      mobile.viewportMeta,
      "width=device-width,initial-scale=1",
      `${testCase.name}: mobile browser should receive device-width viewport metadata`,
    );
    assert(
      mobile.windowWidth <= 380,
      `${testCase.name}: mobile browser should not render with a desktop layout viewport (${mobile.windowWidth}px)`,
    );
    assert(
      mobile.documentScrollWidth <= mobile.windowWidth + 1,
      `${testCase.name}: mobile device settings tab has horizontal overflow`,
    );
    assert.strictEqual(
      mobile.supportVisible,
      false,
      `${testCase.name}: support button should not cover settings on mobile devices`,
    );
  } catch (error) {
    fs.mkdirSync(FAILURE_DIR, { recursive: true });
    await page.screenshot({
      path: path.join(
        FAILURE_DIR,
        `${testCase.name}-${testCase.slug}-mobile.png`,
      ),
      fullPage: true,
    });
    throw error;
  } finally {
    await context.close();
  }
}

async function assertEmptyCellSettings(page, posts, label) {
  const emptyCell = page
    .locator(".sp-empty-cell:not(.sp-info-only-hidden)")
    .first();
  if ((await emptyCell.count()) === 0) return;
  assert.strictEqual(
    await page.locator(".sp-btn .sp-add-pill").count(),
    0,
    `${label}: configured slots do not show add controls`,
  );
  const addPill = emptyCell.locator(".sp-add-pill");
  await page.mouse.move(0, 0);
  assert.strictEqual(
    await addPill.evaluate((el) => getComputedStyle(el).opacity),
    "0",
    `${label}: empty-slot add control is hidden until hover`,
  );
  await emptyCell.hover();
  await page.waitForFunction(
    (el) => parseFloat(getComputedStyle(el).opacity) > 0.9,
    await addPill.elementHandle(),
  );
  const pos = await emptyCell.getAttribute("data-pos");
  const before = posts.length;
  await emptyCell.click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.waitForTimeout(100);
  assert.strictEqual(
    posts.length,
    before,
    `${label}: opening a new card draft should not post immediately`,
  );
  assert(
    await page.locator("#sp-card-type-picker").isVisible(),
    `${label}: new card draft shows the card type grid`,
  );
  const switchTypeOption = page.getByRole("button", {
    name: "Switch card type",
  });
  assert(
    await switchTypeOption.isVisible(),
    `${label}: new card draft shows Switch as a card type`,
  );
  assert(
    (
      await switchTypeOption.locator(".sp-card-type-icon").getAttribute("class")
    ).includes("mdi-toggle-switch"),
    `${label}: card type picker preserves pre-slugged MDI icon names`,
  );
  assert.strictEqual(
    await page.locator("#sp-inp-type").count(),
    0,
    `${label}: new card draft does not show the compact type dropdown before selection`,
  );
  assert.strictEqual(
    await page.locator(".sp-settings-modal .sp-save-btn").count(),
    0,
    `${label}: new card draft hides Save until a type is selected`,
  );
  assert.strictEqual(
    await page.locator(".sp-settings-modal .sp-delete-btn").count(),
    0,
    `${label}: new card draft hides Delete before save`,
  );
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
  assert(
    modalLayout.visible,
    `${label}: card settings modal should be visible`,
  );
  assert(
    modalLayout.left >= -1 && modalLayout.right <= modalLayout.windowWidth + 1,
    `${label}: card settings modal overflows horizontally`,
  );
  assert(
    modalLayout.top < modalLayout.windowHeight && modalLayout.bottom > 0,
    `${label}: card settings modal is outside the viewport`,
  );
  assert(
    modalLayout.documentScrollWidth <= modalLayout.windowWidth + 1,
    `${label}: card settings modal introduced horizontal overflow`,
  );
  const closeControl = await page.evaluate(() => {
    var button = document.querySelector(".sp-settings-close");
    var icon = button && button.querySelector(".sp-settings-close-icon path");
    if (!button) return { visible: false };
    var rect = button.getBoundingClientRect();
    var styles = getComputedStyle(button);
    return {
      visible: rect.width > 1 && rect.height > 1,
      hasCloseIcon: !!icon,
      usesInlineIcon: !!icon && icon.tagName.toLowerCase() === "path",
      width: rect.width,
      height: rect.height,
      radius: parseFloat(styles.borderRadius),
      borderWidth: parseFloat(styles.borderTopWidth),
    };
  });
  assert(
    closeControl.visible,
    `${label}: settings modal close control is visible`,
  );
  assert(
    closeControl.hasCloseIcon,
    `${label}: settings modal close control uses a close icon`,
  );
  assert(
    closeControl.usesInlineIcon,
    `${label}: settings modal close control icon does not depend on external icon fonts`,
  );
  assert(
    Math.abs(closeControl.width - closeControl.height) <= 1,
    `${label}: settings modal close control is circular`,
  );
  assert(
    closeControl.radius >= closeControl.width / 2 - 1,
    `${label}: settings modal close control has a circle container`,
  );
  assert(
    closeControl.borderWidth >= 1,
    `${label}: settings modal close control has a visible container border`,
  );
  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
  assert.strictEqual(
    posts.length,
    before,
    `${label}: closing a new card draft before choosing a type should not post`,
  );
  await page
    .locator(`.sp-main [data-pos="${pos}"].sp-empty-cell`)
    .waitFor({ state: "visible" });

  await page.locator(`.sp-main [data-pos="${pos}"]`).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.getByRole("button", { name: "Action card type" }).click();
  await page.locator("#sp-inp-type").waitFor({ state: "visible" });
  await page.locator("#sp-inp-label").fill("Keep this label");
  await page.locator("#sp-inp-entity").fill("switch.keep_this_entity");
  await page.locator("#sp-inp-action").selectOption({ label: "Run Script" });
  await page.locator("#sp-inp-type").selectOption({ label: "Switch" });
  await page.locator("#sp-inp-entity").waitFor({ state: "visible" });
  assert.strictEqual(
    await page.locator("#sp-inp-label").inputValue(),
    "Keep this label",
    `${label}: changing the default card type preserves the typed label`,
  );
  assert.strictEqual(
    await page.locator("#sp-inp-entity").inputValue(),
    "switch.keep_this_entity",
    `${label}: changing the default card type preserves the typed entity`,
  );
  assert.strictEqual(
    await page.locator("#sp-inp-icon").inputValue(),
    "Auto",
    `${label}: changing the default Action card type clears its icon default`,
  );
  assert.strictEqual(
    await page.locator("#sp-inp-sensor-when-on-toggle").isChecked(),
    false,
    `${label}: changing the default Action card type clears its active display default`,
  );
  assert(
    await page.locator(".sp-settings-modal .sp-save-btn").isVisible(),
    `${label}: changing the default card type keeps Save visible`,
  );
  assert.strictEqual(
    await page.locator(".sp-settings-modal .sp-delete-btn").count(),
    0,
    `${label}: unsaved new card keeps Delete hidden after type selection`,
  );
  await page.locator("#sp-inp-type").selectOption({ label: "Sensor" });
  const sensorTypeOptions = await page.locator("#sp-inp-sensor-type option").allTextContents();
  assert.deepStrictEqual(
    sensorTypeOptions,
    ["Numeric", "Time", "Text", "Icon"],
    `${label}: Home Assistant Sensor uses the Numeric, Time, Text, and Icon Type dropdown`,
  );
  const sensorActiveColor = page.locator("#sp-inp-sensor-active-color");
  const sensorActiveColorRow = sensorActiveColor.locator("xpath=../..");
  assert(
    await sensorActiveColorRow.isVisible(),
    `${label}: Numeric Sensor exposes Lit When Active`,
  );
  await sensorActiveColorRow.getByText("Lit When Active", { exact: true }).click();
  await page.locator("#sp-inp-sensor-type").selectOption("time");
  assert.strictEqual(
    await sensorActiveColorRow.isVisible(),
    false,
    `${label}: Time Sensor hides Lit When Active`,
  );
  assert(
    await page.locator("#sp-inp-time-unit").isVisible(),
    `${label}: Time type shows the input unit dropdown`,
  );
  assert.strictEqual(
    await page.locator("#sp-inp-time-unit").inputValue(),
    "",
    `${label}: Time input unit defaults to Auto`,
  );
  assert.strictEqual(
    await page.locator("#sp-inp-unit").isVisible(),
    false,
    `${label}: Time type hides the normal unit field`,
  );
  await page.locator("#sp-inp-time-unit").selectOption("hours");
  await page.locator("#sp-inp-sensor-type").selectOption("numeric");
  assert.strictEqual(
    await sensorActiveColor.isChecked(),
    false,
    `${label}: switching through Time clears Lit When Active`,
  );
  await page.locator("#sp-inp-sensor-type").selectOption("time");
  assert.strictEqual(
    await page.locator("#sp-inp-time-unit").inputValue(),
    "",
    `${label}: switching away from Time clears its manual input unit`,
  );
  await page.getByRole("button", { name: "Local Sensor", exact: true }).click();
  assert.strictEqual(
    await page.locator("#sp-inp-sensor-type").count(),
    0,
    `${label}: Local Sensor keeps its existing configuration controls`,
  );
  assert(
    await page.getByRole("button", { name: "Numeric", exact: true }).isVisible() &&
      await page.getByRole("button", { name: "Text", exact: true }).isVisible(),
    `${label}: Local Sensor retains its Numeric and Text mode buttons`,
  );
  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
  await page.waitForTimeout(100);
  assert.strictEqual(
    posts.length,
    before,
    `${label}: closing a typed new card draft before Save should not post`,
  );
  await page
    .locator(`.sp-main [data-pos="${pos}"].sp-empty-cell`)
    .waitFor({ state: "visible" });

  await page.locator(`.sp-main [data-pos="${pos}"]`).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.getByRole("button", { name: "Switch card type" }).click();
  await page.locator("#sp-inp-label").fill("New Card");
  await page.locator("#sp-inp-entity").fill("switch.new_card");
  await page.getByRole("button", { name: "Save" }).click();
  await page
    .locator(`.sp-main [data-pos="${pos}"][data-slot]`)
    .waitFor({ state: "visible" });
  const slot = await page
    .locator(`.sp-main [data-pos="${pos}"]`)
    .getAttribute("data-slot");
  await waitForPost(
    posts,
    { domain: "text", name: "button_order", action: "set" },
    `${label}: saving new card posts button order`,
    before,
  );
  await waitForAnyPost(
    posts,
    [
      { domain: "text", name: `button_${slot}_config`, action: "set" },
      { domain: "text", name: `Button ${slot} Config`, action: "set" },
    ],
    `${label}: saving new card posts card config`,
    before,
  );
}

async function assertCoverSettingsPanels(page, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  await page.locator('.sp-main [data-slot="5"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");

  const cardSettings = page.locator(".sp-settings-modal .sp-disclosure").filter({ hasText: "Card Settings" }).first();
  const modalSettings = page.locator(".sp-settings-modal .sp-disclosure").filter({ hasText: "Modal Settings" }).first();
  assert(await cardSettings.isVisible(), `${label}: cover card settings panel should render`);
  assert(await modalSettings.isVisible(), `${label}: cover modal settings panel should render`);
  assert(!(await cardSettings.getAttribute("class")).includes("sp-open"), `${label}: cover card settings panel should start collapsed`);
  assert(!(await modalSettings.getAttribute("class")).includes("sp-open"), `${label}: cover modal settings panel should start collapsed`);
  assert.strictEqual(
    await page.locator("#sp-inp-cover-interaction").evaluate((el) => !!el.closest(".sp-disclosure")),
    false,
    `${label}: cover type selector should sit outside collapsible panels`
  );
  assert.strictEqual(
    await page.locator("#sp-inp-entity").evaluate((el) => !!el.closest(".sp-disclosure")),
    false,
    `${label}: cover entity field should sit outside collapsible panels`
  );

  await modalSettings.locator(".sp-disclosure-button").click();
  assert(await modalSettings.getByText("Controls", { exact: true }).isVisible(), `${label}: cover modal settings panel should contain modal tab controls`);
  assert.strictEqual(
    await modalSettings.getByText("Modal Tabs", { exact: true }).count(),
    0,
    `${label}: cover modal settings panel should not show a Modal Tabs heading`
  );
  assert(
    await modalSettings.locator(".sp-light-tab-move").count() > 0,
    `${label}: cover modal settings panel should include non-drag move controls`
  );
  await page.locator("#sp-inp-cover-interaction").selectOption("toggle");
  await page.waitForFunction(() => {
    var panels = Array.from(document.querySelectorAll(".sp-settings-modal .sp-disclosure"));
    var panel = panels.find(function (item) {
      return item.textContent && item.textContent.indexOf("Modal Settings") !== -1;
    });
    return panel && getComputedStyle(panel).display === "none";
  });

  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
}

async function assertMediaCoverArtSettingsPanels(page, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  await page.locator('.sp-main [data-slot="4"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.locator("#sp-inp-type").selectOption("media_cover_art");

  const cardSettings = page.locator(".sp-settings-modal .sp-disclosure").filter({
    has: page.locator("#sp-inp-media-cover-art-card-settings"),
  });
  const secondaryPlayer = page.locator(".sp-settings-modal .sp-disclosure").filter({
    has: page.locator("#sp-inp-media-cover-art-secondary-player"),
  });
  assert(await cardSettings.isVisible(), `${label}: Cover Art card settings panel should render`);
  assert(await secondaryPlayer.isVisible(), `${label}: Cover Art secondary media player panel should render`);
  assert(!(await cardSettings.getAttribute("class")).includes("sp-open"), `${label}: Cover Art card settings should start collapsed`);
  assert(!(await secondaryPlayer.getAttribute("class")).includes("sp-open"), `${label}: Cover Art secondary media player should start collapsed`);
  assert.strictEqual(
    await page.locator("#sp-inp-entity").evaluate((el) => !!el.closest(".sp-disclosure")),
    false,
    `${label}: Cover Art primary entity should remain outside collapsible panels`,
  );
  assert.strictEqual(
    await cardSettings.locator(".sp-field").filter({ hasText: "Press Action" }).count(),
    1,
    `${label}: Cover Art Press Action should be inside Card Settings`,
  );
  assert.strictEqual(
    await cardSettings.locator("#sp-inp-media-cover-art-details").count(),
    1,
    `${label}: Cover Art track details toggle should be inside Card Settings`,
  );
  assert.strictEqual(
    await secondaryPlayer.locator("#sp-inp-media-cover-art-secondary-entity").count(),
    1,
    `${label}: Cover Art secondary entity should be inside Secondary Media Player`,
  );

  await cardSettings.locator("> .sp-disclosure-button").click();
  assert(
    await cardSettings.getByText("Press Action", { exact: true }).isVisible(),
    `${label}: Cover Art Card Settings should reveal Press Action`,
  );
  assert(
    await cardSettings.getByText("Show Track Details", { exact: true }).isVisible(),
    `${label}: Cover Art Card Settings should reveal Show Track Details`,
  );
  await secondaryPlayer.locator("> .sp-disclosure-button").click();
  const info = secondaryPlayer.locator("#sp-inp-media-cover-art-secondary-player-info");
  assert(await info.isVisible(), `${label}: Cover Art secondary media player explanation should be visible`);
  assert.strictEqual(await info.getAttribute("role"), "note", `${label}: Cover Art secondary explanation should be announced as a note`);
  assert(
    (await info.textContent()).includes("Line In, TV, or HDMI"),
    `${label}: Cover Art secondary explanation should describe external sources`,
  );
  assert(
    await secondaryPlayer.getByText("External Source Media Entity", { exact: true }).isVisible(),
    `${label}: Cover Art secondary entity picker should be visible`,
  );

  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
}

async function assertAlarmSettingsPanels(page, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  await page.locator('.sp-main [data-slot="6"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");

  const cardSettings = page.locator(".sp-settings-modal .sp-disclosure").filter({ hasText: "Card Settings" }).first();
  const modalSettings = page.locator(".sp-settings-modal .sp-disclosure").filter({ hasText: "Modal Settings" }).first();
  assert(await cardSettings.isVisible(), `${label}: alarm card settings panel should render`);
  assert(await modalSettings.isVisible(), `${label}: alarm modal settings panel should render`);
  assert(!(await cardSettings.getAttribute("class")).includes("sp-open"), `${label}: alarm card settings panel should start collapsed`);
  assert(!(await modalSettings.getAttribute("class")).includes("sp-open"), `${label}: alarm modal settings panel should start collapsed`);
  assert.strictEqual(
    await page.locator("#sp-inp-alarm-card-type").evaluate((el) => !!el.closest(".sp-disclosure")),
    false,
    `${label}: alarm type selector should sit outside collapsible panels`
  );
  assert.strictEqual(
    await page.locator("#sp-inp-alarm-entity").evaluate((el) => !!el.closest(".sp-disclosure")),
    false,
    `${label}: alarm entity field should sit outside collapsible panels`
  );

  await cardSettings.locator("> .sp-disclosure-button").click();
  assert(await cardSettings.getByText("Label Display", { exact: true }).isVisible(), `${label}: alarm card settings panel should contain label display controls`);
  assert(await cardSettings.getByText("Icon Display", { exact: true }).isVisible(), `${label}: alarm card settings panel should contain icon display controls`);
  const labelInput = page.locator("#sp-inp-alarm-label");
  await labelInput.waitFor({ state: "attached" });
  assert.strictEqual(
    await labelInput.isVisible(),
    false,
    `${label}: alarm label input starts hidden when status label display is selected`,
  );

  await cardSettings
    .locator(".sp-field")
    .filter({ hasText: "Label Display" })
    .getByRole("button", { name: "Name", exact: true })
    .click();
  assert(
    await labelInput.isVisible(),
    `${label}: alarm label input appears when name label display is selected`,
  );
  assert(
    await labelInput.evaluate((el) => {
      var field = el.closest(".sp-cond-field");
      var displayField = field && field.previousElementSibling;
      return !!(
        field &&
        field.classList.contains("sp-visible") &&
        displayField &&
        displayField.textContent.indexOf("Label Display") !== -1
      );
    }),
    `${label}: alarm label input is shown directly below label display controls`,
  );

  await cardSettings
    .locator(".sp-field")
    .filter({ hasText: "Label Display" })
    .getByRole("button", { name: "Status", exact: true })
    .click();
  assert.strictEqual(
    await labelInput.isVisible(),
    false,
    `${label}: alarm label input hides again when status label display is selected`,
  );

  await modalSettings.locator("> .sp-disclosure-button").click();
  assert(await modalSettings.getByText("Visible Actions", { exact: true }).isVisible(), `${label}: alarm modal settings panel should contain visible actions controls`);
  const pinSettings = modalSettings.locator(".sp-disclosure").filter({ hasText: "PIN Settings" }).first();
  assert(await pinSettings.isVisible(), `${label}: alarm modal settings panel should contain PIN settings`);
  assert(!(await pinSettings.getAttribute("class")).includes("sp-open"), `${label}: alarm PIN settings panel should start collapsed`);
  await pinSettings.locator("> .sp-disclosure-button").click();
  assert(await pinSettings.getByText("PIN required for arming", { exact: true }).isVisible(), `${label}: alarm PIN settings panel should contain arming PIN controls`);
  assert(await pinSettings.getByText("PIN required for disarming", { exact: true }).isVisible(), `${label}: alarm PIN settings panel should contain disarming PIN controls`);

  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
}

async function assertPlaylistValidationOpensSourcePanel(page, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");
  await page.locator('.sp-main [data-slot="4"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page.locator("#sp-inp-media-mode").selectOption("playlist");
  await page.waitForSelector("#sp-inp-playlist-content-id");

  const sourceSettings = page
    .locator(".sp-settings-modal .sp-disclosure")
    .filter({ hasText: "Source" })
    .first();
  assert(await sourceSettings.isVisible(), `${label}: playlist source panel should render`);
  await page.locator("#sp-inp-playlist-content-id").fill("");
  if ((await sourceSettings.getAttribute("class")).includes("sp-open")) {
    await sourceSettings.locator("> .sp-disclosure-button").click();
  }
  assert(
    !(await sourceSettings.getAttribute("class")).includes("sp-open"),
    `${label}: playlist source panel should be collapsed before validation`,
  );

  await page.getByRole("button", { name: "Save" }).click();
  await page.waitForFunction(() => {
    var input = document.querySelector("#sp-inp-playlist-content-id");
    var disclosure = input && input.closest(".sp-disclosure");
    return disclosure && disclosure.classList.contains("sp-open");
  });
  assert(
    await page.getByText("Add a media ID before saving.", { exact: true }).isVisible(),
    `${label}: playlist content ID error should be visible after validation`,
  );

  await page.locator(".sp-settings-close").click();
  await page.waitForFunction(() => {
    var overlay = document.querySelector(".sp-settings-overlay");
    return overlay && !overlay.classList.contains("sp-visible");
  });
}

function postRecord(requestUrl) {
  const url = new URL(requestUrl);
  const parts = url.pathname
    .split("/")
    .filter(Boolean)
    .map((part) => decodeURIComponent(part));
  return {
    time: Date.now(),
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
    if (posts.slice(startIndex).some((post) => postMatches(post, expected)))
      return;
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  assert.fail(
    `${label}: expected POST ${JSON.stringify(expected)}, got ${JSON.stringify(posts.slice(startIndex), null, 2)}`,
  );
}

async function waitForAnyPost(posts, expectedList, label, startIndex = 0) {
  const deadline = Date.now() + 4000;
  while (Date.now() < deadline) {
    if (
      posts
        .slice(startIndex)
        .some((post) =>
          expectedList.some((expected) => postMatches(post, expected)),
        )
    )
      return;
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  assert.fail(
    `${label}: expected one of ${JSON.stringify(expectedList)}, got ${JSON.stringify(posts.slice(startIndex), null, 2)}`,
  );
}

function backupButtons(count) {
  const buttons = Array.from({ length: count }, () => ({}));
  buttons[0] = {
    entity: "light.kitchen",
    label: "Kitchen",
    icon: "Lightbulb",
    icon_on: "Lightbulb",
  };
  buttons[1] = {
    entity: "sensor.energy",
    label: "Energy",
    icon: "Gauge",
    sensor: "sensor.energy",
    unit: "W",
    type: "sensor",
    precision: "0",
  };
  buttons[2] = { label: "Rooms", icon: "Home", type: "subpage" };
  buttons[3] = {
    entity: "media_player.living",
    label: "Media",
    type: "media",
    sensor: "play_pause",
  };
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
      3: "~B,1,2,6,5,3,4,7|,switch.office_filament_heater_893b,Heater,Movie Roll,,sensor.office_filament_heater_sensor_humidity,%25,,confirm_off%2Cconfirm_on|,switch.office_3d_printer_power,Printer,Printer 3D,,sensor.centauri_carbon_percent_complete_helper,%25|,switch.battery_charger,Battery,Battery 30%25,Battery Charging|H,climate.central_heating,Heating|C,cover.office_blind,Blind,Blinds Open,Blinds,modal,,,cover_tabs=controls%257Cposition%257Ctilt|Y,alarm_control_panel.alarmo,Alarm,Security|H,climate.office_air_conditioner,Aircon",
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
      cover_art_screensaver: true,
      cover_art_media_player_entity: "media_player.living",
      cover_art_attribute_conditions: "app_id=com.apple.TVMusic",
      cover_art_delay: 30,
      cover_art_track_overlay_duration: 10,
      cover_art_hide_external_input: false,
      home_assistant_artwork_protocol: "https",
      home_assistant_artwork_port: 9443,
      firmware_auto_update: false,
      firmware_update_frequency: "Weekly",
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
      schedule_sensor_activation: "on",
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
  const file = path.join(
    os.tmpdir(),
    `${name}-${process.pid}-${Date.now()}.json`,
  );
  fs.writeFileSync(
    file,
    typeof value === "string" ? value : JSON.stringify(value, null, 2),
  );
  return file;
}

async function openBackupControls(page) {
  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  if (!(await page.getByRole("button", { name: "Import" }).isVisible())) {
    await page.getByText("Backup", { exact: true }).click();
  }
  await page
    .getByRole("button", { name: "Import" })
    .waitFor({ state: "visible" });
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
      var descriptor = Object.getOwnPropertyDescriptor(
        Node.prototype,
        "textContent",
      );
      if (descriptor && descriptor.set && descriptor.get) {
        Object.defineProperty(Node.prototype, "textContent", {
          get: function () {
            return descriptor.get.call(this);
          },
          set: function (value) {
            if (this.classList && this.classList.contains("sp-banner")) {
              window.__bannerMessages = window.__bannerMessages || [];
              window.__bannerMessages.push({
                className: this.className || "",
                text: String(value || ""),
              });
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
    window.__bannerObserver.observe(banner, {
      attributes: true,
      childList: true,
      subtree: true,
    });
    record();
  });
}

async function assertBackupImportSmoke(page, posts, testCase) {
  const before = posts.length;
  await importBackup(
    page,
    backupFixture(testCase.slug, testCase.slots),
    "same-device-backup",
  );
  await page.waitForSelector(".sp-banner.sp-success");
  assert(
    (await page.locator(".sp-banner").textContent()).includes(
      "Configuration imported successfully",
    ),
    "same-device import succeeds",
  );
  await waitForPost(
    posts,
    { domain: "text", name: "button_on_color", action: "set", value: "AA5500" },
    "backup color import",
    before,
  );
  await waitForPost(
    posts,
    { domain: "text", name: "button_3_config", action: "set" },
    "backup subpage button config",
    before,
  );
  await waitForPost(
    posts,
    { domain: "text", name: "Subpage 3 Config", action: "set" },
    "backup subpage config",
    before,
  );
  await waitForPost(
    posts,
    { domain: "text", name: "Subpage 3 Config Ext", action: "set" },
    "backup subpage config extension",
    before,
  );
  await waitForPost(
    posts,
    {
      domain: "select",
      name: "screen__timezone",
      action: "set",
      option: "Europe/London (GMT+0)",
    },
    "backup timezone import",
    before,
  );
  await waitForPost(
    posts,
    { domain: "select", name: "screen__language", action: "set", option: "en" },
    "backup language import",
    before,
  );
  await waitForPost(
    posts,
    { domain: "switch", name: "screen__clock_bar_time", action: "turn_on" },
    "backup clock bar time reset",
    before,
  );
  const screensaverImportPosts = [
    [
      { domain: "text", name: "screensaver_mode", action: "set", value: "timer" },
      "backup screensaver mode import",
    ],
    [
      {
        domain: "text",
        name: "presence_sensor_entity",
        action: "set",
        value: "binary_sensor.office_presence",
      },
      "backup screensaver presence import",
    ],
    [
      {
        domain: "switch",
        name: "screen_saver__media_player_sleep_prevention",
        action: "turn_on",
      },
      "backup media sleep prevention import",
    ],
    [
      {
        domain: "text",
        name: "media_player_sleep_prevention_entity",
        action: "set",
        value: "media_player.living",
      },
      "backup media sleep prevention entity import",
    ],
    [
      { domain: "switch", name: "screen_saver__cover_art", action: "turn_on" },
      "backup cover art import",
    ],
    [
      {
        domain: "text",
        name: "screen_saver__cover_art_entity",
        action: "set",
        value: "media_player.living",
      },
      "backup cover art entity import",
    ],
    [
      {
        domain: "text",
        name: "screen_saver__cover_art_conditions",
        action: "set",
        value: "app_id=com.apple.TVMusic",
      },
      "backup cover art conditions import",
    ],
    [
      {
        domain: "number",
        name: "screen_saver__cover_art_delay",
        action: "set",
        value: "30",
      },
      "backup cover art delay import",
    ],
    [
      {
        domain: "number",
        name: "screen_saver__track_overlay_duration",
        action: "set",
        value: "10",
      },
      "backup cover art track overlay import",
    ],
    [
      {
        domain: "switch",
        name: "screen_saver__hide_cover_art_on_external_input",
        action: "turn_off",
      },
      "backup cover art external input import",
    ],
    [
      {
        domain: "select",
        name: "home_assistant_artwork_protocol",
        action: "set",
        option: "https",
      },
      "backup Home Assistant artwork protocol import",
    ],
    [
      {
        domain: "number",
        name: "home_assistant_artwork_port",
        action: "set",
        value: "9443",
      },
      "backup Home Assistant artwork port import",
    ],
    [
      {
        domain: "switch",
        name: "firmware__auto_update",
        action: "turn_off",
      },
      "backup firmware auto-update import",
    ],
    [
      {
        domain: "select",
        name: "firmware__update_frequency",
        action: "set",
        option: "Weekly",
      },
      "backup firmware update frequency import",
    ],
    [
      {
        domain: "select",
        name: "screen_saver__action",
        action: "set",
        option: "Screen Dimmed",
      },
      "backup screensaver action import",
    ],
    [
      { domain: "switch", name: "screen_saver__clock", action: "turn_off" },
      "backup clock screensaver switch import",
    ],
    [
      {
        domain: "number",
        name: "screen_saver__dimmed_brightness",
        action: "set",
        value: "15",
      },
      "backup dimmed screensaver brightness import",
    ],
    [
      { domain: "number", name: "screensaver_timeout", action: "set", value: "60" },
      "backup screensaver timeout import",
    ],
    [
      { domain: "number", name: "home_screen_timeout", action: "set", value: "120" },
      "backup home screen timeout import",
    ],
  ];
  for (const [expected, label] of screensaverImportPosts) {
    await waitForPost(posts, expected, label, before);
  }
  await waitForPost(
    posts,
    {
      domain: "number",
      name: "Screen: Daytime Brightness",
      action: "set",
      value: "88",
    },
    "backup brightness import",
    before,
  );
  await waitForPost(
    posts,
    { domain: "select", name: "screen__rotation", action: "set", option: "90" },
    "backup rotation import",
    before,
  );
  const importPosts = posts.slice(before);
  assert(
    importPosts.length >= 3 && importPosts[1].time - importPosts[0].time >= 40,
    `backup import posts should be throttled for device stability: ${JSON.stringify(importPosts.slice(0, 3), null, 2)}`,
  );

  await importBackup(page, "{", "invalid-backup");
  await page.waitForSelector(".sp-banner.sp-error");
  assert(
    (await page.locator(".sp-banner").textContent()).includes(
      "could not parse JSON",
    ),
    "invalid JSON shows an error",
  );

  await startBannerCapture(page);
  await importBackup(
    page,
    backupFixture("different-panel", 3),
    "cross-device-backup",
  );
  await page.waitForSelector(".sp-banner.sp-success");
  const warnings = await page.evaluate(() => window.__bannerMessages || []);
  assert(
    warnings.some(
      (entry) =>
        entry.className.includes("sp-warning") &&
        (entry.text.includes("different panel") ||
          entry.text.includes("slots")),
    ),
    `cross-device import shows an adaptation warning: ${JSON.stringify(warnings)}`,
  );
}

async function entitySuggestionValues(
  page,
  inputSelector,
  query = "light",
  expectedValues = [],
) {
  await page.locator(inputSelector).fill(query);
  const suggestions = await page.waitForFunction(
    ({ selector, query, expectedValues }) => {
      const input = document.querySelector(selector);
      const normalizedQuery = String(query || "").toLowerCase();
      if (!input || String(input.value || "").toLowerCase() !== normalizedQuery)
        return false;
      const options = Array.from(
        input.parentElement.querySelectorAll(
          ".sp-entity-dropdown.sp-open .sp-entity-option",
        ),
      );
      if (!options.length) return false;
      const values = options.map((option) => String(option.textContent || ""));
      if (
        !options.every((option) =>
          String(option.textContent || "")
            .toLowerCase()
            .includes(normalizedQuery),
        )
      )
        return false;
      if (!expectedValues.every((value) => values.indexOf(value) !== -1))
        return false;
      return values;
    },
    { selector: inputSelector, query, expectedValues },
  );
  return suggestions.jsonValue();
}

async function assertEditSmoke(page, posts, errors) {
  const before = posts.length;
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");

  await page.locator('.sp-main [data-slot="1"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  const switchSuggestions = await entitySuggestionValues(
    page,
    "#sp-inp-entity",
    "light",
    ["light.kitchen"],
  );
  assert(
    switchSuggestions.includes("light.kitchen"),
    "switch card suggestions include a recently used light",
  );
  assert(
    !switchSuggestions.includes("sensor.energy"),
    "switch card suggestions exclude recently used sensors",
  );
  assert(
    !switchSuggestions.includes("media_player.living"),
    "switch card suggestions exclude recently used media players",
  );
  await page.locator("#sp-inp-label").fill("Kitchen Main");
  await page.locator("#sp-inp-entity").fill("switch.kitchen_main");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "button_1_config",
      action: "set",
      value: "switch.kitchen_main;Kitchen Main;Lightbulb;Lightbulb",
    },
    "switch card edit",
    before,
  );

  await page.locator('.sp-main [data-slot="2"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.locator("#sp-inp-label").fill("Energy Usage");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "button_2_config",
      action: "set",
      value: "sensor.energy;Energy Usage;Gauge;Auto;sensor.energy;W;sensor;0",
    },
    "sensor card edit",
    before,
  );

  await page.locator('.sp-main [data-slot="4"]').click();
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.locator("#sp-inp-label").fill("Living Media");
  await page.getByRole("button", { name: "Save" }).click();
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "button_4_config",
      action: "set",
    },
    "media card edit",
    before,
  );

  assert.deepStrictEqual(
    errors,
    [],
    "browser errors were reported during edit interactions",
  );
}

async function assertApplySmoke(page, posts, errors) {
  const before = posts.length;
  await page.getByRole("button", { name: "Apply Configuration" }).click();
  await waitForPost(
    posts,
    {
      domain: "button",
      name: "Apply Configuration",
      action: "press",
    },
    "apply configuration",
    before,
  );
  assert.deepStrictEqual(
    errors,
    [],
    "browser errors were reported during edit interactions",
  );
}

async function openPasteCardCodeDialog(page) {
  const emptyCell = page.locator(".sp-main .sp-empty-cell").first();
  assert(await emptyCell.isVisible(), "card transfer test requires an empty destination cell");
  const pos = await emptyCell.getAttribute("data-pos");
  await emptyCell.click({ button: "right", force: true });
  await page.locator(".sp-ctx-menu").waitFor({ state: "visible" });
  await page
    .locator(".sp-ctx-menu")
    .getByText("Paste Code…", { exact: true })
    .click();
  await page.locator(".sp-transfer-dialog").waitFor({ state: "visible" });
  const dialog = page.locator(".sp-transfer-dialog");
  assert.strictEqual(
    await dialog.getByRole("heading", { name: "Paste Code", exact: true }).count(),
    1,
    "paste dialog uses the concise title",
  );
  const cancel = dialog.getByRole("button", { name: "Cancel", exact: true });
  const paste = dialog.getByRole("button", { name: "Paste", exact: true });
  assert(
    await cancel.evaluate((button) =>
      button.classList.contains("sp-action-btn") && button.classList.contains("sp-cancel-btn"),
    ),
    "paste dialog cancel action uses the standard modal button style",
  );
  assert(
    await paste.evaluate((button) =>
      button.classList.contains("sp-action-btn") && button.classList.contains("sp-save-btn"),
    ),
    "paste dialog primary action uses the standard modal button style",
  );
  return { dialog, pos };
}

async function assertCardTransferSmoke(page, posts, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.locator('.sp-main [data-slot="1"]').click({ button: "right", force: true });
  await page.locator(".sp-ctx-menu").waitFor({ state: "visible" });
  assert(
    await page.locator(".sp-ctx-menu").getByText("Copy Code", { exact: true }).isVisible(),
    `${label}: card context menu exposes transfer-code copying`,
  );
  await page.locator(".sp-ctx-menu").getByText("Copy Code", { exact: true }).click();
  const copyDialog = page.locator(".sp-transfer-dialog");
  await copyDialog.waitFor({ state: "visible" });
  assert.strictEqual(
    await copyDialog.getByRole("heading", { name: "Copy Code", exact: true }).count(),
    1,
    `${label}: copy dialog uses the concise title`,
  );
  const code = await copyDialog.locator("textarea").inputValue();
  const envelope = JSON.parse(code);
  assert.strictEqual(envelope.format, "espcontrol.cards", `${label}: copied card code has the format marker`);
  assert.strictEqual(envelope.version, 1, `${label}: copied card code uses version 1`);
  assert.strictEqual(envelope.cards.length, 1, `${label}: single-card code contains one card`);
  assert(
    String(envelope.cards[0].entity || "").includes("."),
    `${label}: copied code preserves the configured entity`,
  );
  assert.strictEqual(
    await copyDialog.getByText("Copy this code to another controller.", { exact: true }).count(),
    1,
    `${label}: copy dialog uses concise guidance`,
  );
  assert.strictEqual(
    await copyDialog.getByRole("button", { name: "Copy Code" }).count(),
    0,
    `${label}: copy dialog does not show a non-functional copy button`,
  );
  assert.strictEqual(
    await copyDialog.locator(".sp-transfer-actions").count(),
    0,
    `${label}: copy dialog does not show footer actions`,
  );
  assert.strictEqual(
    await copyDialog.getByText(/Press (Command|Ctrl)\+C to copy\./).count(),
    0,
    `${label}: copy dialog does not show a clipboard shortcut instruction`,
  );
  const copySelection = await copyDialog.locator("textarea").evaluate((textarea) => ({
    start: textarea.selectionStart,
    end: textarea.selectionEnd,
    length: textarea.value.length,
  }));
  assert.deepStrictEqual(
    copySelection,
    { start: 0, end: copySelection.length, length: copySelection.length },
    `${label}: card code is selected for manual copying`,
  );
  const dialogFont = await copyDialog.evaluate((element) => getComputedStyle(element).fontFamily);
  assert(/Inter|Segoe UI|Roboto|sans-serif/i.test(dialogFont), `${label}: copy dialog uses the web UI font stack`);
  const codeFont = await copyDialog.locator("textarea").evaluate((element) => getComputedStyle(element).fontFamily);
  assert(/ui-monospace|SFMono|Menlo|Consolas|monospace/i.test(codeFont), `${label}: transfer code uses a monospace font`);
  const closeControl = await copyDialog.locator(".sp-transfer-close").evaluate((button) => {
    const rect = button.getBoundingClientRect();
    const icon = button.querySelector(".sp-transfer-close-icon path");
    return {
      buttonType: button.type,
      hasInlineIcon: !!icon,
      width: rect.width,
      height: rect.height,
      touchAction: getComputedStyle(button).touchAction,
    };
  });
  assert.strictEqual(closeControl.buttonType, "button", `${label}: close control cannot submit another form`);
  assert(closeControl.hasInlineIcon, `${label}: close control uses a self-contained icon`);
  assert(closeControl.width >= 36 && closeControl.height >= 36, `${label}: close control has a usable target size`);
  assert.strictEqual(closeControl.touchAction, "manipulation", `${label}: close control responds promptly to touch`);
  await copyDialog.locator(".sp-transfer-close").click();
  await copyDialog.waitFor({ state: "detached" });

  const beforePaste = posts.length;
  const destination = await openPasteCardCodeDialog(page);
  await destination.dialog.locator("textarea").fill(code);
  await destination.dialog.getByRole("button", { name: "Paste", exact: true }).click();
  await page.locator(`.sp-main [data-pos="${destination.pos}"][data-slot]`).waitFor({ state: "visible" });
  const pastedSlot = await page
    .locator(`.sp-main [data-pos="${destination.pos}"]`)
    .getAttribute("data-slot");
  await waitForAnyPost(
    posts,
    [
      { domain: "text", name: `button_${pastedSlot}_config`, action: "set" },
      { domain: "text", name: `Button ${pastedSlot} Config`, action: "set" },
    ],
    `${label}: transferred card is saved`,
    beforePaste,
  );
  assert(
    (await page.locator(".sp-banner").textContent()).includes("Card pasted"),
    `${label}: successful transfer is reported`,
  );

  const noRoom = JSON.parse(code);
  noRoom.cards = Array.from({ length: 20 }, () => ({ ...noRoom.cards[0] }));
  await page.waitForTimeout(500);
  const beforeNoRoom = posts.length;
  const noRoomDialog = await openPasteCardCodeDialog(page);
  await noRoomDialog.dialog.locator("textarea").fill(JSON.stringify(noRoom));
  await noRoomDialog.dialog.getByRole("button", { name: "Paste", exact: true }).click();
  await page.waitForFunction(() => {
    const error = document.querySelector(".sp-transfer-error");
    return error && /not enough room/.test(error.textContent || "");
  });
  assert.strictEqual(posts.length, beforeNoRoom, `${label}: an impossible bulk paste writes nothing`);

  const unknown = JSON.parse(code);
  unknown.cards[0].type = "not_a_real_card";
  await noRoomDialog.dialog.locator("textarea").fill(JSON.stringify(unknown));
  await noRoomDialog.dialog.getByRole("button", { name: "Paste", exact: true }).click();
  await page.waitForFunction(() => {
    const error = document.querySelector(".sp-transfer-error");
    return error && /does not support/.test(error.textContent || "");
  });
  assert.strictEqual(posts.length, beforeNoRoom, `${label}: an unknown card type writes nothing`);

  const oversized = JSON.parse(code);
  oversized.cards[0].label = "x".repeat(300);
  await noRoomDialog.dialog.locator("textarea").fill(JSON.stringify(oversized));
  await noRoomDialog.dialog.getByRole("button", { name: "Paste", exact: true }).click();
  await page.waitForFunction(() => {
    const error = document.querySelector(".sp-transfer-error");
    return error && /settings are too large/.test(error.textContent || "");
  });
  assert.strictEqual(posts.length, beforeNoRoom, `${label}: an oversized card config writes nothing`);
  await noRoomDialog.dialog.getByRole("button", { name: "Cancel" }).click();

  const local = JSON.parse(code);
  local.cards[0] = {
    ...local.cards[0],
    entity: "local_action_key",
    type: "action",
    sensor: "local",
    options: "",
  };
  const localDialog = await openPasteCardCodeDialog(page);
  await localDialog.dialog.locator("textarea").fill(JSON.stringify(local));
  await localDialog.dialog.getByRole("button", { name: "Paste", exact: true }).click();
  await page.waitForSelector(".sp-banner.sp-warning");
  assert(
    (await page.locator(".sp-banner").textContent()).includes("Review local device references"),
    `${label}: local-device transfers show a review warning`,
  );

  const subpage = JSON.parse(code);
  subpage.cards[0] = {
    entity: "",
    label: "Transferred Page",
    icon: "Folder",
    icon_on: "Auto",
    sensor: "generic",
    unit: "",
    type: "subpage",
    precision: "",
    options: "",
    size: 3,
    subpage: {
      order: ["B", "1"],
      back_label: "Return",
      buttons: [{
        entity: "switch.transferred",
        label: "Transferred Switch",
        icon: "Toggle Switch",
        icon_on: "Toggle Switch",
        sensor: "",
        unit: "",
        type: "",
        precision: "",
        options: "",
      }],
    },
  };
  const beforeSubpage = posts.length;
  const subpageDialog = await openPasteCardCodeDialog(page);
  await subpageDialog.dialog.locator("textarea").fill(JSON.stringify(subpage));
  await subpageDialog.dialog.getByRole("button", { name: "Paste", exact: true }).click();
  const transferredSubpage = page
    .locator(".sp-main [data-slot]")
    .filter({ hasText: "Transferred Page" })
    .first();
  await transferredSubpage.waitFor({ state: "visible" });
  const subpageSlot = await transferredSubpage.getAttribute("data-slot");
  await waitForPost(
    posts,
    { domain: "text", name: `Subpage ${subpageSlot} Config`, action: "set" },
    `${label}: transferred subpage configuration is saved`,
    beforeSubpage,
  );
}

async function assertClockBarEditorSmoke(page, posts, label) {
  await page.getByRole("tab", { name: "Screen" }).click();
  await page.waitForSelector("#sp-screen.sp-page.active");

  async function openClockBarContextMenu(item, expectedAction) {
    await page
      .locator(`[data-clockbar-item="${item}"]`)
      .click({ button: "right", force: true });
    await page.locator(".sp-ctx-menu").waitFor({ state: "visible" });
    assert(
      await page
        .locator(".sp-ctx-menu")
        .getByText(expectedAction, { exact: true })
        .isVisible(),
      `${label}: right click menu for ${item} shows ${expectedAction}`,
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

  assert.strictEqual(
    await page.locator("[data-clockbar-add]").count(),
    0,
    `${label}: clock bar add controls are removed`,
  );
  assert.strictEqual(
    await page.locator("#sp-clockbar-add-type").count(),
    0,
    `${label}: add-item selector is not present`,
  );
  assert.strictEqual(
    await page.locator("#sp-clockbar-weather-entity").count(),
    0,
    `${label}: clock bar weather editor is not present`,
  );
  assert.strictEqual(
    await page.locator("#sp-clockbar-temperature-entity-0").count(),
    0,
    `${label}: clock bar temperature editor is not present`,
  );

  const before = posts.length;
  await page
    .locator('.sp-clockbar-section[data-clockbar-section="left"]')
    .hover();
  await page.locator('[data-clockbar-item="time"]').click({ force: true });
  assert.strictEqual(
    await page
      .locator('[data-clockbar-item="time"]')
      .getAttribute("data-clockbar-section"),
    "middle",
    `${label}: clock bar time remains in the fixed middle section`,
  );
  assert.strictEqual(
    await page.locator(".sp-selection-bar.sp-visible").count(),
    1,
    `${label}: clicking clock bar selects an item`,
  );
  assert(
    (await page.locator(".sp-selection-bar").textContent()).includes(
      "Clock selected",
    ),
    `${label}: clock selection is labelled`,
  );
  assert.strictEqual(
    await page.getByRole("button", { name: "Edit", exact: true }).isDisabled(),
    true,
    `${label}: clock edit button is disabled`,
  );
  assert.strictEqual(
    await page
      .getByRole("button", { name: "Clock bar actions", exact: true })
      .count(),
    0,
    `${label}: clock selection does not expose redundant actions menu button`,
  );
  const selectedClockItem = page.locator('[data-clockbar-item="time"]');
  await page.mouse.move(0, 0);
  await page.waitForTimeout(250);
  const selectedClockStyle = await selectedClockItem.evaluate((el) => {
    var style = getComputedStyle(el);
    return {
      borderColor: style.borderTopColor,
      backgroundColor: style.backgroundColor,
    };
  });
  await selectedClockItem.hover();
  await page.waitForTimeout(250);
  const hoveredSelectedClockStyle = await selectedClockItem.evaluate((el) => {
    var style = getComputedStyle(el);
    return {
      borderColor: style.borderTopColor,
      backgroundColor: style.backgroundColor,
    };
  });
  assert.notStrictEqual(
    selectedClockStyle.borderColor,
    "rgba(0, 0, 0, 0)",
    `${label}: selected clock bar item has a visible border`,
  );
  assert.deepStrictEqual(
    hoveredSelectedClockStyle,
    selectedClockStyle,
    `${label}: selected clock bar item keeps its selected styling while hovered`,
  );
  await openClockBarContextMenu("time", "Hide Clock");
  await page
    .locator(".sp-ctx-menu")
    .getByText("Hide Clock", { exact: true })
    .click();
  await waitForPost(
    posts,
    { domain: "switch", name: "screen__clock_bar_time", action: "turn_off" },
    `${label}: hiding clock posts clock switch`,
    before,
  );
  assert(
    (
      await page.locator('[data-clockbar-item="time"]').getAttribute("class")
    ).includes("sp-clockbar-hidden"),
    `${label}: hidden clock is greyed in preview`,
  );
  await openClockBarContextMenu("time", "Show Clock");
  await page
    .locator(".sp-ctx-menu")
    .getByText("Show Clock", { exact: true })
    .click();
  await waitForPost(
    posts,
    { domain: "switch", name: "screen__clock_bar_time", action: "turn_on" },
    `${label}: showing clock posts clock switch`,
    before,
  );

  await openClockBarContextMenu("network", "Hide Connectivity");
  assert(
    (await page.locator(".sp-selection-bar").textContent()).includes(
      "Connectivity selected",
    ),
    `${label}: connectivity selection is labelled`,
  );
  assert.strictEqual(
    await page.getByRole("button", { name: "Edit", exact: true }).isDisabled(),
    true,
    `${label}: connectivity edit button is disabled`,
  );
  await page
    .locator(".sp-ctx-menu")
    .getByText("Hide Connectivity", { exact: true })
    .click();
  await waitForPost(
    posts,
    {
      domain: "switch",
      name: "screen__network_status_icon",
      action: "turn_off",
    },
    `${label}: hiding connectivity posts network switch`,
    before,
  );
  assert(
    (
      await page.locator('[data-clockbar-item="network"]').getAttribute("class")
    ).includes("sp-clockbar-hidden"),
    `${label}: hidden connectivity is greyed in preview`,
  );
  const hiddenNetworkPreview = await page
    .locator('[data-clockbar-item="network"] .sp-network-preview')
    .evaluate((el) => {
      const style = getComputedStyle(el);
      const rect = el.getBoundingClientRect();
      return {
        className: el.className,
        opacity: style.opacity,
        width: rect.width,
        height: rect.height,
      };
    });
  assert(
    hiddenNetworkPreview.className.includes("sp-visible"),
    `${label}: hidden connectivity keeps the preview icon visible`,
  );
  assert.strictEqual(
    hiddenNetworkPreview.opacity,
    "1",
    `${label}: hidden connectivity icon is not faded out by its own style`,
  );
  assert(
    hiddenNetworkPreview.width > 0 && hiddenNetworkPreview.height > 0,
    `${label}: hidden connectivity icon remains measurable`,
  );
  await openClockBarContextMenu("network", "Show Connectivity");
  await page
    .locator(".sp-ctx-menu")
    .getByText("Show Connectivity", { exact: true })
    .click();
  await waitForPost(
    posts,
    {
      domain: "switch",
      name: "screen__network_status_icon",
      action: "turn_on",
    },
    `${label}: showing connectivity posts network switch`,
    before,
  );

  await page
    .locator('[data-clockbar-item="temperature"]')
    .click({ force: true });
  assert(
    (await page.locator(".sp-selection-bar").textContent()).includes(
      "Temperature selected",
    ),
    `${label}: temperature selection is labelled`,
  );
  assert.strictEqual(
    await page.getByRole("button", { name: "Edit", exact: true }).isDisabled(),
    false,
    `${label}: temperature edit button is enabled`,
  );
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  await page
    .locator("#sp-clockbar-temperature-entity")
    .fill("sensor.porch_temperature");
  const degreeSymbolLabel = page.locator(
    'label[for="sp-clockbar-temperature-degree-symbol"]',
  );
  assert.strictEqual(
    await degreeSymbolLabel.count(),
    1,
    `${label}: degree-symbol label is clickable`,
  );
  await degreeSymbolLabel.click();
  assert.strictEqual(
    await page.locator("#sp-clockbar-temperature-degree-symbol").isChecked(),
    false,
    `${label}: clicking the degree-symbol label toggles the control`,
  );
  await page.getByRole("button", { name: "Save", exact: true }).click();
  await waitForAnyPost(
    posts,
    [
      {
        domain: "text",
        name: "Clock Bar: Temperature Entities",
        action: "set",
        value: "sensor.porch_temperature",
      },
      {
        domain: "text",
        name: "clock_bar__temperature_entities",
        action: "set",
        value: "sensor.porch_temperature",
      },
    ],
    `${label}: saving temperature posts entity`,
    before,
  );
  await waitForPost(
    posts,
    {
      domain: "switch",
      name: "screen__temperature_degree_symbol",
      action: "turn_off",
    },
    `${label}: saving temperature posts degree symbol`,
    before,
  );
  assert(
    !(
      await page
        .locator('[data-clockbar-item="temperature"] .sp-temp')
        .textContent()
    ).includes("\u00B0"),
    `${label}: saving temperature removes the degree symbol from the preview`,
  );
  await page
    .locator('[data-clockbar-item="temperature"]')
    .click({ force: true });
  await page.getByRole("button", { name: "Edit", exact: true }).click();
  await page.waitForSelector(".sp-settings-overlay.sp-visible");
  const modalHideButton = page.locator(".sp-settings-modal .sp-hide-btn");
  assert.strictEqual(
    await modalHideButton.count(),
    1,
    `${label}: temperature modal shows one hide button`,
  );
  assert.strictEqual(
    (await modalHideButton.innerText()).trim(),
    "Hide",
    `${label}: temperature modal hide button is labelled`,
  );
  const hideIconClass = await modalHideButton
    .locator(".mdi")
    .getAttribute("class");
  assert(
    hideIconClass.includes("mdi-eye-off-outline"),
    `${label}: temperature modal hide button uses the eye-off icon`,
  );
  assert(
    !hideIconClass.includes("trash"),
    `${label}: temperature modal hide button does not use a trash icon`,
  );
  await modalHideButton.click();
  await waitForPost(
    posts,
    { domain: "switch", name: "outdoor_temp_enable", action: "turn_off" },
    `${label}: hiding temperature posts visibility switch`,
    before,
  );
  assert(
    (
      await page
        .locator('[data-clockbar-item="temperature"]')
        .getAttribute("class")
    ).includes("sp-clockbar-hidden"),
    `${label}: hidden temperature is greyed in preview`,
  );
  await page
    .locator('[data-clockbar-item="temperature"]')
    .click({ force: true });
  await page.getByRole("button", { name: "Show", exact: true }).click();
  await waitForPost(
    posts,
    { domain: "switch", name: "outdoor_temp_enable", action: "turn_on" },
    `${label}: showing temperature posts visibility switch`,
    before,
  );

  await openClockBarContextMenu("temperature", "Edit Temperature");
  assert(
    await page
      .locator(".sp-ctx-menu")
      .getByText("Hide Temperature", { exact: true })
      .isVisible(),
    `${label}: right click menu for temperature includes visibility action`,
  );
  await page
    .locator(".sp-ctx-menu")
    .getByText("Hide Temperature", { exact: true })
    .click();
  await waitForPost(
    posts,
    { domain: "switch", name: "outdoor_temp_enable", action: "turn_off" },
    `${label}: hiding temperature posts visibility switch`,
    before,
  );
  assert(
    (
      await page
        .locator('[data-clockbar-item="temperature"]')
        .getAttribute("class")
    ).includes("sp-clockbar-hidden"),
    `${label}: hidden temperature is greyed in preview`,
  );
  await openClockBarContextMenu("temperature", "Show Temperature");
  await page
    .locator(".sp-ctx-menu")
    .getByText("Show Temperature", { exact: true })
    .click();
  await waitForPost(
    posts,
    { domain: "switch", name: "outdoor_temp_enable", action: "turn_on" },
    `${label}: showing temperature posts visibility switch`,
    before,
  );
  assert.strictEqual(
    await page.locator(".sp-settings-overlay.sp-visible").count(),
    0,
    `${label}: clock bar temperature dialog closes after save`,
  );

  const screenBox = await page.locator(".sp-screen").boundingBox();
  const timeBox = await page
    .locator('[data-clockbar-item="time"][data-clockbar-section="middle"]')
    .boundingBox();
  assert(screenBox && timeBox, `${label}: middle clock bar time is measurable`);
  const screenCenter = screenBox.x + screenBox.width / 2;
  const timeCenter = timeBox.x + timeBox.width / 2;
  assert(
    Math.abs(timeCenter - screenCenter) <= 1,
    `${label}: middle clock bar time remains centered on the screen`,
  );

  const topbarBox = await page.locator(".sp-topbar").boundingBox();
  const networkBox = await page
    .locator('[data-clockbar-item="network"][data-clockbar-section="right"]')
    .boundingBox();
  const firstCardBox = await page.locator(".sp-main > *").first().boundingBox();
  assert(
    topbarBox && networkBox,
    `${label}: network status has a visible bounded area`,
  );
  assert(
    networkBox.y > topbarBox.y,
    `${label}: network status does not touch the top of the clock bar`,
  );
  assert(
    networkBox.y + networkBox.height < topbarBox.y + topbarBox.height,
    `${label}: network status does not touch the bottom of the clock bar`,
  );
  assert(firstCardBox, `${label}: first card has a visible bounded area`);
  assert(
    networkBox.y + networkBox.height < firstCardBox.y,
    `${label}: network status stays clear of the first card row`,
  );

  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const timeSettingsCard = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Time$/ }),
    })
    .first();
  const timeSettingsText = await timeSettingsCard.textContent();
  assert(
    timeSettingsText.includes("Timezone"),
    `${label}: timezone remains in global time settings`,
  );
  assert(
    timeSettingsText.includes("Clock Format"),
    `${label}: clock format remains in global time settings`,
  );
  const clockBarCard = page
    .locator("#sp-settings .card")
    .filter({ hasText: "Clock Bar" })
    .first();
  const clockBarText = await clockBarCard.textContent();
  assert(
    clockBarText.includes("Show Clock Bar"),
    `${label}: clock bar settings keep the master toggle`,
  );
  assert(
    !clockBarText.includes("Show Network Status Icon"),
    `${label}: network toggle moved out of clock bar settings`,
  );
  assert(
    !clockBarText.includes("Outdoor Temperature"),
    `${label}: outdoor controls moved out of clock bar settings`,
  );
  assert(
    !clockBarText.includes("Indoor Temperature"),
    `${label}: indoor controls moved out of clock bar settings`,
  );
  assert(
    !clockBarText.includes("Show Degree Symbol"),
    `${label}: degree-symbol control moved out of clock bar settings`,
  );
  await page.getByRole("tab", { name: "Screen" }).click();
}

async function assertNightScheduleSensorControls(page, posts, label) {
  await page.getByRole("tab", { name: "Settings" }).click();
  await page.waitForSelector("#sp-settings.sp-page.active");
  const card = page
    .locator("#sp-settings .card")
    .filter({
      has: page.locator(".card-header h3", { hasText: /^Night Schedule$/ }),
    })
    .first();
  if (await card.evaluate((element) => element.classList.contains("collapsed"))) {
    await card.locator(".card-header").click();
  }

  const timeButton = card.getByRole("button", { name: "Time", exact: true });
  const sensorButton = card.getByRole("button", { name: "Sensor", exact: true });
  const disabledButton = card.getByRole("button", { name: "Disabled", exact: true });
  const timeFields = card.locator("#sp-set-schedule-on-hour");
  const sensorField = card.locator("#sp-set-schedule-presence");
  const sensorFieldLabel = card.getByText("Sensor Entity", { exact: true });
  const sensorActivation = card.locator("#sp-set-schedule-sensor-activation");
  const actions = card.locator("#sp-set-schedule-actions");
  const actionSelect = card.locator("#sp-set-schedule-mode");
  const wakeTimeout = card.locator("#sp-set-schedule-wake-timeout");
  const dimmedBrightness = card.locator("#sp-set-schedule-dimmed-brightness");
  const clockBrightness = card.locator("#sp-set-schedule-clock-brightness");
  const clockTextColor = card.locator("#sp-set-schedule-clock-text-color");
  async function actionGroupGap() {
    return actions.evaluate((element) => {
      let previous = element.previousElementSibling;
      while (previous && getComputedStyle(previous).display === "none") {
        previous = previous.previousElementSibling;
      }
      if (!previous) return 0;
      return element.getBoundingClientRect().top - previous.getBoundingClientRect().bottom;
    });
  }

  let before = posts.length;
  await timeButton.click();
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "screen__schedule_trigger",
      action: "set",
      value: "time",
    },
    `${label}: selecting time schedule posts its trigger`,
    before,
  );
  assert(await timeFields.isVisible(), `${label}: Time mode should show time fields`);
  assert.strictEqual(
    await sensorField.isVisible(),
    false,
    `${label}: Time mode should hide the sensor field`,
  );
  assert.strictEqual(
    await sensorActivation.isVisible(),
    false,
    `${label}: Time mode should hide the sensor activation field`,
  );
  assert(await actions.isVisible(), `${label}: Time mode should show night action controls`);
  assert(
    (await actionGroupGap()) >= 22,
    `${label}: Time mode should leave space before the night action controls`,
  );
  assert(await wakeTimeout.isVisible(), `${label}: Screen Off should show wake controls`);
  assert.strictEqual(
    await dimmedBrightness.isVisible(),
    false,
    `${label}: Screen Off should hide dimmed brightness`,
  );
  assert.strictEqual(
    await clockBrightness.isVisible(),
    false,
    `${label}: Screen Off should hide clock controls`,
  );

  before = posts.length;
  await actionSelect.selectOption("clock");
  await waitForPost(
    posts,
    {
      domain: "select",
      name: "screen__schedule_mode",
      action: "set",
      option: "Clock",
    },
    `${label}: selecting the night clock posts the shared action`,
    before,
  );
  assert.strictEqual(
    await wakeTimeout.isVisible(),
    false,
    `${label}: Clock should hide Screen Off wake controls`,
  );
  assert(await clockBrightness.isVisible(), `${label}: Clock should show brightness`);
  assert(await clockTextColor.isVisible(), `${label}: Clock should show its text colour`);

  before = posts.length;
  await sensorButton.click();
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "screen__schedule_trigger",
      action: "set",
      value: "sensor",
    },
    `${label}: selecting sensor schedule posts its trigger`,
    before,
  );
  assert.strictEqual(
    await timeFields.isVisible(),
    false,
    `${label}: Sensor mode should hide time fields`,
  );
  assert(await sensorField.isVisible(), `${label}: Sensor mode should show the sensor entity`);
  assert(await sensorFieldLabel.isVisible(), `${label}: Sensor mode should label the sensor entity clearly`);
  assert.strictEqual(
    await sensorField.getAttribute("placeholder"),
    "Sensor Entity",
    `${label}: Sensor mode should use the sensor entity field prompt`,
  );
  assert(await sensorActivation.isVisible(), `${label}: Sensor mode should show the sensor activation field`);
  assert.strictEqual(
    await sensorActivation.inputValue(),
    "off",
    `${label}: Sensor mode should default to activating when the sensor is off`,
  );
  assert(await actions.isVisible(), `${label}: Sensor mode should show night action controls`);
  assert(
    (await actionGroupGap()) >= 22,
    `${label}: Sensor mode should leave space before the night action controls`,
  );
  assert.strictEqual(
    await actionSelect.inputValue(),
    "clock",
    `${label}: switching to Sensor mode should preserve the selected night action`,
  );
  assert(await clockBrightness.isVisible(), `${label}: Sensor clock should show brightness`);
  assert(await clockTextColor.isVisible(), `${label}: Sensor clock should show its text colour`);

  before = posts.length;
  await sensorActivation.selectOption("on");
  await waitForPost(
    posts,
    {
      domain: "select",
      name: "screen__schedule_sensor_activation",
      action: "set",
      option: "Sensor On",
    },
    `${label}: Sensor mode posts the selected activation state`,
    before,
  );
  assert.strictEqual(
    await sensorActivation.inputValue(),
    "on",
    `${label}: Sensor activation choice should remain selected`,
  );

  before = posts.length;
  await sensorField.fill("binary_sensor.all_lights_on");
  await sensorField.blur();
  await clockBrightness.evaluate((input) => {
    input.value = "4";
    input.dispatchEvent(new Event("input", { bubbles: true }));
    input.dispatchEvent(new Event("change", { bubbles: true }));
  });
  await clockTextColor.fill("330000");
  await clockTextColor.blur();
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "presence_sensor_entity",
      action: "set",
      value: "binary_sensor.all_lights_on",
    },
    `${label}: Sensor mode posts the sensor entity`,
    before,
  );
  await waitForPost(
    posts,
    {
      domain: "number",
      name: "screen__schedule_clock_brightness",
      action: "set",
      value: "4",
    },
    `${label}: Sensor mode posts clock brightness`,
    before,
  );
  await waitForPost(
    posts,
    {
      domain: "text",
      name: "Screen: Schedule Clock Text Color",
      action: "set",
      value: "330000",
    },
    `${label}: Sensor mode posts clock text colour`,
    before,
  );

  await actionSelect.selectOption("screen_dimmed");
  assert(await dimmedBrightness.isVisible(), `${label}: Sensor Dimmed should show brightness`);
  assert.strictEqual(
    await clockBrightness.isVisible(),
    false,
    `${label}: Sensor Dimmed should hide clock controls`,
  );
  await actionSelect.selectOption("screen_off");
  assert(await wakeTimeout.isVisible(), `${label}: Sensor Screen Off should show wake controls`);
  assert.strictEqual(
    await dimmedBrightness.isVisible(),
    false,
    `${label}: Sensor Screen Off should hide dimmed brightness`,
  );

  await timeButton.click();
  assert.strictEqual(
    await sensorField.isVisible(),
    false,
    `${label}: returning to Time mode should hide the sensor field`,
  );
  await sensorButton.click();
  assert.strictEqual(
    await sensorField.inputValue(),
    "binary_sensor.all_lights_on",
    `${label}: trigger changes should preserve the sensor entity`,
  );

  await disabledButton.click();
  assert.strictEqual(
    await timeFields.isVisible(),
    false,
    `${label}: Disabled should hide time fields after interaction`,
  );
  assert.strictEqual(
    await sensorField.isVisible(),
    false,
    `${label}: Disabled should hide the sensor field after interaction`,
  );
  assert.strictEqual(
    await actions.isVisible(),
    false,
    `${label}: Disabled should hide shared night action controls`,
  );

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
    if (message.type() === "error" || message.type() === "warning")
      errors.push(`[${message.type()}] ${message.text()}`);
  });
  page.on("request", (request) => {
    const requestUrl = new URL(request.url());
    if (
      request.method() === "POST" &&
      requestUrl.hostname === "espcontrol.test"
    ) {
      posts.push(postRecord(request.url()));
    }
  });
  await installFakeEventSource(page);

  try {
    await page.goto(`http://espcontrol.test/${testCase.slug}?events=1`, {
      waitUntil: "domcontentloaded",
    });
    await page.waitForSelector("#sp-app");
    await page.waitForFunction(
      () => window.__eventSources && window.__eventSources.length > 0,
    );
    await page.evaluate(
      (events) => window.__seedEspState(events),
      seededEvents(),
    );
    await page.waitForSelector(".sp-main > .sp-btn");
    await page.waitForTimeout(100);

    assert.deepStrictEqual(
      errors,
      [],
      `${testCase.name}: browser errors were reported`,
    );
    assertNoLayoutBreaks(
      await measureCoreLayout(page),
      testCase.name,
      testCase,
    );
    await assertSettingsPage(page, testCase.name, testCase);
    if (testCase.exerciseInteractions) {
      await assertNightScheduleSensorControls(page, posts, testCase.name);
    }
    assertNoLayoutBreaks(
      await measureCoreLayout(page),
      `${testCase.name} after settings`,
      testCase,
    );
    await assertCoverSettingsPanels(page, testCase.name);
    await assertMediaCoverArtSettingsPanels(page, testCase.name);
    await assertAlarmSettingsPanels(page, testCase.name);
    await assertPlaylistValidationOpensSourcePanel(page, testCase.name);
    if (testCase.exerciseInteractions) {
      await assertMobileTabLayout(page, testCase.name, testCase.viewport);
    }
    await assertEmptyCellSettings(page, posts, testCase.name);
    if (testCase.exerciseInteractions) {
      await assertClockBarEditorSmoke(page, posts, testCase.name);
      await assertBackupImportSmoke(page, posts, testCase);
      await assertEditSmoke(page, posts, errors);
      await assertCardTransferSmoke(page, posts, testCase.name);
      await assertApplySmoke(page, posts, errors);
    } else if (testCase.exerciseDeviceMocks) {
      await assertBackupImportSmoke(page, posts, testCase);
    }
  } catch (error) {
    fs.mkdirSync(FAILURE_DIR, { recursive: true });
    try {
      await page.screenshot({
        path: path.join(FAILURE_DIR, `${testCase.name}-${testCase.slug}.png`),
        fullPage: true,
        timeout: 5000,
      });
    } catch (screenshotError) {
      console.error(`${testCase.name}: could not capture failure screenshot: ${screenshotError.message}`);
    }
    throw error;
  } finally {
    await context.close();
  }
}

(async function main() {
  const browser = await chromium.launch();
  try {
    await assertPageTitleEvents(browser);
    await assertRotationStartupOrdering(browser);
    for (const testCase of CASES) {
      await runCase(browser, testCase);
      await assertMobileDeviceViewport(browser, testCase);
    }
  } finally {
    await browser.close();
  }
  console.log(
    `Browser web smoke checks passed for ${CASES.length} generated layouts.`,
  );
})().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
