"use strict";
const { describe, test } = require("node:test");
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const { spawnSync } = require("node:child_process");
const { loadTypescriptTest } = require("./helpers/load_typescript_test");
const { decodeNativeV2, encodeNativeV2 } = loadTypescriptTest("src/webserver/model/panel_config_v2.ts");
const ROOT = path.resolve(__dirname, "../../..");
const fixture = JSON.parse(fs.readFileSync(path.join(ROOT, "compatibility/fixtures/panel_config_v2.json"), "utf8"));
const bytes = (hex) => Uint8Array.from(Buffer.from(hex, "hex"));
const rejects = (fn) => assert.throws(fn, (error) => error.name === "PanelConfigError");
function record(r) {
  return { kind: r.kind, id: r.id ?? 0, first: r.first ?? 0, second: r.second ?? 0,
    third: r.third ?? 0, fieldMask: r.field_mask ?? 0, flags: r.flags ?? 0,
    fields: Array.from({ length: 9 }, (_, i) => r.fields?.[String(i)] ?? 0),
    rawSource: r.raw_source ?? 0, text: r.text ?? "" };
}
const semantic = fixture.records.map(record).sort((a, b) => a.kind - b.kind || a.id - b.id);

describe("Staged native-v2 browser codec", () => {
  test("decodes independent semantic golden and writes independent canonical bytes", () => {
    assert.deepEqual(decodeNativeV2(bytes(fixture.wire_hex)), semantic);
    const sorted = [...fixture.records].sort((a, b) => a.kind - b.kind || (a.id ?? 0) - (b.id ?? 0));
    const payload = sorted.map((r) => {
      const body = Buffer.from(r.body_hex, "hex"), prefix = Buffer.alloc(3);
      prefix[0] = r.kind; prefix.writeUInt16LE(body.length, 1);
      return Buffer.concat([prefix, body]);
    });
    const header = Buffer.from(fixture.wire_hex, "hex").subarray(0, 16);
    assert.deepEqual(Buffer.from(encodeNativeV2(semantic)), Buffer.concat([header, ...payload]));
  });

  test("agrees with the C++ reader's independent truncation and hostile graph corpus", () => {
    const result = spawnSync("python3", ["tests/firmware/panel_config_document_v2_cases.py"], { cwd: ROOT, encoding: "utf8", maxBuffer: 16 * 1024 * 1024 });
    assert.equal(result.status, 0, result.stderr);
    const corpus = JSON.parse(result.stdout);
    assert.ok(corpus.length > 900);
    for (const { name, hex, valid } of corpus) {
      const run = () => decodeNativeV2(bytes(hex));
      if (valid) assert.doesNotThrow(run, name);
      else assert.throws(run, (error) => error.name === "PanelConfigError", name);
    }
  });

  test("retains BOM, NUL, astral text, inactive pages and sparse slot positions", () => {
    const input = structuredClone(semantic);
    input.find((r) => r.kind === 2 && r.id === 1).text = "\ufeffKitchen\0😀";
    input.find((r) => r.kind === 1).text = "\ufeffpanel";
    input.find((r) => r.kind === 5 && r.id === 11).second = 65535;
    const decoded = decodeNativeV2(encodeNativeV2(input));
    assert.deepEqual(decoded, input);
    // The decoded values own their text and IDs independently of the wire input.
    const encoded = encodeNativeV2(input), snapshot = decodeNativeV2(encoded);
    encoded.fill(0);
    assert.deepEqual(snapshot, input);
  });

  test("rejects lossy or unused writer fields, malformed strings and graph errors", () => {
    for (const update of [
      { kind: 1, id: 1, text: "a" },
      { kind: 1, text: "\ud800" },
      { kind: 1, text: "\udc00" },
      { kind: 1, text: "x".repeat(65) },
      { kind: 2, id: 1, text: "x".repeat(4096) },
      { kind: 2, id: 1, text: "a", first: 1 },
      { kind: 5, id: 90, first: 50, second: 2, fieldMask: 512 },
      { kind: 5, id: 90, first: 50, second: 2, rawSource: 1 },
      { kind: 5, id: 90, first: 50, second: 2, flags: 2 },
      { kind: 5, id: 90, first: 50, second: 2, fields: [1] },
      { kind: 5, id: 90, first: 50, second: 2, fields: Array(9).fill(1) },
      { kind: 5, id: 90, first: 50, second: 2, firstTypo: 1 },
      { kind: 5, id: 90, first: -1, second: 2 },
      { kind: 5, id: 90, first: 50, second: 0.5 },
      { kind: 5, id: 90, first: 50, second: NaN },
      { kind: 5, id: 90, first: 50, second: 65536 },
      { kind: 99, id: 1 },
    ]) {
      const input = structuredClone(semantic);
      const index = input.findIndex((r) => r.kind === update.kind && (r.id === update.id || update.kind === 1));
      if (index < 0) input.push(update); else input[index] = update;
      rejects(() => encodeNativeV2(input));
    }
    rejects(() => encodeNativeV2([...semantic, semantic[0]]));
    rejects(() => encodeNativeV2([]));
    const missing = semantic.filter((r) => r.kind !== 1);
    rejects(() => encodeNativeV2(missing));
  });

  test("accepts sliced inputs and deployed string boundary without enabling live v2", () => {
    const padded = Buffer.concat([Buffer.from([0]), Buffer.from(fixture.wire_hex, "hex"), Buffer.from([0])]);
    assert.deepEqual(decodeNativeV2(padded.subarray(1, -1)), semantic);
    const input = structuredClone(semantic);
    input.push({ kind: 2, id: 200, text: "x".repeat(4095) });
    assert.equal(decodeNativeV2(encodeNativeV2(input)).find((r) => r.kind === 2 && r.id === 200).text.length, 4095);
    const v1 = loadTypescriptTest("src/webserver/model/panel_config.ts");
    assert.equal(v1.PANEL_CONFIG_DOCUMENT_VERSION, 1);
    rejects(() => v1.decodePanelConfig(bytes(fixture.wire_hex)));
  });
});
