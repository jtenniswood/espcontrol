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
    await page.setContent(`<style>${createWebStyles(false)} .sp-btn{width:100px;height:100px;background:#333;--btn-r:20px}</style><div class="sp-btn" data-slot="1"><button class="sp-resize-handle">Resize</button></div>`);
    await page.addScriptTag({ content: bundle.outputFiles[0].text });
    await page.evaluate(() => {
      window.commits = [];
      window.locked = false;
      window.clicks = 0;
      resize.installPreviewResize({ document, isConfigLocked: () => window.locked,
        targets: () => [resize.resizeTargetDimensions(1, 100, 100, 10), resize.resizeTargetDimensions(3, 100, 100, 10)],
        commit: (slot, size) => window.commits.push([slot, size]) });
      document.addEventListener("click", () => window.clicks++);
    });
    const opacity = () => page.locator(".sp-resize-handle").evaluate(el => getComputedStyle(el).opacity);
    const restored = () => page.waitForFunction(() => document.querySelector(".sp-btn").getBoundingClientRect().width === 100);
    assert.equal(await opacity(), "0");
    const start = async () => {
      await restored();
      await page.locator(".sp-btn").hover();
      assert.equal(await opacity(), "1");
      const box = await page.locator(".sp-resize-handle").boundingBox();
      await page.mouse.move(box.x + 16, box.y + 16);
      await page.mouse.down();
      await page.mouse.move(box.x + 126, box.y + 16);
    };
    await start();
    assert.equal(await page.locator(".sp-btn").evaluate(el => el.getBoundingClientRect().width), 210);
    assert.equal(await opacity(), "1");
    assert.deepEqual(await page.evaluate(() => window.commits), []);
    await page.mouse.up();
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    assert.equal(await page.evaluate(() => window.clicks), 0);
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
    console.log("Card resize browser checks passed: hover, preview, release, Escape, cancellation, lock and click isolation.");
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
