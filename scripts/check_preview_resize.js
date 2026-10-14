"use strict";

const assert = require("node:assert/strict");
const path = require("node:path");
const esbuild = require("esbuild");
const { chromium } = require("playwright");
const { loadTypeScriptModule } = require("./load_typescript_module");

(async () => {
  const root = path.resolve(__dirname, "..");
  const { createWebStyles } = loadTypeScriptModule(path.join(root, "src/webserver/application/styles.ts"));
  const bundle = esbuild.buildSync({ entryPoints: [path.join(root, "src/webserver/application/preview_resize.ts")], bundle: true, format: "iife", globalName: "resize", write: false });
  const browser = await chromium.launch();
  try {
    const page = await browser.newPage();
    await page.setContent(`<style>${createWebStyles(false)} #grid{display:grid;width:320px;height:320px;gap:10px;grid-template-columns:repeat(3,1fr);grid-template-rows:repeat(3,1fr)} .sp-btn{background:#333;--btn-r:20px}</style><div id="grid"><div class="sp-btn" data-slot="1" draggable="true"><button class="sp-resize-handle">Resize</button></div>${'<div class="sp-empty-cell"></div>'.repeat(8)}</div>`);
    await page.addScriptTag({ content: bundle.outputFiles[0].text });
    await page.evaluate(() => {
      window.commits = [];
      window.locked = false;
      window.clicks = 0;
      resize.installPreviewResize({ document, isConfigLocked: () => window.locked,
        targets: () => [1, 2, 3].map(size => resize.resizeTargetDimensions(size, 100, 100, 10)),
        commit: (slot, size) => window.commits.push([slot, size]) });
      document.addEventListener("click", () => window.clicks++);
    });
    const opacity = () => page.locator(".sp-resize-handle").evaluate(el => getComputedStyle(el).opacity);
    const restored = () => page.waitForFunction(() => {
      const rect = document.querySelector(".sp-btn").getBoundingClientRect();
      return rect.width === 100 && rect.height === 100;
    });
    const emptyBounds = () => page.locator(".sp-empty-cell").evaluateAll(elements => elements.map(el => {
      const { x, y, width, height } = el.getBoundingClientRect();
      return { x, y, width, height };
    }));
    const initialBounds = await emptyBounds();
    assert.equal(await opacity(), "0");
    const start = async (dx = 110, dy = 0) => {
      await restored();
      await page.locator(".sp-btn").hover();
      assert.equal(await opacity(), "1");
      const box = await page.locator(".sp-resize-handle").boundingBox();
      await page.mouse.move(box.x + 16, box.y + 16);
      await page.mouse.down();
      await page.mouse.move(box.x + 16 + dx, box.y + 16 + dy);
    };
    await start();
    assert.equal(await page.locator(".sp-btn").evaluate(el => el.getBoundingClientRect().width), 210);
    assert.equal(await opacity(), "1");
    assert.deepEqual(await emptyBounds(), initialBounds, "horizontal resizing must not stretch or rearrange the grid");
    assert.equal(await page.locator(".sp-btn").evaluate(el => el.draggable), false);
    assert.equal(await page.locator(".sp-btn.sp-btn-wide").count(), 1);
    assert.deepEqual(await page.evaluate(() => window.commits), []);
    await page.mouse.up();
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    assert.equal(await page.evaluate(() => window.clicks), 0);
    assert.equal(await page.locator(".sp-btn").evaluate(el => el.draggable), true);
    await start(0, 110);
    assert.equal(await page.locator(".sp-btn").evaluate(el => el.getBoundingClientRect().height), 210);
    assert.deepEqual(await emptyBounds(), initialBounds, "vertical resizing must not stretch or rearrange the grid");
    assert.equal(await page.locator(".sp-btn.sp-btn-double").count(), 1);
    await page.keyboard.press("Escape");
    await page.mouse.up();
    await restored();
    assert.equal(await page.locator(".sp-btn.sp-btn-double").count(), 0);
    await start();
    await page.keyboard.press("Escape");
    await page.mouse.up();
    await restored();
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    await start();
    await page.locator(".sp-resize-handle").dispatchEvent("pointercancel", { pointerId: 1 });
    await page.mouse.up();
    assert.equal(await page.locator(".sp-resizing").count(), 0);
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    await page.evaluate(() => window.locked = true);
    await start();
    await page.mouse.up();
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    console.log("Card resize browser checks passed: stable horizontal/vertical grid layout, preview classes, drag exclusion, hover, release, Escape, cancellation, lock and click isolation.");
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
