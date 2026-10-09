const assert = require("assert/strict");
const fs = require("fs");
const path = require("path");
const { createHash } = require("crypto");
const { gunzipSync, brotliDecompressSync } = require("zlib");

const template = fs.readFileSync(path.join(__dirname, "../components/captive_portal/portal.html"), "utf8");
const webStyles = require("./captive_portal_assets").sharedStyles();
const source = template.replace("/* __ESPCONTROL_WEB_STYLE__ */", webStyles);
// Keep provisioning behavior intact, allowing removal of unused metadata updates.
const script = source.match(/<script\b[^>]*>([\s\S]*?)<\/script>/)[1]
  .replace("document.title=`EspControl WiFi setup`;", "document.title=t.name,document.getElementById(`mac`).innerText=`MAC Address: `+t.mac,document.getElementById(`h1`).innerText=`WiFi Networks: `+t.name;");
assert.equal(createHash("sha256").update(script).digest("hex"),
  "6e9143fc4bf8cd3c370c03432fc6f94c99ab0380337a9dec0d52372dd034f7d6", "Confirmed captive portal script changed");
assert.ok(source.includes(webStyles), "Portal must embed the shared webserver stylesheet");
assert.ok(!/<(?:script|link)\b[^>]*(?:src|href)=["']?https?:/i.test(source), "Portal must work without internet assets");
const embedded = fs.readFileSync(path.join(__dirname, "../components/captive_portal/captive_index.h"), "utf8");
const arrays = [...embedded.matchAll(/INDEX_GZ\[\] PROGMEM = \{([\s\S]*?)\};/g)]
  .map(match => Buffer.from([...match[1].matchAll(/0x([0-9a-f]{2})/g)].map(byte => parseInt(byte[1], 16))));
assert.equal(arrays.length, 2);
assert.equal(gunzipSync(arrays[0]).toString(), source);
assert.equal(brotliDecompressSync(arrays[1]).toString(), source);
const { chromium } = require("playwright");
const { pageSource } = require("./captive_portal_assets");
const savedSource = pageSource("wifi_saved.html", webStyles);
const savedHeader = fs.readFileSync(path.join(__dirname, "../components/captive_portal/wifi_saved.h"), "utf8");
const savedArrays = [...savedHeader.matchAll(/WIFI_SAVED_GZ\[\] PROGMEM = \{([\s\S]*?)\};/g)]
  .map(match => Buffer.from([...match[1].matchAll(/0x([0-9a-f]{2})/g)].map(byte => parseInt(byte[1], 16))));
assert.equal(savedArrays.length, 2);
assert.equal(gunzipSync(savedArrays[0]).toString(), savedSource);
assert.equal(brotliDecompressSync(savedArrays[1]).toString(), savedSource);
assert.ok(!/<(?:script|link)\b[^>]*(?:src|href)=["']?https?:/i.test(savedSource));
const names = ["Unifi-Devices", 'Guest & "Family" café', "<img src=x onerror=alert(1)>"];
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.CHROME_BIN || "/usr/bin/google-chrome", headless: true });
  try {
    for (const width of [320, 360, 640]) {
      for (const scenario of ["selection", "empty", "offline", "saved", "confirmation"]) {
        const page = await browser.newPage({ viewport: { width, height: 900 } });
        const requests = [];
        await page.route("**/*", async route => {
          const url = new URL(route.request().url());
          requests.push(url.pathname);
          assert.equal(url.origin, "http://192.168.4.1");
          if (url.pathname === "/config.json") {
            if (scenario === "offline") await route.abort();
            else await route.fulfill({ json: { name: "espcontrol", mac: "30:ED:A0:E2:F3:6A", aps: scenario === "empty" ? [{}] : [{}, ...names.map(ssid => ({ ssid, rssi: -65, lock: true }))] } });
          } else if (url.pathname === "/") {
            await route.fulfill({ contentType: "text/html", body: pageSource(scenario === "confirmation" ? "wifi_saved.html" : "portal.html", webStyles) });
          } else throw new Error(`Unexpected request: ${url}`);
        });
        await page.goto(`http://192.168.4.1/${scenario === "saved" ? "?save" : ""}`);
        await page.waitForLoadState("networkidle");
        assert.equal(await page.title(), "EspControl WiFi setup");
        assert.equal(await page.locator(".sp-brand-title").textContent(), "EspControl");
        const layout = await page.evaluate(() => ({
          width: document.documentElement.scrollWidth,
          viewport: innerWidth,
          background: getComputedStyle(document.body).backgroundColor,
          radius: getComputedStyle(document.querySelector(".card")).borderRadius,
        }));
        assert.ok(layout.width <= layout.viewport, `${width}/${scenario}: horizontal overflow`);
        assert.equal(layout.background, "rgb(27, 27, 31)");
        assert.equal(layout.radius, width <= 480 ? "10px" : "12px");
        if (scenario === "confirmation") {
          assert.equal(await page.locator("h1").textContent(), "WiFi details saved");
          assert.deepEqual(await page.locator(".setup-steps li").allTextContents(), [
            "Reconnect your phone or computer to your home WiFi.",
            "Follow the instructions on your display’s screen to continue setup.",
          ]);
          assert.equal(await page.locator("form, input, button, script").count(), 0);
          assert.deepEqual(requests, ["/"]);
        } else {
          const password = page.locator("#psk");
          const ssid = page.locator("#ssid");
          await password.fill("typed-password");
          if (scenario === "selection" || scenario === "saved") {
            assert.deepEqual(await page.locator(".network-ssid").allTextContents(), names);
            assert.equal(await page.locator("#net img").count(), 0);
            const before = page.url();
            for (let index = 0; index < names.length; index++) {
              await page.locator(".network a").nth(index).click();
              assert.equal(await ssid.inputValue(), names[index]);
              assert.equal(await password.inputValue(), "typed-password");
              assert.equal(page.url(), before);
              assert.ok(await password.evaluate(node => node === document.activeElement));
            }
          } else {
            assert.equal(await page.locator(".network").count(), 0);
            await ssid.fill("Manual network");
          }
          assert.equal(await page.locator("label[for=ssid], label[for=psk]").count(), 2);
          assert.equal(await page.locator("h3").textContent(), "Available Networks");
          assert.equal(await page.locator("#net").getAttribute("aria-labelledby"), "available-networks-heading");
          assert.equal(await page.locator("#mac, #h1, #ota").count(), 0);
          assert.equal(await page.locator("form").count(), 1);
          assert.equal(await page.locator("form").getAttribute("action"), "/wifisave");
          assert.equal(await page.locator("aside").isVisible(), scenario === "saved");
          assert.equal(await ssid.evaluate(node => getComputedStyle(node).backgroundColor), "rgb(46, 46, 50)");
          assert.equal(await page.locator("button").evaluate(node => getComputedStyle(node).backgroundColor), "rgb(92, 115, 231)");
          const expectedSsid = await ssid.inputValue();
          const values = await page.locator("form").evaluate(form => {
            let values;
            form.addEventListener("submit", event => { event.preventDefault(); values = Object.fromEntries(new FormData(form)); }, { once: true });
            form.requestSubmit();
            return values;
          });
          assert.deepEqual(values, { ssid: expectedSsid, psk: "typed-password" });
          assert.deepEqual(requests, ["/", "/config.json"]);
        }
        await page.close();
      }
    }
  } finally { await browser.close(); }
  console.log("WiFi setup: gzip/Brotli parity, offline styles, 320/360/640px layouts, escaped SSIDs, network selection, manual entry, form submission and saved instructions passed.");
})().catch(error => { console.error(error); process.exitCode = 1; });
