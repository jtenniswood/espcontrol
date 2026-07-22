#!/usr/bin/env node
"use strict";

const assert = require("assert");
const path = require("path");
const vm = require("vm");
const { loadBuiltWebSource } = require("./web_source");

const SOURCE = path.resolve(__dirname, "..", "src", "webserver", "entry.ts");

function loadRestoreHook(fetchImpl) {
  const sandbox = {
    __ESPCONTROL_TEST_HOOKS__: {},
    console: { log() {}, warn() {}, error() {} },
    location: { search: "" },
    URLSearchParams,
    TextDecoder,
    TextEncoder,
    Uint8Array,
    fetch: fetchImpl,
    setTimeout,
    clearTimeout,
    requestAnimationFrame(fn) { return setTimeout(fn, 0); },
    document: {
      readyState: "loading",
      activeElement: null,
      addEventListener() {},
    },
  };
  sandbox.window = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(loadBuiltWebSource(), sandbox, { filename: SOURCE });
  return sandbox.__ESPCONTROL_TEST_HOOKS__.config.backupRestoreArchivedImages;
}

function manifestEntries(images) {
  const manifest = { format: "espcontrol.card-images", version: 1, images };
  const entries = {
    "images.json": new TextEncoder().encode(JSON.stringify(manifest)),
  };
  for (const image of images) {
    entries[image.file] = new Uint8Array([0xFF, 0xD8, 0xFF, 0xD9]);
  }
  return entries;
}

async function main() {
  let fetchCalls = 0;
  const emptyRestore = loadRestoreHook(() => {
    fetchCalls++;
    throw new Error("empty image restore must not access storage");
  });
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(await emptyRestore(manifestEntries([])))),
    {},
    "image-free ZIP restore succeeds without card-image storage"
  );
  assert.strictEqual(fetchCalls, 0, "image-free ZIP restore does not query card-image storage");

  const existingId = "img-existing-1";
  const requests = [];
  const existingRestore = loadRestoreHook((url, options) => {
    requests.push({ url, method: options && options.method || "GET" });
    if (url === "/api/card-images") {
      return Promise.resolve({
        ok: true,
        json: () => Promise.resolve({
          available: true,
          storage_bytes: 4096,
          used_bytes: 4096,
          free_bytes: 0,
          max_bytes: 65536,
          images: [{ id: existingId, name: "Existing", size: 4 }],
        }),
      });
    }
    throw new Error("existing image restore must not upload or rename images");
  });
  const idMap = await existingRestore(manifestEntries([
    { id: existingId, name: "Existing", file: `images/${existingId}.jpg`, size: 4 },
  ]));
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(idMap)),
    { [existingId]: existingId },
    "ZIP restore maps existing image IDs to themselves"
  );
  assert.deepStrictEqual(requests, [{ url: "/api/card-images", method: "GET" }],
    "ZIP restore reuses an existing image without consuming storage");

  console.log("Backup image restore checks passed.");
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
