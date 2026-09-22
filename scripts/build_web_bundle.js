#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const esbuild = require("esbuild");

const ROOT = path.resolve(__dirname, "..");
const ENTRY = path.join(ROOT, "src", "webserver", "entry.ts");

function overlayPlugin(overlays) {
  const normalized = new Map(
    Object.entries(overlays || {}).map(([filePath, content]) => [
      path.resolve(filePath),
      content,
    ]),
  );
  return {
    name: "generated-output-overlay",
    setup(build) {
      build.onLoad({ filter: /.*/ }, (args) => {
        const contents = normalized.get(path.resolve(args.path));
        if (contents === undefined) return null;
        const extension = path.extname(args.path).slice(1);
        const loader =
          extension === "ts" ? "ts" : extension === "json" ? "json" : "text";
        return { contents, loader };
      });
    },
  };
}

async function bundleApp(devices, embeddedMdiStyles, testHooks, overlays) {
  const defaultDeviceId = testHooks ? Object.keys(devices)[0] : "";
  const timezoneOptions = Object.values(devices)[0].timezoneOptions;
  const profiles = Object.fromEntries(
    Object.entries(devices).map(([slug, config]) => {
      if (
        JSON.stringify(config.timezoneOptions) !==
        JSON.stringify(timezoneOptions)
      ) {
        throw new Error(
          `${slug}: timezone options differ from the shared profile data`,
        );
      }
      const { timezoneOptions: _shared, ...profile } = config;
      return [slug, profile];
    }),
  );
  const result = await esbuild.build({
    bundle: true,
    define: {
      __ESPCONTROL_DEFAULT_DEVICE_ID__: JSON.stringify(defaultDeviceId),
      __ESPCONTROL_DEVICE_PROFILES__: JSON.stringify(profiles),
      __ESPCONTROL_TIMEZONE_OPTIONS__: JSON.stringify(timezoneOptions),
      __ESPCONTROL_EMBEDDED_MDI_STYLES__: JSON.stringify(embeddedMdiStyles),
      __ESPCONTROL_TEST_HOOKS_ENABLED__: testHooks ? "true" : "false",
    },
    entryPoints: [ENTRY],
    format: "iife",
    logLevel: "silent",
    minify: true,
    platform: "browser",
    plugins: [overlayPlugin(overlays)],
    target: "es2020",
    write: false,
  });
  if (result.outputFiles.length !== 1)
    throw new Error("esbuild returned an unexpected output set");
  return result.outputFiles[0].text;
}

function legacyDeviceLoader(slug) {
  return `(()=>{const c=document.currentScript,u=new URL("../www.js",c.src),s=document.createElement("script");u.search=c.src.includes("?")?c.src.slice(c.src.indexOf("?")):"";u.searchParams.set("device",${JSON.stringify(slug)});s.src=u.href;document.head.appendChild(s)})();\n`;
}

function webAssetBridge() {
  return `(()=>{const c=document.currentScript,s=c&&c.getAttribute("src"),u=new URL(s||"/webserver/www.js",window.location.href),q=u.search,p=u.searchParams.get("device")||"",v=u.searchParams.get("v")||"",z=()=>{const g=globalThis.__ESPCONTROL_START_EMBEDDED__;typeof g==="function"&&g()};const h=new URL(window.location.href);if(h.searchParams.has("espcontrol_fallback")){h.searchParams.delete("espcontrol_fallback");try{history.replaceState(null,"",h.pathname+h.search+h.hash)}catch(_){}z();return}const l=x=>fetch(x,{cache:"no-store"}).then(r=>r.ok?r.json():null).catch(()=>null),m=l(new URL("web-assets.json",u)),f=v?Promise.resolve(null):l("/espcontrol/version.json"),a=l("/api/v1/capabilities");Promise.all([m,f,a]).then(([M,F,A])=>{const d=p||String(F&&F.device_slug||""),w=v||String(F&&(F.firmware_version||F.version)||""),x=A&&A.web_assets&&Array.isArray(A.web_assets.versions)?A.web_assets.versions:[],b=M&&Array.isArray(M.bundles)?M.bundles:[],e=b.find(B=>(!d||Array.isArray(B.deviceProfiles)&&B.deviceProfiles.includes(d))&&(!w||!Array.isArray(B.firmwareVersions)||B.firmwareVersions.includes(w))&&(!x.length||!B.webAssetVersion||x.includes(B.webAssetVersion)));if(!e||typeof e.path!=="string"){z();return}const n=new URL(e.path,u),t=document.createElement("script");n.search=q,t.src=n.href,t.onerror=z,document.head.appendChild(t)}).catch(z)})();\n`;
}

function protectedApp(bundle) {
  return `(()=>{if(globalThis.__ESPCONTROL_UI_STARTED__||globalThis.__ESPCONTROL_UI_STARTING__)return;try{${bundle}}catch(e){const r=globalThis.__ESPCONTROL_RELOAD_EMBEDDED__;typeof r==="function"&&r();throw e}})();\n`;
}

function embeddedFallback(bundle) {
  return `(()=>{let e=!1;const t=()=>{if(e)return;e=!0;globalThis.__ESPCONTROL_USING_EMBEDDED__=!0;${bundle}},r=()=>{if(globalThis.__ESPCONTROL_USING_EMBEDDED__){t();return}try{const u=new URL(window.location.href);u.searchParams.set("espcontrol_fallback","1");window.location.replace(u.href)}catch(_){t()}};globalThis.__ESPCONTROL_START_EMBEDDED__=t;globalThis.__ESPCONTROL_RELOAD_EMBEDDED__=r;setTimeout(()=>{globalThis.__ESPCONTROL_UI_STARTED__||globalThis.__ESPCONTROL_UI_STARTING__||t()},1500)})();\n`;
}

async function main() {
  const request = JSON.parse(fs.readFileSync(0, "utf8"));
  if (!request.outputDir || !request.devices || !request.embeddedMdiStyles)
    throw new Error("Expected outputDir, devices, and embeddedMdiStyles");
  const outputPath = path.join(request.outputDir, "app.js");
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  const app = protectedApp(
    await bundleApp(
      request.devices,
      request.embeddedMdiStyles,
      !!request.testHooks,
      request.overlays,
    ),
  );
  fs.writeFileSync(outputPath, app);
  fs.writeFileSync(path.join(request.outputDir, "embedded.js"), embeddedFallback(app));
  fs.writeFileSync(path.join(request.outputDir, "www.js"), webAssetBridge());
  for (const slug of Object.keys(request.devices)) {
    const legacyPath = path.join(request.outputDir, slug, "www.js");
    fs.mkdirSync(path.dirname(legacyPath), { recursive: true });
    fs.writeFileSync(legacyPath, legacyDeviceLoader(slug));
  }
}

main().catch((error) => {
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
});
