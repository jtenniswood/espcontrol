#!/usr/bin/env node
// Capture the actual web editor against the repository's simulated panel.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const Module = require('node:module');
const { execFileSync } = require('node:child_process');
const { parseArgs } = require('node:util');

async function main() {
  const { values } = parseArgs({ options: {
    repo: { type: 'string' }, config: { type: 'string' }, out: { type: 'string' },
    'admin-slot': { type: 'string' }, 'admin-out': { type: 'string' },
  } });
  assert(values.repo && values.config && values.out,
    'Usage: capture-preview.cjs --repo WORKTREE --config FIXTURE.json --out SCREEN.png [--admin-slot SLOT --admin-out SETTINGS.png]');
  assert.equal(Boolean(values['admin-slot']), Boolean(values['admin-out']),
    '--admin-slot and --admin-out must be supplied together');
  const root = execFileSync('git', ['-C', values.repo, 'rev-parse', '--show-toplevel'],
    { encoding: 'utf8' }).trim();
  const config = JSON.parse(fs.readFileSync(values.config, 'utf8'));
  assert(config.buttons && typeof config.buttons === 'object' && !Array.isArray(config.buttons),
    'Fixture must include a buttons object');
  assert(Object.keys(config.buttons).length > 0, 'Fixture must include at least one card');
  assert(Object.entries(config.buttons).every(([slot, card]) => /^\d+$/.test(slot) && typeof card === 'string'),
    'Each button must map a numeric slot to a saved card record');
  assert(typeof config.settings?.button_order === 'string' && config.settings.button_order,
    'Fixture must include settings.button_order');
  if (values['admin-slot']) assert(config.buttons[values['admin-slot']], 'Admin slot must exist in the fixture');

  const sourcePath = path.join(root, 'scripts', 'check_web_browser_smoke.js');
  assert(fs.existsSync(sourcePath), 'This worktree has no supported EspControl browser harness');
  const source = fs.readFileSync(sourcePath, 'utf8');
  const entry = source.indexOf('(async function main() {');
  assert(entry > 0, 'Browser harness entrypoint changed; inspect the capture bridge before continuing');
  const filename = path.join(root, 'scripts', '__feature_capture_bridge.cjs');
  const bridge = new Module(filename, module);
  bridge.filename = filename;
  bridge.paths = Module._nodeModulePaths(path.dirname(filename));
  try {
    bridge._compile(source.slice(0, entry) + '\nmodule.exports = { chromium, nativeConfigState, nativeDocumentEvents, installRoutes, installFakeEventSource };', filename);
  } catch (error) {
    throw new Error('Cannot load the browser capture harness. Check repository dependencies and helper compatibility: ' + error.message);
  }
  const { chromium, nativeConfigState, nativeDocumentEvents, installRoutes, installFakeEventSource } = bridge.exports;
  const slug = 'esp32-p4-86';
  const state = nativeConfigState(slug);
  state.document = { deviceProfile: slug, buttons: config.buttons,
    settings: config.settings, subpages: config.subpages || {} };
  const browser = await chromium.launch({ headless: true });
  try {
    const context = await browser.newContext({ viewport: { width: 1280, height: 1000 }, deviceScaleFactor: 2 });
    await installRoutes(context, slug, { nativeState: state });
    const page = await context.newPage();
    page.setDefaultTimeout(20000);
    await installFakeEventSource(page);
    await page.goto('http://espcontrol.test/' + slug + '?events=1');
    await page.waitForSelector('#sp-app');
    await page.waitForFunction(() => window.__eventSources?.length > 0);
    await page.evaluate(events => window.__seedEspState(events), nativeDocumentEvents(state.document));
    await page.locator('.sp-main > .sp-btn[data-slot]').first().waitFor();
    await page.evaluate(() => document.fonts.ready);
    const screen = page.locator('.sp-screen');
    const bounds = await screen.boundingBox();
    assert(bounds && Math.abs(bounds.width / bounds.height - 1) < 0.03,
      'The saved photo template requires a square screen preview');
    const output = path.resolve(values.out);
    fs.mkdirSync(path.dirname(output), { recursive: true });
    await screen.screenshot({ path: output, animations: 'disabled' });
    const cards = await page.locator('.sp-main > .sp-btn[data-slot]').evaluateAll(nodes => nodes.map(node => ({
      slot: node.dataset.slot, text: node.textContent.trim(), background: getComputedStyle(node).backgroundColor,
    })));
    if (values['admin-slot']) {
      await page.locator('.sp-main [data-slot="' + values['admin-slot'] + '"]').click();
      await page.getByRole('button', { name: 'Edit', exact: true }).click();
      await page.waitForSelector('.sp-settings-overlay.sp-visible');
      const adminOutput = path.resolve(values['admin-out']);
      fs.mkdirSync(path.dirname(adminOutput), { recursive: true });
      await page.locator('.sp-settings-modal').screenshot({ path: adminOutput, animations: 'disabled' });
    }
    console.log(JSON.stringify({ output, deviceProfile: slug, cards }));
  } finally {
    await browser.close();
  }
}

main().catch(error => { console.error(error); process.exitCode = 1; });
