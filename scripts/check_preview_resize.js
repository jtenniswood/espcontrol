"use strict";

const assert = require("node:assert/strict");
const path = require("node:path");
const esbuild = require("esbuild");
const { chromium, webkit } = require("playwright");
const { loadTypeScriptModule } = require("./load_typescript_module");

(async () => {
  const root = path.resolve(__dirname, "..");
  const { createWebStyles } = loadTypeScriptModule(path.join(root, "src/webserver/application/styles.ts"));
  const bundle = esbuild.buildSync({ stdin: { contents: 'export * from "./src/webserver/application/preview_resize"; export { syncPreviewCorners } from "./src/webserver/application/preview_corners";', resolveDir: root }, bundle: true, format: "iife", globalName: "resize", write: false });
  const engine = process.env.ESPCONTROL_BROWSER_ENGINE === "webkit" ? webkit : chromium;
  const browser = await engine.launch(process.env.ESPCONTROL_BROWSER_EXECUTABLE_PATH ? { executablePath: process.env.ESPCONTROL_BROWSER_EXECUTABLE_PATH } : {});
  try {
    const page = await browser.newPage();
    await page.setContent(`<style>${createWebStyles(false)} #grid{display:grid;width:320px;height:320px;gap:10px;grid-template-columns:repeat(3,1fr);grid-template-rows:repeat(3,1fr)} .sp-btn{background:#333;--btn-r:20px}</style><div id="grid"><div class="sp-btn" data-slot="1" draggable="true"><button class="sp-resize-handle"><svg viewBox="0 0 32 32"><path d="M 27 8 Q 24 24 8 27"/></svg></button></div><div class="sp-btn sp-move-test-card" data-slot="2" draggable="true">Other card</div>${'<div class="sp-empty-cell"></div>'.repeat(7)}</div>`);
    await page.addScriptTag({ content: bundle.outputFiles[0].text });
    await page.evaluate(() => {
      window.commits = [];
      window.locked = false;
      window.clicks = 0;
      window.nativeDrags = 0;
      resize.syncPreviewCorners(document.querySelector("#grid"), 5, 5, true);
      resize.installPreviewResize({ document, isConfigLocked: () => window.locked,
        targets: () => [1, 2, 3].map(size => resize.resizeTargetDimensions(size, 100, 100, 10)),
        commit: (slot, size) => window.commits.push([slot, size]) });
      document.addEventListener("click", () => window.clicks++);
      document.addEventListener("dragstart", () => window.nativeDrags++);
      // Match the editor's native drop: move a card and redraw all preview nodes.
      const grid = document.querySelector("#grid");
      let movingSlot = null;
      grid.addEventListener("dragstart", event => { movingSlot = event.target.closest(".sp-btn").dataset.slot; });
      grid.addEventListener("dragover", event => event.preventDefault());
      grid.addEventListener("drop", event => {
        event.preventDefault();
        const children = Array.from(grid.children);
        const from = children.findIndex(el => el.getAttribute("data-slot") === movingSlot);
        const to = children.indexOf(event.target.closest(".sp-empty-cell"));
        const next = children.map(el => el.cloneNode(true));
        [next[from], next[to]] = [next[to], next[from]];
        next.forEach(el => { if (el.classList.contains("sp-btn")) el.draggable = true; });
        grid.replaceChildren(...next);
      });
    });
    const card = page.locator('.sp-btn[data-slot="1"]');
    const opacity = () => page.locator(".sp-resize-handle").evaluate(el => getComputedStyle(el).opacity);
    const restored = () => page.waitForFunction(() => {
      const rect = document.querySelector('.sp-btn[data-slot="1"]').getBoundingClientRect();
      return rect.width === 100 && rect.height === 100;
    });
    const emptyBounds = () => page.locator(".sp-empty-cell,.sp-move-test-card").evaluateAll(elements => elements.map(el => {
      const { x, y, width, height } = el.getBoundingClientRect();
      return { x, y, width, height };
    }));
    let initialBounds = await emptyBounds();
    assert.equal(await opacity(), "0");
    const start = async (dx = 110, dy = 0) => {
      await restored();
      await page.mouse.move(0, 0);
      const box = await page.locator(".sp-resize-handle").boundingBox();
      assert.equal(await page.evaluate(({x, y}) => !!document.elementFromPoint(x, y)?.closest(".sp-resize-handle"), {x: box.x + 16, y: box.y + 16}), true, "the rounded clip must leave the resize handle clickable");
      await page.mouse.move(box.x + 16, box.y + 16);
      assert.equal(await opacity(), "1");
      assert.equal(await card.evaluate(el => el.draggable), false, "the resize corner must block native dragging before the first mouse-down");
      await page.mouse.down();
      await page.mouse.move(box.x + 16 + dx, box.y + 16 + dy);
    };
    const moveCard = async (slot) => {
      await page.mouse.move(0, 0);
      await page.locator(`.sp-btn[data-slot="${slot}"]`).dragTo(page.locator(".sp-empty-cell").first());
      assert.equal(await page.evaluate(() => window.nativeDrags), 1, "the regression must perform a real native card drag");
      await page.evaluate(() => window.nativeDrags = 0);
      initialBounds = await emptyBounds();
    };
    await moveCard(2);
    await start();
    assert.equal(await card.evaluate(el => el.getBoundingClientRect().width), 210);
    assert.equal(await opacity(), "1");
    assert.deepEqual(await emptyBounds(), initialBounds, "horizontal resizing must not stretch or rearrange the grid");
    assert.equal(await card.evaluate(el => el.draggable), false);
    assert.equal(await page.locator(".sp-btn.sp-btn-wide").count(), 1);
    assert.deepEqual(await page.evaluate(() => window.commits), []);
    await page.mouse.up();
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    assert.equal(await page.evaluate(() => window.clicks), 0);
    assert.equal(await card.evaluate(el => el.draggable), true);
    await card.hover();
    assert.equal(await card.evaluate(el => el.draggable), true, "card content must remain draggable");
    await moveCard(1);
    await start(0, 110);
    assert.equal(await card.evaluate(el => el.getBoundingClientRect().height), 210);
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
    await page.mouse.move(0, 0);
    const lockedHandle = await page.locator(".sp-resize-handle").boundingBox();
    await page.mouse.move(lockedHandle.x + 16, lockedHandle.y + 16);
    await page.mouse.down();
    await page.mouse.move(lockedHandle.x + 126, lockedHandle.y + 16);
    await page.mouse.up();
    assert.deepEqual(await page.evaluate(() => window.commits), [[1, 3]]);
    assert.equal(await page.evaluate(() => window.nativeDrags), 0);
    console.log("Card resize browser checks passed: native move-then-resize of the same or another card, stable horizontal/vertical grid layout, preview classes, drag exclusion, hover, release, Escape, cancellation, lock and click isolation.");
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
