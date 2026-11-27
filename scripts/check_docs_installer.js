// Check the exact flashing dependency resolved from ESP Web Tools. No USB writes.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { createRequire } = require('node:module');
const { buildSync } = require('esbuild');

const root = path.resolve(__dirname, '..');
const installerRequire = createRequire(require.resolve('esp-web-tools'));
const p4Path = installerRequire.resolve('esptool-js/lib/targets/esp32p4.js');
const compiled = buildSync({ entryPoints: [p4Path], bundle: true, platform: 'node', format: 'cjs', write: false });
const loaded = { exports: {} };
new Function('module', 'exports', 'require', compiled.outputFiles[0].text)(loaded, loaded.exports, require);
const { ESP32P4ROM } = loaded.exports;

async function checkFlashPower(revision, romPowered = false) {
  const chip = new ESP32P4ROM();
  chip.getChipRevision = async () => revision;
  chip.usesUsbOtg = async () => false;
  chip.disableWatchdogs = async () => {};
  const registers = new Map([
    [chip.EFUSE_RD_REPEAT_DATA1_REG, romPowered ? chip.EFUSE_DOWNLOAD_MODE_XPD_ON_MASK : 0],
    [chip.PMU_DATE_REG, 0x103],
  ]);
  const writes = [];
  await chip.postConnect({
    usesUsbOtg: async () => false,
    IS_STUB: false,
    secureDownloadMode: false,
    readReg: async address => registers.get(address) || 0,
    writeReg: async (address, value) => { writes.push([address, value]); registers.set(address, value); },
  });
  if (revision < 301) {
    assert.equal(writes.length, 0, 'Older P4 chips must not use the new flash power sequence');
  } else if (romPowered) {
    assert.deepEqual(writes, [[chip.PMU_DATE_REG, 0x100]], 'ECO7 must release ROM flash force-on without powering up twice');
  } else {
    assert(writes.some(([address, value]) => address === chip.LP_SYSTEM_REG_ANA_XPD_PAD_GROUP_REG && value === 1),
      `P4 revision ${revision}: postConnect must power on flash before attachment (esptool-js#268)`);
  }
}

async function checkPostFlashReset() {
  const { usbInstallerReset } = await import('../docs/.vitepress/usb-installer-reset.mjs');
  const flashPath = path.join(path.dirname(require.resolve('esp-web-tools')), 'flash.js');
  const source = fs.readFileSync(flashPath, 'utf8');
  const installer = usbInstallerReset();
  const flashSource = installer.transform(source, flashPath);
  assert.equal(installer.transform(source, flashPath + '?v=dev'), flashSource);
  const runFlash = new Function('Transport', 'ESPLoader', 'window', 'location', 'console', flashSource
    .replace(/^import .*;$/gm, '')
    .replace('export const flash', 'const sleep = async () => {}; const flash') + '\nreturn flash;');

  const v3 = 'guition-esp32-p4-jc1060p470-v3';
  for (const scenario of [
    { label: 'V3 manually booted over native USB', expected: 'watchdog' },
    { label: 'V3 booted into download mode by USB', straps: 0x8, expected: 'normal' },
    { label: 'V3 on a USB-to-UART adapter', nativeUsb: false, expected: 'normal' },
    { label: 'V2 manually booted over native USB', slug: 'guition-esp32-p4-jc1060p470-v2', expected: 'normal' },
    { label: 'V3 with a failed write', writeFails: true, expected: 'normal' },
  ]) {
    const calls = [];
    const chip = new ESP32P4ROM();
    chip.usesUsbJtagSerial = async () => scenario.nativeUsb !== false;
    chip.watchdogReset = async () => calls.push('watchdog');
    class Transport {
      async setRTS() { calls.push('RTS'); }
      async disconnect() { calls.push('disconnect'); }
    }
    class Loader {
      chip = chip;
      async main() {}
      async flashId() {}
      async readReg(address) {
        assert.equal(address, chip.GPIO_STRAP_REG);
        return scenario.straps ?? 0;
      }
      async writeFlash() {
        calls.push('write');
        if (scenario.writeFails) throw new Error('Write failed');
      }
      async after() { calls.push('normal'); }
    }
    const location = new URL('https://example.com/espcontrol/getting-started/install');
    const flash = runFlash(Transport, Loader, {}, location, { log() {} });
    const manifest = { builds: [{ chipFamily: 'ESP32-P4', parts: [] }] };
    const events = [];
    await flash(event => events.push(event), { getInfo: () => ({}) },
      `/espcontrol/firmware/${scenario.slug ?? v3}/manifest.json`, manifest, false);
    assert.deepEqual(calls, ['write', ...(scenario.expected === 'normal' ? ['RTS'] : []),
      scenario.expected, 'disconnect'], scenario.label);
    assert.equal(events.at(-1).state, scenario.writeFails ? 'error' : 'finished', scenario.label);
    if (!scenario.writeFails) {
      assert.equal(events.at(-1).restartedWithWatchdog, scenario.expected === 'watchdog', scenario.label);
    }
  }
  console.log('7-inch V3 post-flash reset checks passed.');

  const dialogPath = path.join(path.dirname(flashPath), 'install-dialog.js');
  const dialogSource = installer.transform(fs.readFileSync(dialogPath, 'utf8'), dialogPath + '?v=dev');
  const confirmInstall = dialogSource.match(/    async _confirmInstall\(\) \{([\s\S]+?)\n    \}/)[1];
  for (const { restartedWithWatchdog, improvWait, shouldReopen } of [
    { restartedWithWatchdog: true, improvWait: 0, shouldReopen: false },
    { restartedWithWatchdog: true, improvWait: 10, shouldReopen: true },
    { restartedWithWatchdog: false, improvWait: 0, shouldReopen: true },
  ]) {
    const calls = [];
    const wakeLockCalls = [];
    const state = { state: 'finished', restartedWithWatchdog };
    let reopen;
    const runConfirmInstall = new Function('flash', 'sleep', `return async function () {${confirmInstall}}`)(
      async onEvent => onEvent(state), () => new Promise(resolve => { reopen = resolve; }));
    const dialog = {
      _manifest: { new_install_improv_wait_time: improvWait },
      _wakeLock: {
        async acquire() { wakeLockCalls.push('acquire'); },
        async release() { wakeLockCalls.push('release'); },
      },
      port: {
        async close() { calls.push('close'); },
        async open() { calls.push('reopen'); },
      },
      async _initialize(justInstalled) { assert.equal(justInstalled, true); calls.push('initialize'); },
      requestUpdate() { calls.push('update'); },
    };
    await runConfirmInstall.call(dialog);
    assert.deepEqual(wakeLockCalls, ['acquire', 'release']);
    assert.equal(dialog._installState, state);
    assert.equal(Boolean(reopen), shouldReopen);
    if (shouldReopen) {
      reopen();
      await new Promise(resolve => setImmediate(resolve));
      assert.deepEqual(calls, ['close', 'reopen', 'initialize', 'update']);
    } else {
      assert.deepEqual(calls, ['close']);
      assert.equal(dialog._client, null, 'Show completion without checking for serial Wi-Fi setup');
    }
  }
  console.log('7-inch V3 installer completion checks passed.');
}

async function checkBrowser() {
  const { chromium } = require('playwright');
  const { createServer } = require('node:http');
  const dist = path.join(root, 'docs/.vitepress/dist');
  const server = createServer((request, response) => {
    const pathname = decodeURIComponent(new URL(request.url, 'http://localhost').pathname);
    const relative = pathname.replace(/^\/espcontrol\//, '');
    let file = path.resolve(dist, relative);
    if (!file.startsWith(dist + path.sep)) { response.writeHead(404).end(); return; }
    if (!path.extname(file)) file += '.html';
    if (!fs.existsSync(file)) { response.writeHead(404).end(); return; }
    response.setHeader('Content-Type', ({ '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.json': 'application/json' })[path.extname(file)] || 'application/octet-stream');
    fs.createReadStream(file).pipe(response);
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const base = `http://127.0.0.1:${server.address().port}`;
  let browser;
  try {
    browser = await chromium.launch();
    const page = await browser.newPage();
    const errors = [];
    const externalScripts = [];
    const manifests = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.route('**/*', route => {
      const request = route.request();
      if (!request.url().startsWith(base)) {
        if (request.resourceType() === 'script') externalScripts.push(request.url());
        return route.abort();
      }
      if (request.url().includes('/firmware/')) {
        manifests.push(request.url());
        return route.fulfill({ json: { name: 'Espcontrol', version: 'test', builds: [] } });
      }
      return route.continue();
    });
    await page.addInitScript(() => {
      // A cancelled port selection loads the real dialog without touching hardware.
      Object.defineProperty(navigator, 'serial', { value: { requestPort: async () => undefined } });
    });
    await page.goto(`${base}/espcontrol/getting-started/install`);
    await page.waitForFunction(() => document.querySelector('.esp-install-selector'));
    assert.equal(await page.locator('esp-web-install-button').count(), 0);
    assert.equal(manifests.length, 0, 'No panel firmware should be chosen on page load');
    const ten = page.locator('#guition-esp32-p4-jc8012p4a1-version');
    assert.equal(await ten.inputValue(), '');
    await page.locator('.device-choice').filter({ hasText: '10.1 inch' }).click();
    assert.equal(await page.locator('esp-web-install-button').count(), 0, 'Choosing a size must not choose its revision');
    await ten.selectOption('guition-esp32-p4-jc8012p4a1-v3');
    await page.waitForFunction(() => !!customElements.get('esp-web-install-button'));
    const install = page.locator('esp-web-install-button');
    await install.waitFor();
    assert.equal(await install.getAttribute('manifest'), '/espcontrol/firmware/guition-esp32-p4-jc8012p4a1-v3/manifest.json');
    await install.locator('button[slot="activate"]').click();
    await page.waitForFunction(() => !!customElements.get('ewt-install-dialog'));
    await page.locator('#guition-esp32-p4-jc4880p443-version').selectOption('guition-esp32-p4-jc4880p443-v3');
    await install.waitFor();
    assert.equal(await install.getAttribute('manifest'), '/espcontrol/firmware/guition-esp32-p4-jc4880p443-v3/manifest.json');
    await page.locator('#guition-esp32-p4-jc4880p443-version').selectOption('guition-esp32-p4-jc4880p443');
    await install.waitFor();
    assert.equal(await install.getAttribute('manifest'), '/espcontrol/firmware/guition-esp32-p4-jc4880p443/manifest.json');
    await page.locator('#guition-esp32-p4-jc1060p470-version').selectOption('guition-esp32-p4-jc1060p470-v3');
    await install.waitFor();
    assert.equal(await install.getAttribute('manifest'), '/espcontrol/firmware/guition-esp32-p4-jc1060p470-v3/manifest.json');
    for (const [pagePath, manifest] of [
      ['screens/jc1060p470-v3', '/espcontrol/firmware/guition-esp32-p4-jc1060p470-v3/manifest.json'],
      ['screens/jc4880p443-v3', '/espcontrol/firmware/guition-esp32-p4-jc4880p443-v3/manifest.json'],
      ['screens/jc8012p4a1-v3', '/espcontrol/firmware/guition-esp32-p4-jc8012p4a1-v3/manifest.json'],
    ]) {
      await page.goto(`${base}/espcontrol/${pagePath}`);
      await page.waitForFunction(() => !!customElements.get('esp-web-install-button'));
      await install.waitFor();
      assert.equal(await install.getAttribute('manifest'), manifest);
    }
    assert.deepEqual(errors, []);
    assert.deepEqual(externalScripts, [], 'USB installer must work without third-party scripts');
    console.log('Built-site installer checks passed (mocked Web Serial, no physical flashing).');
  } finally {
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
}

(async () => {
  for (const revision of [100, 300, 301, 302]) await checkFlashPower(revision);
  await checkFlashPower(302, true);
  console.log('P4 flash-power regression checks passed for the resolved installer dependency.');
  await checkPostFlashReset();
  if (process.argv.includes('--browser')) await checkBrowser();
})().catch(error => { console.error(error); process.exitCode = 1; });
