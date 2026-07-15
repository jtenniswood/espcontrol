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

async function bundleApp(devices, testHooks, overlays) {
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

async function main() {
  const request = JSON.parse(fs.readFileSync(0, "utf8"));
  if (!request.outputDir || !request.devices)
    throw new Error("Expected outputDir and devices");
  const outputPath = path.join(request.outputDir, "www.js");
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(
    outputPath,
    await bundleApp(request.devices, !!request.testHooks, request.overlays),
  );
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
