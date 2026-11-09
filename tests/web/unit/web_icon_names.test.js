"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { test } = require("node:test");
const { runtimeIconNames } = require("../../../scripts/web_icon_names");

const known = ["refresh", "home", "alert", "folder-plus", "sync"];
function scan(source) {
  return runtimeIconNames([{ file: "fixture.ts", source }], known);
}

test("icon discovery excludes TypeScript types and comments", () => {
  assert.deepEqual(scan(`
    import type { Home } from "home";
    type Loader = Pick<Controller, "refresh" | "sync">;
    interface Options { "alert": string; icon: "mdi-folder-plus" }
    const options: Record<"home", string> = {};
    // icon("refresh") and mdi-alert are documentation only.
    /** mdi-sync */
    const action = (name: "refresh"): void => {};
  `), []);
});

test("runtime literal icons remain discoverable after type erasure", () => {
  assert.deepEqual(scan(`
    type Action = "sync";
    const icon = "refresh";
    const markup = '<i class="mdi mdi-alert"></i>';
    const template = \`<span class="mdi-folder-plus">Home</span>\`;
    const withType = "home" as const;
    const unrelated = "not-an-icon";
  `), ["alert", "folder-plus", "home", "refresh"]);
});

test("unknown explicit MDI classes cannot evade missing-glyph validation", () => {
  assert.deepEqual(scan('const html = "<i class=mdi-missing-glyph></i>";'), ["missing-glyph"]);
});

test("the actual native reconnect type does not introduce a refresh glyph", () => {
  const file = path.resolve(__dirname, "../../../src/webserver/application/app_events.ts");
  const names = runtimeIconNames([{ file, source: fs.readFileSync(file, "utf8") }], known);
  assert(!names.includes("refresh"));
});

test("malformed runtime source fails validation", () => {
  assert.throws(() => scan("const icon = ;"), /Unexpected/);
});

test("compiler helper strings do not become browser glyphs", () => {
  const sources = [{ file: "private.ts", source: "class Counter { #count = 1; get() { return this.#count; } }" }];
  assert.deepEqual(runtimeIconNames(sources, ["symbol"]), []);
});
